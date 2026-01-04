import matplotlib.pyplot as plt
import numpy as np
import re
import json
import pathlib

CORES_PER_SOCKET = 38
CORES_PER_NODE = 76
TIME_LIMIT_S = 600
DPI = 200

MARKERS = ["o", "s", "D", "^", "v", "P", "X", "d"]


def load_all_data(cwd):
    """
    Scans the current directory for JSON result files and extracts all metrics.
    Returns:
    - scaling_data: dict { 'weak': { graph: { app: { np: (dur, mem, progress) } } }, 'strong': ... }
    - all_metrics_raw: dict { graph: { app: { np: (dur, mem, progress) } } } (combined)
    - apps: set of all app names found
    - all_stages: set of all algorithmic step names found
    - global_all_nps: set of all process counts found
    """
    pattern = re.compile(r"(weak|strong)_(kaspan|ispan|hpc_graph)_np([0-9]+)_(.*)\.json")
    files = [p for p in cwd.rglob("*.json") if p.is_file()]

    all_metrics_raw = {}
    apps = set()
    all_stages = set()
    global_all_nps = set()
    scaling_data = {"weak": {}, "strong": {}}

    for p in files:
        m = pattern.fullmatch(p.name)
        if not m: continue
        scaling, app, np_val, graph = m.groups()
        np_val = int(np_val)
        try:
            with p.open("r") as f:
                j = json.load(f)

                # Dynamic dispatch to app-specific extraction functions
                dur_func = globals().get(f"get_{app}_duration")
                mem_func = globals().get(f"get_{app}_memory")
                prog_func = globals().get(f"get_{app}_progress")

                if not (dur_func and mem_func and prog_func):
                    print(f"Unknown app: {app}")
                    continue

                duration = dur_func(j)
                memory = mem_func(j, np_val)
                progress = prog_func(j)

                if duration <= TIME_LIMIT_S:
                    entry = (duration, memory, progress)
                    all_metrics_raw.setdefault(graph, {}).setdefault(app, {})[np_val] = entry
                    scaling_data[scaling].setdefault(graph, {}).setdefault(app, {})[np_val] = entry
                    apps.add(app)
                    global_all_nps.add(np_val)
                    for stage in progress:
                        all_stages.add(stage["name"])
                else:
                    print(f"Skipping {p.name}: duration {duration:.2f}s > {TIME_LIMIT_S}s")
        except Exception as e:
            print(f"Error loading {p.name}: {e}")
            continue
    return scaling_data, all_metrics_raw, apps, all_stages, global_all_nps


def calculate_global_stats(all_metrics_raw):
    """
    Calculates global min/max for all metrics across all loaded data.
    Ensures Y-axis consistency.
    """
    all_metrics = {
        "duration": [],
        "memory": [],
        "step_throughput": [],
        "speedup": [],
        "step_duration": []
    }

    for graph in all_metrics_raw:
        graph_data = all_metrics_raw[graph]
        all_nps = sorted(list(set(n for a in graph_data for n in graph_data[a])))
        if not all_nps: continue
        min_np = min(all_nps)
        candidates = [graph_data[app][min_np][0] for app in graph_data if min_np in graph_data[app]]
        baseline_dur = min(candidates) if candidates else None

        for app in graph_data:
            for np_val in graph_data[app]:
                dur, mem, progress = graph_data[app][np_val]
                all_metrics["duration"].append(dur)
                all_metrics["memory"].append(mem)
                if baseline_dur:
                    all_metrics["speedup"].append(baseline_dur / (dur if dur > 0 else 1e-9))

                sum_step_dur = 0
                for s in progress:
                    if s["duration"] > 0:
                        all_metrics["step_throughput"].append(s["decided_count"] / s["duration"])
                        all_metrics["step_duration"].append(s["duration"])
                        sum_step_dur += s["duration"]
                all_metrics["step_duration"].append(max(0, dur - sum_step_dur))

    def get_max_safe(v, default=1.0):
        if not v: return default
        m = max(v)
        return m if not np.isnan(m) and m > 0 else default

    def get_min_safe(v, default=0.1):
        if not v: return default
        m = min(v)
        return m if not np.isnan(m) and m > 0 else default

    g_max = {k: get_max_safe(v) for k, v in all_metrics.items()}
    g_min = {k: get_min_safe(v) for k, v in all_metrics.items()}

    # Adjustments for better plotting as per plot.txt
    g_max["speedup"] = max(1.1, g_max["speedup"])
    g_min["speedup"] = 0
    g_min["memory"] = 0
    g_min["duration"] = 1e-9  # Fixed to 1ns
    g_min["step_duration"] = 1e-9  # Fixed to 1ns for consistency
    g_min["step_throughput"] = 1.0  # Ensure positive for log

    return g_min, g_max


def get_kaspan_duration(data):
    """Retrieves the maximum 'scc' duration across all ranks for KaSpan (in seconds)."""
    n = len(data)
    dur = max(int(data[str(i)].get("benchmark", {}).get("scc", {}).get("duration", 0)) for i in range(n)) * 1e-9
    assert dur > 0, f"Non-positive duration found: {dur}"
    return dur


def get_ispan_duration(data):
    """Retrieves the maximum 'scc' duration across all ranks for iSpan (in seconds)."""
    n = len(data)
    dur = max(int(data[str(i)].get("benchmark", {}).get("scc", {}).get("duration", 0)) for i in range(n)) * 1e-9
    assert dur > 0, f"Non-positive duration found: {dur}"
    return dur


def get_hpc_graph_duration(data):
    """Retrieves the maximum 'scc' duration across all ranks for hpc_graph (in seconds)."""
    n = len(data)
    dur = max(int(data[str(i)].get("benchmark", {}).get("scc", {}).get("duration", 0)) for i in range(n)) * 1e-9
    assert dur > 0, f"Non-positive duration found: {dur}"
    return dur


def get_all_step_data(data, app):
    """
    Recursively searches for all nodes under 'benchmark/scc' that contain a 'decided_count'.
    Returns a list of dictionaries, each containing:
    - 'name': The path to the node (joined by underscores).
    - 'duration': The maximum duration for this node across all ranks (in seconds).
    - 'decided_count': The decided count (from rank 0, assuming it's allreduced).
    """
    n = len(data)
    paths = []

    def find_paths(node, current_path):
        if not isinstance(node, dict):
            return
        if "decided_count" in node:
            paths.append(current_path)
        for k, v in node.items():
            if isinstance(v, dict):
                find_paths(v, current_path + [k])

    scc_root_rank0 = data["0"].get("benchmark", {}).get("scc", {})
    find_paths(scc_root_rank0, [])

    results = []
    for path in paths:
        durs = []
        decided_val = 0
        for i in range(n):
            curr = data[str(i)].get("benchmark", {}).get("scc", {})
            for p in path:
                if isinstance(curr, dict) and p in curr:
                    curr = curr[p]
                else:
                    curr = None
                    break
            if isinstance(curr, dict):
                if "duration" in curr:
                    durs.append(int(curr["duration"]))
                if i == 0 and "decided_count" in curr:
                    decided_val = int(curr["decided_count"])

        if durs:
            dur = max(durs) * 1e-9
            assert dur > 0, f"Non-positive duration found for step {path}: {dur}"
            name = "_".join(path) if path else "scc"
            results.append({
                "name": name,
                "duration": dur,
                "decided_count": decided_val
            })

    # Handle the special case for KaSpan's tarjan fallback if it doesn't have decided_count explicitly
    if app == "kaspan" and not any(r["name"] == "tarjan" for r in results):
        durs = []
        for i in range(n):
            node = data[str(i)].get("benchmark", {}).get("scc", {}).get("tarjan", {})
            if isinstance(node, dict) and "duration" in node:
                durs.append(int(node["duration"]))
        if durs:
            dur = max(durs) * 1e-9
            assert dur > 0, f"Non-positive duration found for tarjan: {dur}"
            results.append({
                "name": "tarjan",
                "duration": dur,
                "decided_count": int(data["0"].get("benchmark", {}).get("n", 0))
            })

    return results


def get_kaspan_progress(data):
    """Retrieves progress data for KaSpan using recursive search."""
    return get_all_step_data(data, "kaspan")


def get_ispan_progress(data):
    """Retrieves progress data for iSpan using recursive search."""
    return get_all_step_data(data, "ispan")


def get_hpc_graph_progress(data):
    """Retrieves progress data for hpc_graph using recursive search."""
    return get_all_step_data(data, "hpc_graph")


def get_kaspan_memory(data, np_val):
    """
    Extracts peak memory usage (increase over base) for KaSpan across all ranks.
    Retrieves the maximum 'memory' (resident set size) reported in 'benchmark'
    and nested 'scc' scopes (forward_backward_search, ecl, residual).
    Returns average memory increase per core (total increase / np_val).
    """
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)].get("benchmark", {})
        base = int(node.get("memory", 0))
        scc = node.get("scc", {})
        m = base
        if "forward_backward_search" in scc and "memory" in scc["forward_backward_search"]:
            m = max(m, int(scc["forward_backward_search"]["memory"]))
        if "ecl" in scc and "memory" in scc["ecl"]:
            m = max(m, int(scc["ecl"]["memory"]))
        if "residual" in scc and "memory" in scc["residual"]:
            m = max(m, int(scc["residual"]["memory"]))

        diff = m - base
        assert diff >= 0, f"Negative memory increase found on rank {i}: {m} - {base} = {diff}"
        mem_sum += diff
    return mem_sum / np_val


def get_ispan_memory(data, np_val):
    """
    Extracts peak memory usage (increase over base) for iSpan across all ranks.
    Retrieves the maximum 'memory' reported in 'benchmark' and 'scc' (alloc, residual/post_processing).
    Returns average memory increase per core (total increase / np_val).
    """
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)].get("benchmark", {})
        base = int(node.get("memory", 0))
        scc = node.get("scc", {})
        m = base
        if "alloc" in scc and "memory" in scc["alloc"]:
            m = max(m, int(scc["alloc"]["memory"]))
        if "residual" in scc and "post_processing" in scc["residual"] and "memory" in scc["residual"][
            "post_processing"]:
            m = max(m, int(scc["residual"]["post_processing"]["memory"]))

        diff = m - base
        assert diff >= 0, f"Negative memory increase found on rank {i}: {m} - {base} = {diff}"
        mem_sum += diff
    return mem_sum / np_val


def get_hpc_graph_memory(data, np_val):
    """
    Extracts peak memory usage (increase over base) for hpc_graph across all ranks.
    Retrieves 'memory' from benchmark, memory_after_adapter, scc, and memory_after_scc.
    Returns average memory increase per core (total increase / np_val).
    """
    n = len(data)
    mem_sum = 0
    for i in range(n):
        node = data[str(i)].get("benchmark", {})
        base = int(node.get("memory", 0))
        m = base
        if "memory_after_adapter" in node:
            m = max(m, int(node["memory_after_adapter"]))
        if "scc" in node and "scc" in node["scc"] and "memory" in node["scc"]["scc"]:
            m = max(m, int(node["scc"]["scc"]["memory"]))
        if "memory_after_scc" in node:
            m = max(m, int(node["memory_after_scc"]))

        diff = m - base
        assert diff >= 0, f"Negative memory increase found on rank {i}: {m} - {base} = {diff}"
        mem_sum += diff
    return mem_sum / np_val


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


def setup_ax(ax, title, ylabel, nps, log_y=True, x_min=None, x_max=None):
    """
    Sets up the axes with common parameters.
    - title: Plot title.
    - ylabel: Y-axis label.
    - nps: List of core counts for ticks.
    - log_y: Whether to use logarithmic scale for Y-axis.
    - x_min, x_max: Optional limits for X-axis (in core units).
    """
    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xlabel("Cores")
    ax.set_xscale("log", base=2)
    if log_y: ax.set_yscale("log")
    ax.set_xticks(nps)
    ax.set_xticklabels([str(n) for n in nps], rotation=45)
    ax.grid(True, which="both", linestyle="--", alpha=0.5)
    if nps:
        xmin = x_min if x_min is not None else 1
        xmax = x_max if x_max is not None else max(nps) * 1.1
        ax.set_xlim(xmin, xmax)
        mark_topology(ax, xmin, xmax)


def get_plot_config(plot_type, scaling_type, graph, global_min, global_max, app=None):
    """
    Provides a consistent configuration (title, labels, scale, limits) for all plot types.
    """
    t_scale, t_unit = get_time_info(global_max["duration"])
    m_scale, m_unit = get_memory_info(global_max["memory"])

    # scaling_type should be e.g., "Strong Scaling" or "Weak Scaling"
    prefix = f"{scaling_type} " if scaling_type else ""
    app_suffix = f" ({app})" if app else ""

    if plot_type == "duration":
        return {
            "title": f"{prefix}Duration: {graph}{app_suffix}",
            "ylabel": f"Time [{t_unit}]",
            "log_y": True,
            "ylim": (global_min["duration"] * t_scale * 0.9, global_max["duration"] * t_scale * 1.1),
            "scale": t_scale
        }
    elif plot_type == "speedup":
        label = "Speedup" if "Strong" in scaling_type else "Efficiency"
        return {
            "title": f"{prefix}{label}: {graph}{app_suffix}",
            "ylabel": label,
            "log_y": False,
            "ylim": (global_min["speedup"] * 0.9, global_max["speedup"] * 1.1),
            "scale": 1.0
        }
    elif plot_type == "memory":
        return {
            "title": f"{prefix}Memory: {graph}{app_suffix}",
            "ylabel": f"Memory [{m_unit}/core]",
            "log_y": False,
            "ylim": (global_min["memory"] * m_scale * 0.9, global_max["memory"] * m_scale * 1.1),
            "scale": m_scale
        }
    elif plot_type == "step_throughput":
        return {
            "title": f"{prefix}Step Throughput: {graph}{app_suffix}",
            "ylabel": "Decisions per second",
            "log_y": True,
            "ylim": (global_min["step_throughput"] * 0.9, global_max["step_throughput"] * 1.1),
            "scale": 1.0
        }
    elif plot_type == "step_duration":
        return {
            "title": f"{prefix}Step Duration: {graph}{app_suffix}",
            "ylabel": f"Time [{t_unit}]",
            "log_y": True,  # Use log for consistency with total duration
            "ylim": (global_min["step_duration"] * t_scale * 0.9, global_max["step_duration"] * t_scale * 1.1),
            "scale": t_scale
        }
    return None


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


def plot_graph_summary(graph, graph_data, apps, global_min, global_max, app_style, scaling_type, out_dir):
    """Internal helper to plot Duration, Speedup/Efficiency, and Memory for all variants."""
    all_nps = sorted(list(set(n for a in graph_data for n in graph_data[a])))
    if not all_nps: return
    min_np = min(all_nps)
    max_np = max(all_nps)
    candidates = [graph_data[app][min_np][0] for app in graph_data if min_np in graph_data[app]]
    baseline_dur = min(candidates) if candidates else None

    scaling_prefix = "strong" if "Strong" in scaling_type else "weak"

    for plot_type in ["duration", "speedup", "memory"]:
        fig, ax = plt.subplots(figsize=(8, 6), layout="constrained")

        cfg = get_plot_config(plot_type, scaling_type, graph, global_min, global_max)
        setup_ax(ax, cfg["title"], cfg["ylabel"], all_nps, log_y=cfg["log_y"], x_min=min_np * 0.9, x_max=max_np * 1.1)
        ax.set_ylim(*cfg["ylim"])
        scale = cfg["scale"]

        if plot_type == "speedup":
            if "Strong" in scaling_type:
                # Ideal strong scaling: speedup = np / min_np
                ax.plot([min_np, max_np], [1.0, max_np / min_np], color="gray", linestyle="--", alpha=0.5,
                        label="ideal")
            else:
                # Ideal weak scaling: efficiency = 1.0
                ax.axhline(1.0, color="gray", linestyle="--", alpha=0.5, label="ideal")

        for app in sorted(list(apps)):
            if app not in graph_data: continue
            app_nps = sorted(list(graph_data[app].keys()))
            c, m = app_style[app]
            x_vals, y_vals = [], []
            for n in app_nps:
                dur, mem, _ = graph_data[app][n]
                if plot_type == "duration":
                    x_vals.append(n);
                    y_vals.append(dur * scale)
                elif plot_type == "speedup":
                    if baseline_dur: x_vals.append(n); y_vals.append(baseline_dur / (dur if dur > 0 else 1e-9))
                elif plot_type == "memory":
                    x_vals.append(n);
                    y_vals.append(mem * scale)
            if x_vals:
                ax.plot(x_vals, y_vals, label=app, color=c, marker=m)

        ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.15), ncol=min(4, len(apps)))
        out_path = out_dir / f"{scaling_prefix}_{graph}_{plot_type}.png"
        print(f"Saving plot to {out_path}")
        fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
        plt.close(fig)


def plot_graph_steps(graph, graph_data, apps, global_min, global_max, stage_style, scaling_type, out_dir):
    """Internal helper to plot side-by-side Step Throughput and Duration for each variant."""
    all_nps = sorted(list(set(n for a in graph_data for n in graph_data[a])))
    if not all_nps: return
    min_np = min(all_nps)
    max_np = max(all_nps)
    scaling_prefix = "strong" if "Strong" in scaling_type else "weak"

    for app in sorted(list(apps)):
        if app not in graph_data: continue
        app_nps = sorted(list(graph_data[app].keys()))

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6), layout="constrained")

        cfg1 = get_plot_config("step_throughput", scaling_type, graph, global_min, global_max, app=app)
        setup_ax(ax1, cfg1["title"], cfg1["ylabel"], all_nps, log_y=cfg1["log_y"], x_min=min_np * 0.9, x_max=max_np * 1.1)
        ax1.set_ylim(*cfg1["ylim"])

        cfg2 = get_plot_config("step_duration", scaling_type, graph, global_min, global_max, app=app)
        setup_ax(ax2, cfg2["title"], cfg2["ylabel"], all_nps, log_y=cfg2["log_y"], x_min=min_np * 0.9, x_max=max_np * 1.1)
        ax2.set_ylim(*cfg2["ylim"])
        scale2 = cfg2["scale"]

        stages = sorted(list(set(s["name"] for n in app_nps for s in graph_data[app][n][2])))
        for stage_name in stages:
            c, m = stage_style[stage_name]
            # Throughput
            x_vals, y_vals = [], []
            for n in app_nps:
                progress = graph_data[app][n][2]
                stage_data = next((s for s in progress if s["name"] == stage_name), None)
                if stage_data and stage_data["duration"] > 0:
                    x_vals.append(n);
                    y_vals.append(stage_data["decided_count"] / stage_data["duration"])
            if x_vals:
                ax1.plot(x_vals, y_vals, label=stage_name, color=c, marker=m)

            # Duration
            x_vals, y_vals = [], []
            for n in app_nps:
                progress = graph_data[app][n][2]
                stage_data = next((s for s in progress if s["name"] == stage_name), None)
                if stage_data:
                    x_vals.append(n);
                    y_vals.append(stage_data["duration"] * scale2)
            if x_vals:
                ax2.plot(x_vals, y_vals, label=stage_name, color=c, marker=m)

        # Add "Other" to Duration plot
        x_vals, y_vals = [], []
        for n in app_nps:
            dur, _, progress = graph_data[app][n]
            val = dur - sum(s["duration"] for s in progress)
            x_vals.append(n);
            y_vals.append(max(0, val) * scale2)
        if x_vals:
            c, m = stage_style["Other"]
            ax2.plot(x_vals, y_vals, label="Other", color=c, marker=m)

        # Unified legend for the whole figure
        handles, labels = ax1.get_legend_handles_labels()
        h2, l2 = ax2.get_legend_handles_labels()
        for h, l in zip(h2, l2):
            if l not in labels:
                handles.append(h);
                labels.append(l)
        fig.legend(handles, labels, loc="lower center", bbox_to_anchor=(0.5, -0.1), ncol=min(6, len(labels)))

        out_path = out_dir / f"{scaling_prefix}_{graph}_steps_{app}.png"
        print(f"Saving plot to {out_path}")
        fig.savefig(out_path, dpi=DPI, bbox_inches="tight")
        plt.close(fig)


def plot_strong_summary(graph, graph_data, apps, global_min, global_max, app_style, out_dir):
    """Plots Duration, Speedup, and Memory for Strong Scaling."""
    plot_graph_summary(graph, graph_data, apps, global_min, global_max, app_style, "Strong Scaling", out_dir)


def plot_weak_summary(graph, graph_data, apps, global_min, global_max, app_style, out_dir):
    """Plots Duration, Efficiency, and Memory for Weak Scaling."""
    plot_graph_summary(graph, graph_data, apps, global_min, global_max, app_style, "Weak Scaling", out_dir)


def plot_strong_steps(graph, graph_data, apps, global_min, global_max, stage_style, out_dir):
    """Plots side-by-side Step Throughput and Duration for Strong Scaling."""
    plot_graph_steps(graph, graph_data, apps, global_min, global_max, stage_style, "Strong Scaling", out_dir)


def plot_weak_steps(graph, graph_data, apps, global_min, global_max, stage_style, out_dir):
    """Plots side-by-side Step Throughput and Duration for Weak Scaling."""
    plot_graph_steps(graph, graph_data, apps, global_min, global_max, stage_style, "Weak Scaling", out_dir)


def get_styles(apps, stages):
    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    app_style = {app: (colors[i % len(colors)], MARKERS[i % len(MARKERS)]) for i, app in enumerate(sorted(list(apps)))}

    stage_list = sorted(list(stages))
    stage_style = {stage: (plt.get_cmap("tab20")(i % 20), MARKERS[i % len(MARKERS)]) for i, stage in
                   enumerate(stage_list)}

    return app_style, stage_style
