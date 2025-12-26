import itertools
import json
import os
import pathlib
import re
import matplotlib.pyplot as plt
import numpy as np

CORES_PER_SOCKET = 38
CORES_PER_NODE = 76
TIME_LIMIT_S = 600
PLOT_SPEEDUP = False
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

def get_kaspan_memory(data):
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)]["benchmark"]
        base = int(node["memory"])
        scc = node["scc"]
        if n == 1:
            mem_sum += int(scc["tarjan"]["memory"]) - base
        elif "residual" in scc:
            mem_sum += int(scc["residual"]["memory"]) - base
        else:
            mem_sum += int(scc["alloc"]["memory"]) - base
    return mem_sum / n

def get_ispan_memory(data):
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)]["benchmark"]
        base = int(node["memory"])
        mem_sum += int(node["scc"]["alloc"]["memory"]) - base
    return mem_sum / n

def get_hpc_graph_memory(data):
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)]["benchmark"]
        base = int(node["memory"])
        scc = node["scc"]
        if n == 1:
            mem_sum += int(scc["tarjan"]["memory"]) - base
        elif "residual" in scc:
            mem_sum += int(scc["residual"]["memory"]) - base
        else:
            mem_sum += int(scc["alloc"]["memory"]) - base
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
    if nps: mark_topology(ax, max(nps))

def mark_topology(ax, max_n):
    borders = [(1.5, "single threaded", "multithreaded"),
               (CORES_PER_SOCKET + 0.5, "single socket", "multi socket"),
               (CORES_PER_NODE + 0.5, "single node", "multi node")]
    for x, left_lab, right_lab in borders:
        if x < max_n:
            ax.axvline(x, color="black", linestyle="--", alpha=0.5)
            y_min, y_max = ax.get_ylim()
            y_pos = (y_min * y_max)**0.5 if ax.get_yscale() == "log" else (y_min + y_max) / 2
            ax.text(x, y_pos, left_lab, rotation=90, verticalalignment="center", horizontalalignment="right", fontsize=8)
            ax.text(x, y_pos, right_lab, rotation=90, verticalalignment="center", horizontalalignment="left", fontsize=8)

cwd = pathlib.Path(os.getcwd())
pattern = re.compile(r"(kaspan|ispan|hpc_graph)_(.*)_np([0-9]+)\.json")
files = [p for p in cwd.iterdir() if p.is_file()]

raw_data = {}
graphs = set()
apps = set()

for p in files:
    m = pattern.fullmatch(p.name)
    if not m: continue
    app, graph, np_val = m.groups()
    np_val = int(np_val)
    try:
        with p.open("r") as f:
            j = json.load(f)
            duration = globals()[f"get_{app}_duration"](j)
            memory = globals()[f"get_{app}_memory"](j)
            if duration <= TIME_LIMIT_S:
                print(f"Loading {p.name}: duration={duration:.2f}s, memory={memory/1e6:.2f}MB")
                raw_data.setdefault(graph, {}).setdefault(app, {})[np_val] = (duration, memory)
                graphs.add(graph)
                apps.add(app)
            else:
                print(f"Skipping {p.name}: duration {duration:.2f}s > {TIME_LIMIT_S}s")
    except Exception as e:
        print(f"Error loading {p.name}: {e}")
        continue

apps = sorted(list(apps))
colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
markers = ["o", "s", "D", "^", "v", "P", "X", "d"]
app_style = {app: (colors[i % len(colors)], markers[i % len(markers)]) for i, app in enumerate(apps)}

for graph in graphs:
    print(f"Plotting graph: {graph}")
    graph_data = raw_data[graph]
    all_nps = sorted(list(set(n for a in graph_data for n in graph_data[a])))
    
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
    
    min_np = min(all_nps)
    candidates = [graph_data[app][min_np][0] for app in graph_data if min_np in graph_data[app]]
    baseline_dur = min(candidates) if candidates else None

    for plot_type in ["duration", "speedup", "memory"]:
        fig, ax = plt.subplots(figsize=(8, 6))
        if plot_type == "duration":
            setup_ax(ax, f"Duration: {graph}", f"Time [{t_unit}]", all_nps)
        elif plot_type == "speedup":
            setup_ax(ax, f"Speedup: {graph}", "Speedup", all_nps, log_y=False)
        else:
            setup_ax(ax, f"Memory: {graph}", f"Memory [{m_unit}/np]", all_nps)

        for app in apps:
            if app not in graph_data: continue
            c, m = app_style[app]
            durs = [graph_data[app].get(n, (None, None))[0] for n in all_nps]
            mems = [graph_data[app].get(n, (None, None))[1] for n in all_nps]
            
            if plot_type == "duration":
                y = [d * t_scale if d is not None else np.nan for d in durs]
            elif plot_type == "speedup":
                y = [baseline_dur / d if (d is not None and baseline_dur) else np.nan for d in durs]
            else:
                y = [m * m_scale if m is not None else np.nan for m in mems]
            ax.plot(all_nps, y, label=app, color=c, marker=m)

        ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.15), ncol=len(apps))
        fig.tight_layout()
        out_path = cwd / f"{graph}_{plot_type}.png"
        print(f"Saving plot to {out_path}")
        fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
        plt.close(fig)
