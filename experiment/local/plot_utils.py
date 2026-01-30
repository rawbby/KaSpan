import json
import re

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

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
    pattern = re.compile(r"(weak|strong)_(kaspan|ispan|hpc_graph)(?:_([a-zA-Z0-9_]+))?_np([0-9]+)_(.*)\.json")
    files = [p for p in cwd.rglob("*.json") if p.is_file()]

    all_metrics_raw = {}
    apps = set()
    all_stages = set()
    global_all_nps = set()
    scaling_data = {"weak": {}, "strong": {}}

    for p in files:
        m = pattern.fullmatch(p.name)
        if not m: continue

        scaling, app, variant, np_val, graph = m.groups()
        np_val = int(np_val)
        app_variant = f"{app}_{variant}" if variant else app

        try:
            with p.open("r") as f:
                j = json.load(f)

                duration = globals().get(f"get_{app}_duration")(j, np_val)
                memory = globals().get(f"get_{app}_memory")(j, np_val)
                progress = globals().get(f"get_{app}_progress")(j, np_val)

                entry = (duration, memory, progress)
                all_metrics_raw.setdefault(graph, {}).setdefault(app_variant, {})[np_val] = entry
                scaling_data[scaling].setdefault(graph, {}).setdefault(app_variant, {})[np_val] = entry

                apps.add(app_variant)
                global_all_nps.add(np_val)

                for stage in progress:
                    all_stages.add(stage["name"])

        except Exception as e:
            print(f"Error loading {p.name}: {e}")
            continue

    return scaling_data, all_metrics_raw, apps, all_stages, global_all_nps


def calculate_global_stats(all_metrics_raw):
    """
    Calculates global min/max for all metrics across all loaded data.
    Ensures Y-axis (Cores) consistency.
    """
    all_metrics = {
        "duration": [],
        "memory": [],
        "step_throughput": [],
        "speedup": [],
        "step_duration": []
    }
    all_nps_global = []

    for graph in all_metrics_raw:
        graph_data = all_metrics_raw[graph]
        all_nps = sorted(list(set(n for a in graph_data for n in graph_data[a])))
        if not all_nps: continue
        all_nps_global.extend(all_nps)

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
                        if s["decided_count"] > 0:
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

    g_min["cores"] = min(all_nps_global) if all_nps_global else 1
    g_max["cores"] = max(all_nps_global) if all_nps_global else 10

    return g_min, g_max


def get_max_recursive(node, key_name):
    """Recursively searches for the maximum value of a specific key in a nested dictionary."""
    max_val = -1
    if not isinstance(node, dict):
        return max_val

    if key_name in node:
        try:
            max_val = int(node[key_name])
        except (ValueError, TypeError):
            pass

    for v in node.values():
        if isinstance(v, dict):
            max_val = max(max_val, get_max_recursive(v, key_name))
    return max_val


def get_kaspan_duration(data, np_val):
    """Retrieves the maximum 'scc' duration across all ranks for KaSpan (in seconds)."""
    n = np_val
    dur = max(get_max_recursive(data[str(i)].get("benchmark", {}).get("scc", {}), "duration") for i in range(n)) * 1e-9
    assert dur > 0, f"Non-positive duration found: {dur}"
    return dur


def get_ispan_duration(data, np_val):
    """Retrieves the maximum 'scc' duration across all ranks for iSpan (in seconds)."""
    n = np_val
    dur = max(get_max_recursive(data[str(i)].get("benchmark", {}).get("scc", {}), "duration") for i in range(n)) * 1e-9
    assert dur > 0, f"Non-positive duration found: {dur}"
    return dur


def get_hpc_graph_duration(data, np_val):
    """Retrieves the maximum 'scc' duration across all ranks for hpc_graph (in seconds)."""
    n = np_val
    dur = max(get_max_recursive(data[str(i)].get("benchmark", {}).get("scc", {}), "duration") for i in range(n)) * 1e-9
    assert dur > 0, f"Non-positive duration found: {dur}"
    return dur


def get_all_step_data(data, app, np_val):
    """
    Recursively searches for all nodes under 'benchmark/scc' that contain a 'decided_count'.
    Returns a list of dictionaries, each containing:
    - 'name': The path to the node (joined by underscores).
    - 'duration': The maximum duration for this node across all ranks (in seconds).
    - 'decided_count': The decided count (from rank 0, assuming it's allreduced).
    """
    n = np_val
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


def get_kaspan_progress(data, np_val):
    """Retrieves progress data for KaSpan using recursive search."""
    return get_all_step_data(data, "kaspan", np_val)


def get_ispan_progress(data, np_val):
    """Retrieves progress data for iSpan using recursive search."""
    return get_all_step_data(data, "ispan", np_val)


def get_hpc_graph_progress(data, np_val):
    """Retrieves progress data for hpc_graph using recursive search."""
    return get_all_step_data(data, "hpc_graph", np_val)


def get_kaspan_memory(data, np_val):
    """
    Extracts peak memory usage (increase over base) for KaSpan across all ranks.
    Retrieves the maximum 'memory' (resident set size) reported in 'benchmark'
    and recursively under nested 'scc' scopes.
    Returns average memory increase per core (total increase / np_val).
    """
    mem_sum = 0
    for i in range(np_val):
        node = data[str(i)].get("benchmark", {})
        base = int(node.get("memory", 0))
        mem_sum += max(base, get_max_recursive(node.get("scc", {}), "memory")) - base
    return mem_sum / np_val


def get_ispan_memory(data, np_val):
    """
    Extracts peak memory usage (increase over base) for iSpan across all ranks.
    Retrieves the maximum 'memory' reported in 'benchmark' and recursively under 'scc'.
    Returns average memory increase per core (total increase / np_val).
    """
    mem_sum = 0
    for i in range(np_val):
        node = data[str(i)].get("benchmark", {})
        base = int(node.get("memory", 0))
        mem_sum += max(base, get_max_recursive(node.get("scc", {}), "memory")) - base
    return mem_sum / np_val


def get_hpc_graph_memory(data, np_val):
    """
    Extracts peak memory usage (increase over base) for hpc_graph across all ranks.
    Retrieves 'memory' from benchmark and recursively under 'scc'.
    Returns average memory increase per core (total increase / np_val).
    """
    mem_sum = 0
    for i in range(np_val):
        node = data[str(i)].get("benchmark", {})
        base = int(node.get("memory", 0))
        mem_sum += max(base, get_max_recursive(node.get("scc", {}), "memory")) - base
    return mem_sum / np_val


def get_time_info(max_s):
    if max_s >= 10: return 1, "s"
    if max_s >= 1e-2: return 1e3, "ms"
    if max_s >= 1e-5: return 1e6, "us"
    return 1e9, "ns"


def get_memory_info(max_b):
    if max_b >= 10e9: return 1e-9, "GB"
    if max_b >= 10e6: return 1e-6, "MB"
    if max_b >= 10e3: return 1e-3, "KB"
    return 1, "B"


class FiveDigitFormatter(ticker.Formatter):
    def __call__(self, x, pos=None):
        if x == 0: return "0"
        abs_x = abs(x)
        if abs_x >= 1000:
            return f"{x:.0f}"
        if abs_x >= 100:
            res = f"{x:.2f}"[:6]
        elif abs_x >= 10:
            res = f"{x:.3f}"[:6]
        elif abs_x >= 1:
            res = f"{x:.4f}"[:6]
        elif abs_x >= 0.1:
            res = f"{x:.5f}"[:6]
        else:
            res = f"{x:.10f}"[:7]

        if "." in res:
            res = res.rstrip('0').rstrip('.')
        return res if res != "" else "0"


def setup_ax(ax, title, ylabel, x_min, x_max, y_min, y_max, y_scale="log10", y_formatter=None, x_ticks=None):
    """
    Sets up the axes with common parameters.
    X-axis is Cores, Y-axis is the Metric.
    """
    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xlabel("Number of CPU-Cores")

    # X-axis configuration (Number of CPU-Cores)
    ax.set_xscale("log", base=2)
    if x_ticks is not None:
        ax.set_xticks(x_ticks)
        ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    else:
        ax.xaxis.set_major_formatter(ticker.ScalarFormatter())

    ax.xaxis.get_major_formatter().set_scientific(False)
    ax.xaxis.get_major_formatter().set_useOffset(False)

    # Y-axis configuration (Metric)
    if y_scale == "log10":
        ax.set_yscale("log", base=10)
        if y_formatter == "scientific":
            ax.yaxis.set_major_formatter(ticker.LogFormatterSciNotation(base=10))
        elif y_formatter == "five_digit":
            ax.yaxis.set_major_formatter(FiveDigitFormatter())
            # For log scale, we also want labels at non-power-of-10 if the range is small
            ax.yaxis.set_major_locator(ticker.LogLocator(base=10.0, subs=(1.0, 2.0, 5.0), numticks=10))
        else:
            ax.yaxis.set_major_formatter(ticker.ScalarFormatter())
    elif y_scale == "linear":
        ax.set_yscale("linear")
        if y_formatter == "five_digit":
            ax.yaxis.set_major_formatter(FiveDigitFormatter())
        else:
            fmt = ticker.ScalarFormatter()
            fmt.set_scientific(False)
            ax.yaxis.set_major_formatter(fmt)

    ax.set_xlim(x_min, x_max)
    ax.set_ylim(y_min, y_max)
    ax.grid(True, which="both", linestyle="--", alpha=0.5)


def get_plot_config(plot_type, scaling_type, graph, global_min, global_max, graph_data, baseline_dur=None, app=None):
    """
    Provides a consistent configuration for all plot types.
    """
    t_scale, t_unit = get_time_info(global_max["duration"])
    m_scale, m_unit = get_memory_info(global_max["memory"])

    prefix = f"{scaling_type} " if scaling_type else ""
    app_suffix = f" ({app})" if app else ""

    cfg = {
        "title": f"{prefix}{plot_type.replace('_', ' ').title()}: {graph}{app_suffix}",
        "ylabel": "",
        "y_scale": "log10",
        "y_formatter": None,
        "scale": 1.0,
        "x_min": global_min["cores"],
        "x_max": global_max["cores"],
        "y_min": 0,
        "y_max": 1.0
    }

    if plot_type == "duration":
        cfg["ylabel"] = f"Time [{t_unit}]"
        cfg["scale"] = t_scale
        cfg["y_scale"] = "log10"
        cfg["y_formatter"] = "five_digit"
        min_val = float('inf')
        max_val = 0
        for a in graph_data:
            for n in graph_data[a]:
                val = graph_data[a][n][0] * t_scale
                min_val = min(min_val, val)
                max_val = max(max_val, val)
        cfg["y_min"] = min_val * 0.9 if min_val != float('inf') else 1e-9 * t_scale
        cfg["y_max"] = max_val * 1.1 if max_val > 0 else 1.0
    elif plot_type == "speedup":
        label = "Speedup" if "Strong" in scaling_type else "Efficiency"
        cfg["title"] = f"{prefix}{label}: {graph}{app_suffix}"
        cfg["ylabel"] = label
        cfg["y_scale"] = "linear"
        max_val = 0
        if baseline_dur:
            for a in graph_data:
                for n in graph_data[a]:
                    max_val = max(max_val, baseline_dur / (graph_data[a][n][0] if graph_data[a][n][0] > 0 else 1e-9))
        cfg["y_min"] = 0
        cfg["y_max"] = max_val * 1.1 if max_val > 0 else 1.0
    elif plot_type == "memory":
        cfg["ylabel"] = f"Memory [{m_unit} per CPU-Core]"
        cfg["scale"] = m_scale
        cfg["y_scale"] = "linear"
        cfg["y_formatter"] = "five_digit"
        min_val = float('inf')
        max_val = 0
        for a in graph_data:
            for n in graph_data[a]:
                val = graph_data[a][n][1] * m_scale
                min_val = min(min_val, val)
                max_val = max(max_val, val)
        cfg["y_min"] = min_val * 0.9 if min_val != float('inf') else 0
        cfg["y_max"] = max_val * 1.1 if max_val > 0 else 1.0
    elif plot_type == "step_throughput":
        cfg["ylabel"] = "Decisions per second"
        cfg["y_scale"] = "log10"
        cfg["y_formatter"] = "scientific"
        min_val = float('inf')
        max_val = 0
        for a in graph_data:
            for n in graph_data[a]:
                for s in graph_data[a][n][2]:
                    if s["duration"] > 0 and s["decided_count"] > 0:
                        val = s["decided_count"] / s["duration"]
                        min_val = min(min_val, val)
                        max_val = max(max_val, val)
        cfg["y_min"] = min_val * 0.9 if min_val != float('inf') else 0.1
        cfg["y_max"] = max_val * 1.1 if max_val > 0 else 10.0
    elif plot_type == "step_duration":
        cfg["ylabel"] = f"Time [{t_unit}]"
        cfg["scale"] = t_scale
        cfg["y_scale"] = "log10"
        cfg["y_formatter"] = "scientific"
        min_val = float('inf')
        max_val = 0
        for a in graph_data:
            for n in graph_data[a]:
                for s in graph_data[a][n][2]:
                    if s["duration"] > 0:
                        val = s["duration"] * t_scale
                        min_val = min(min_val, val)
                        max_val = max(max_val, val)
                dur, _, progress = graph_data[a][n]
                other = dur - sum(s["duration"] for s in progress)
                if other > 1e-9:
                    val = other * t_scale
                    min_val = min(min_val, val)
                    max_val = max(max_val, val)
        cfg["y_min"] = min_val * 0.9 if min_val != float('inf') else 1e-9 * t_scale
        cfg["y_max"] = max_val * 1.1 if max_val > 0 else 1.0
    return cfg


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

        cfg = get_plot_config(plot_type, scaling_type, graph, global_min, global_max, graph_data, baseline_dur)
        x_ticks = all_nps
        setup_ax(ax, cfg["title"], cfg["ylabel"], cfg["x_min"], cfg["x_max"], cfg["y_min"], cfg["y_max"],
                 y_scale=cfg["y_scale"], y_formatter=cfg.get("y_formatter"), x_ticks=x_ticks)
        scale = cfg["scale"]

        if plot_type == "speedup":
            if "Strong" in scaling_type:
                # Ideal strong scaling: speedup S = n / min_np
                ax.plot([min_np, max_np], [1.0, max_np / min_np], color="gray", linestyle="--", alpha=0.5,
                        label="ideal", clip_on=True)
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
                x_vals.append(n)
                if plot_type == "duration":
                    y_vals.append(dur * scale)
                elif plot_type == "speedup":
                    if baseline_dur: y_vals.append(baseline_dur / (dur if dur > 0 else 1e-9))
                elif plot_type == "memory":
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
    scaling_prefix = "strong" if "Strong" in scaling_type else "weak"

    for app in sorted(list(apps)):
        if app not in graph_data: continue
        app_nps = sorted(list(graph_data[app].keys()))

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6), layout="constrained")

        cfg1 = get_plot_config("step_throughput", scaling_type, graph, global_min, global_max, graph_data, app=app)
        setup_ax(ax1, cfg1["title"], cfg1["ylabel"], cfg1["x_min"], cfg1["x_max"], cfg1["y_min"], cfg1["y_max"],
                 y_scale=cfg1["y_scale"], y_formatter=cfg1.get("y_formatter"), x_ticks=app_nps)

        cfg2 = get_plot_config("step_duration", scaling_type, graph, global_min, global_max, graph_data, app=app)
        setup_ax(ax2, cfg2["title"], cfg2["ylabel"], cfg2["x_min"], cfg2["x_max"], cfg2["y_min"], cfg2["y_max"],
                 y_scale=cfg2["y_scale"], y_formatter=cfg2.get("y_formatter"), x_ticks=app_nps)
        scale2 = cfg2["scale"]

        stages = sorted(list(set(s["name"] for n in app_nps for s in graph_data[app][n][2])))
        for stage_name in stages:
            c, m = stage_style[stage_name]
            # Throughput
            x_vals, y_vals = [], []
            for n in app_nps:
                progress = graph_data[app][n][2]
                stage_data = next((s for s in progress if s["name"] == stage_name), None)
                if stage_data and stage_data["duration"] > 0 and stage_data["decided_count"] > 0:
                    x_vals.append(n)
                    y_vals.append(stage_data["decided_count"] / stage_data["duration"])
            if x_vals:
                ax1.plot(x_vals, y_vals, label=stage_name, color=c, marker=m)

            # Duration
            x_vals, y_vals = [], []
            for n in app_nps:
                progress = graph_data[app][n][2]
                stage_data = next((s for s in progress if s["name"] == stage_name), None)
                if stage_data:
                    x_vals.append(n)
                    y_vals.append(stage_data["duration"] * scale2)
            if x_vals:
                ax2.plot(x_vals, y_vals, label=stage_name, color=c, marker=m)

        # Add "Other" to Duration plot
        x_vals, y_vals = [], []
        for n in app_nps:
            dur, _, progress = graph_data[app][n]
            val = dur - sum(s["duration"] for s in progress)
            x_vals.append(n)
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
