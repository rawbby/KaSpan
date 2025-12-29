import itertools
import json
import matplotlib.pyplot as plt
import numpy as np
import os
import pathlib
import re

CORES_PER_SOCKET = 38
CORES_PER_NODE = 76
TIME_LIMIT_S = 600
DPI = 200


def get_kaspan_duration(data):
    n = len(data)
    return max(int(data[str(i)]["benchmark"]["scc"]["duration"]) for i in range(n)) * 1e-9


def get_ispan_duration(data):
    n = len(data)
    return max(int(data[str(i)]["benchmark"]["scc"]["duration"]) for i in range(n)) * 1e-9


def get_hpc_graph_duration(data):
    n = len(data)
    return max(int(data[str(i)]["benchmark"]["scc"]["duration"]) for i in range(n)) * 1e-9


def get_kaspan_progress(data):
    n = len(data)

    def get_max_dur(path):
        durs = []
        for i in range(n):
            curr = data[str(i)]["benchmark"]["scc"]
            for p in path:
                if p in curr:
                    curr = curr[p]
                else:
                    return None
            durs.append(int(curr["duration"]))
        return max(durs) * 1e-9

    def get_decided(path):
        curr = data["0"]["benchmark"]["scc"]
        for p in path:
            if p in curr:
                curr = curr[p]
            else:
                return None
        if "decided_count" in curr:
            return int(curr["decided_count"])
        if path == ["tarjan"]:
            return int(data["0"]["benchmark"]["n"])
        return None

    res = []
    for stage_path in [["trim_1"], ["forward_backward_search"], ["ecl"], ["residual"], ["tarjan"]]:
        dur = get_max_dur(stage_path)
        decided = get_decided(stage_path)
        if dur is not None and decided is not None:
            res.append({"name": stage_path[-1], "duration": dur, "decided_count": decided})
    return res


def get_ispan_progress(data):
    n = len(data)

    def get_max_dur(path):
        durs = []
        for i in range(n):
            curr = data[str(i)]["benchmark"]["scc"]
            for p in path:
                if p in curr:
                    curr = curr[p]
                else:
                    return None
            durs.append(int(curr["duration"]))
        return max(durs) * 1e-9

    def get_decided(path):
        curr = data["0"]["benchmark"]["scc"]
        for p in path:
            if p in curr:
                curr = curr[p]
            else:
                return None
        if "decided_count" in curr:
            return int(curr["decided_count"])
        return None

    res = []
    for stage_path in [["trim_1_first"], ["forward_backward_search"], ["trim_1_normal"],
                       ["residual", "scc", "residual_scc"]]:
        dur = get_max_dur(stage_path)
        decided = get_decided(stage_path)
        if dur is not None and decided is not None:
            res.append({"name": stage_path[-1], "duration": dur, "decided_count": decided})
    return res


def get_hpc_graph_progress(data):
    n = len(data)

    def get_max_dur(path):
        durs = []
        for i in range(n):
            curr = data[str(i)]["benchmark"]
            for p in path:
                if p in curr:
                    curr = curr[p]
                else:
                    return None
            durs.append(int(curr["duration"]))
        return max(durs) * 1e-9

    def get_decided(path):
        curr = data["0"]["benchmark"]
        for p in path:
            if p in curr:
                curr = curr[p]
            else:
                return None
        if "decided_count" in curr:
            return int(curr["decided_count"])
        return None

    res = []
    for stage_path in [["scc", "forward_backward_search"], ["scc", "color"]]:
        dur = get_max_dur(stage_path)
        decided = get_decided(stage_path)
        if dur is not None and decided is not None:
            res.append({"name": stage_path[-1], "duration": dur, "decided_count": decided})
    return res


def get_kaspan_memory(data):
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)]["benchmark"]
        base = int(node["memory"])
        scc = node["scc"]
        m = base
        if "forward_backward_search" in scc and "memory" in scc["forward_backward_search"]:
            m = max(m, int(scc["forward_backward_search"]["memory"]))
        if "ecl" in scc and "memory" in scc["ecl"]:
            m = max(m, int(scc["ecl"]["memory"]))
        if "residual" in scc and "memory" in scc["residual"]:
            m = max(m, int(scc["residual"]["memory"]))
        mem_sum += m - base
    return mem_sum / n


def get_ispan_memory(data):
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)]["benchmark"]
        base = int(node["memory"])
        scc = node["scc"]
        m = base
        if "alloc" in scc and "memory" in scc["alloc"]:
            m = max(m, int(scc["alloc"]["memory"]))
        if "residual" in scc and "post_processing" in scc["residual"] and "memory" in scc["residual"][
            "post_processing"]:
            m = max(m, int(scc["residual"]["post_processing"]["memory"]))
        mem_sum += m - base
    return mem_sum / n


def get_hpc_graph_memory(data):
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)]["benchmark"]
        base = int(node["memory"])
        m = base
        if "memory_after_adapter" in node:
            m = max(m, int(node["memory_after_adapter"]))
        if "scc" in node and "scc" in node["scc"] and "memory" in node["scc"]["scc"]:
            m = max(m, int(node["scc"]["scc"]["memory"]))
        if "memory_after_scc" in node:
            m = max(m, int(node["memory_after_scc"]))
        mem_sum += m - base
    return mem_sum / n


def get_time_info(max_s):
    if max_s >= 10: return 1, "s"
    if max_s >= 1e-3: return 1e3, "ms"
    if max_s >= 1e-6: return 1e6, "us"
    return 1e9, "ns"


def get_memory_info(max_b):
    if max_b >= 1e9: return 1e-9, "GB"
    if max_b >= 1e6: return 1e-6, "MB"
    if max_b >= 1e3: return 1e-3, "KB"
    return 1, "B"


def setup_ax(ax, title, ylabel, nps, log_y=True):
    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xlabel("Processes (np)")
    ax.set_xscale("log", base=2)
    if log_y: ax.set_yscale("log")
    ax.set_xticks(nps)
    ax.set_xticklabels([str(n) for n in nps], rotation=45)
    ax.grid(True, which="both", linestyle="--", alpha=0.5)
    if nps:
        ax.set_xlim(min(nps), max(nps))
        mark_topology(ax, min(nps), max(nps))


def mark_topology(ax, min_n, max_n):
    borders = [(1.5, "single threaded", "multithreaded"),
               (CORES_PER_SOCKET + 0.5, "single socket", "multi socket"),
               (CORES_PER_NODE + 0.5, "single node", "multi node")]
    for x, left_lab, right_lab in borders:
        if min_n < x < max_n:
            ax.axvline(x, color="black", linestyle="--", alpha=0.5)
            ax.text(x, 0.5, left_lab, rotation=90, verticalalignment="center", horizontalalignment="right", fontsize=8,
                    transform=ax.get_xaxis_transform())
            ax.text(x, 0.5, right_lab, rotation=90, verticalalignment="center", horizontalalignment="left", fontsize=8,
                    transform=ax.get_xaxis_transform())


cwd = pathlib.Path(os.getcwd())
pattern = re.compile(r"(weak|strong)_(kaspan|ispan|hpc_graph)_np([0-9]+)_(.*)\.json")
files = [p for p in cwd.rglob("*.json") if p.is_file()]
raw_data = {}
graphs = set()
apps = set()
for p in files:
    m = pattern.fullmatch(p.name)
    if not m: continue
    scaling, app, np_val, graph = m.groups()
    if scaling != "weak": continue
    np_val = int(np_val)
    try:
        with p.open("r") as f:
            j = json.load(f)
            duration = globals()[f"get_{app}_duration"](j)
            memory = globals()[f"get_{app}_memory"](j)
            progress = globals()[f"get_{app}_progress"](j)
            if duration <= TIME_LIMIT_S:
                print(f"Loading {p.name}: duration={duration:.2f}s, memory={memory / 1e6:.2f}MB")
                raw_data.setdefault(graph, {}).setdefault(app, {})[np_val] = (duration, memory, progress)
                graphs.add(graph)
                apps.add(app)
            else:
                print(f"Skipping {p.name}: duration {duration:.2f}s > {TIME_LIMIT_S}s")
    except Exception as e:
        print(f"Error loading {p.name}: {e}")
        continue
markers = ["o", "s", "D", "^", "v", "P", "X", "d"]
all_stages = set()
for graph in raw_data:
    for app in raw_data[graph]:
        for np_val in raw_data[graph][app]:
            for stage in raw_data[graph][app][np_val][2]:
                all_stages.add(stage["name"])
all_stages = sorted(list(all_stages))
stage_style = {stage: (plt.get_cmap("tab20")(i % 20), markers[i % len(markers)]) for i, stage in enumerate(all_stages)}
apps = sorted(list(apps))
colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
app_style = {app: (colors[i % len(colors)], markers[i % len(markers)]) for i, app in enumerate(apps)}
for graph in graphs:
    print(f"Plotting graph: {graph}")
    graph_data = raw_data[graph]
    all_nps = sorted(list(set(n for a in graph_data for n in graph_data[a])))
    # plot progress per app
    for app in apps:
        if app not in graph_data: continue
        fig, ax = plt.subplots(figsize=(8, 6), layout="constrained")
        app_nps = sorted(list(graph_data[app].keys()))
        setup_ax(ax, f"Weak Scaling Progress: {graph} ({app})", "Decisions per second", app_nps, log_y=True)
        # Find all stages for this app
        stages = set()
        for np_val in graph_data[app]:
            for stage in graph_data[app][np_val][2]:
                stages.add(stage["name"])
        stages = sorted(list(stages))
        for stage_name in stages:
            x_vals = []
            y_vals = []
            for np_val in app_nps:
                progress = graph_data[app][np_val][2]
                stage_data = next((s for s in progress if s["name"] == stage_name), None)
                if stage_data and stage_data["duration"] > 0 and stage_data["decided_count"] > 0:
                    x_vals.append(np_val)
                    y_vals.append(stage_data["decided_count"] / stage_data["duration"])
            if x_vals:
                c, m = stage_style[stage_name]
                ax.plot(x_vals, y_vals, label=stage_name, color=c, marker=m)
        ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.15), ncol=len(stages))
        out_path = cwd / f"{graph}_{app}_weak_progress.png"
        print(f"Saving plot to {out_path}")
        fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
        plt.close(fig)
    max_dur = 0
    max_mem = 0
    for app in apps:
        if app not in graph_data: continue
        durs = [graph_data[app].get(n, (None, None))[0] for n in all_nps]
        mems = [graph_data[app].get(n, (None, None))[1] for n in all_nps]
        valid_durs = [d for d in durs if d is not None]
        valid_mems = [m for m in mems if m is not None]
        if valid_durs: max_dur = max(max_dur, max(valid_durs))
        if valid_mems: max_mem = max(max_mem, max(valid_mems))
    t_scale, t_unit = get_time_info(max_dur)
    m_scale, m_unit = get_memory_info(max_mem)
    for plot_type in ["duration", "memory"]:
        fig, ax = plt.subplots(figsize=(8, 6), layout="constrained")
        if plot_type == "duration":
            setup_ax(ax, f"Weak Scaling Duration: {graph}", f"Time [{t_unit}]", all_nps)
        else:
            setup_ax(ax, f"Weak Scaling Memory: {graph}", f"Memory [{m_unit}/np]", all_nps)
        for app in apps:
            if app not in graph_data: continue
            c, m = app_style[app]
            durs = [graph_data[app].get(n, (None, None))[0] for n in all_nps]
            mems = [graph_data[app].get(n, (None, None))[1] for n in all_nps]
            x_vals = []
            y_vals = []
            if plot_type == "duration":
                for n, d in zip(all_nps, durs):
                    if d is not None and d > 0:
                        x_vals.append(n)
                        y_vals.append(d * t_scale)
            else:
                for n, m_val in zip(all_nps, mems):
                    if m_val is not None and m_val > 0:
                        x_vals.append(n)
                        y_vals.append(m_val * m_scale)
            if x_vals:
                ax.plot(x_vals, y_vals, label=app, color=c, marker=m)
        ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.15), ncol=len(apps))
        out_path = cwd / f"{graph}_weak_{plot_type}.png"
        print(f"Saving plot to {out_path}")
        fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
        plt.close(fig)
