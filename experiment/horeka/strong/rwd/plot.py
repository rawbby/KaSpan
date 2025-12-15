import itertools
import json
import os
import pathlib
import re
import traceback

import matplotlib.pyplot as plt

# Topology (hard-coded)
cores_per_socket = 38
sockets_per_node = 2
cores_per_node = 76  # = cores_per_socket * sockets_per_node

def mark_topology_sections(ax, max_n):
    """
    Shade and label topology regions on the x-axis (np) and draw boundary lines.
    Regions:
      [1,1]              -> single thread
      [2, cores_per_socket] -> single socket
      [cores_per_socket+1, cores_per_node] -> single node
      [cores_per_node+1, max_n] -> multi node
    Works with linear or log-scaled x-axes.
    """
    # Regions (start, end, label)
    regions = [
        (1, 1, 'single thread'),
        (2, cores_per_socket, 'single socket'),
        (cores_per_socket + 1, cores_per_node, 'single node'),
        (cores_per_node + 1, max_n, 'multi node'),
    ]
    # Colors per region
    colors = ['#d1e5f0', '#92c5de', '#4393c3', '#2166ac']
    # Clamp and draw
    for (start, end, label), col in zip(regions, colors):
        if start > end or start > max_n:
            continue
        x0 = max(1, start)
        x1 = min(max_n, end)
        if x0 > x1:
            continue
        ax.axvspan(x0, x1, color=col, alpha=0.12, linewidth=0, zorder=0)
        # label near top, centered (geometric center for log x)
        try:
            is_log = ax.get_xscale() == 'log'
        except Exception:
            is_log = False
        xcen = (x0 * x1) ** 0.5 if is_log else 0.5 * (x0 + x1)
        ylim = ax.get_ylim()
        ypos = ylim[1] - 0.03 * (ylim[1] - ylim[0])
        ax.text(xcen, ypos, label, ha='center', va='top', fontsize=8, color='0.25', zorder=3, clip_on=False)

    # Boundary lines at the start of each new region (except the very first)
    for b in [2, cores_per_socket + 1, cores_per_node + 1]:
        if 1 <= b <= max_n:
            ax.axvline(b, linestyle='--', color='0.3', linewidth=0.8, zorder=1)


cwd = pathlib.Path(os.getcwd())
pattern = re.compile(r"((?:kaspan)|(?:ispan))_(.*)_np([0-9]+)\.json")

paths = [p for p in cwd.iterdir() if p.is_file()]

apps = set()
graphs = set()

duration_data = {}
ecl_duration_data = {}
memory_data = {}

for path in paths:
    try:
        match = pattern.fullmatch(path.name)
        if not match:
            print(f"[SKIPPING] {path} (irrelevant)")
            continue

        app = match.group(1)
        apps.add(app)

        graph = match.group(2)
        graphs.add(graph)

        n = int(match.group(3))

        with path.open("r") as file:
            output = json.load(file)
            duration_data.setdefault((app, graph), {})[n] = max(
                int(output[str(i)]["benchmark"]["scc"]["duration"]) for i in range(n)
            )
            ecl_duration_data.setdefault((app, graph), {})[n] = max(
                float(output[str(i)]["benchmark"]["scc"].get("ecl", {"duration": 0})["duration"])
                for i in range(n)
            ) * 100.0 / max(
                float(output[str(i)]["benchmark"]["scc"]["duration"]) for i in range(n)
            )

            memory = memory_data.setdefault((app, graph), {})
            memory[n] = 0
            for i in range(n):
                base_mem = int(output[str(i)]["benchmark"]["memory"])

                if app != "ispan" and n == 1:
                    memory[n] += int(output[str(i)]["benchmark"]["scc"]["tarjan"]["memory"]) - base_mem
                elif app != "ispan" and "residual" in output[str(i)]["benchmark"]["scc"].keys():
                    memory[n] += int(output[str(i)]["benchmark"]["scc"]["residual"]["memory"]) - base_mem
                else:
                    memory[n] += int(output[str(i)]["benchmark"]["scc"]["alloc"]["memory"]) - base_mem
            memory[n] /= n

    except (KeyError, json.JSONDecodeError):
        print(f"[SKIPPING] {path} (invalid)")
        print(traceback.format_exc())
        continue

apps = sorted(apps)
colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
markers = ["o", "s", "D", "^", "v", "P", "X", "d", "<", ">"]
style_cycle = itertools.cycle(zip(itertools.cycle(colors), itertools.cycle(markers)))
app_style = {app: next(style_cycle) for app in apps}

# for app_graph, values in duration_data.items():
#     app, graph = app_graph
#     nps = sorted(values.keys())
#     memory = [float(values[n]) * 10e-9 for n in nps]
#
#     plt.figure()
#     color, marker = app_style[app]
#     plt.plot(nps, memory, color=color, marker=marker)
#     plt.xlabel("np")
#     plt.ylabel("seconds")
#     plt.title(f"Strong Scaling {app} '{graph}'")
#     plt.grid(True)
#     plt.xscale("log", base=2)
#     plt.tight_layout()
#
#     plt.savefig(cwd / f"{app}_{graph}.png", dpi=200)
#     plt.close()
#
#     print(cwd / f"{app}_{graph}.png")

for graph in graphs:
    # map app -> {n: duration_seconds}, filter out durations > 4 minutes
    data = {
        app_graph[0]: {
            n: (float(v) * 1e-9)
            for n, v in values.items()
            if (float(v) * 1e-9) <= 4 * 60
        }
        for app_graph, values in duration_data.items()
        if app_graph[1] == graph
    }

    # union of all nps across apps
    nps = sorted({n for values in data.values() for n in values.keys()})

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4))

    for app, values in data.items():
        # build durations aligned with union nps, use NaN for missing
        memory = [values.get(n, float("nan")) for n in nps]
        color, marker = app_style[app]
        ax1.plot(nps, memory, label=app, color=color, marker=marker)

    ax1.set_xlabel("np")
    ax1.set_ylabel("seconds")
    ax1.set_yscale("log", base=10)
    ax1.set_xscale("log", base=2)
    ax1.grid(True)
    if nps:
        mark_topology_sections(ax1, max(nps))

    # map app -> {n: memory}
    data = {
        app_graph[0]: values
        for app_graph, values in memory_data.items()
        if app_graph[1] == graph
    }

    # union of all nps across apps
    nps = sorted({n for values in data.values() for n in values.keys()})

    for app, values in data.items():
        # build durations aligned with union nps, use NaN for missing
        memory = [values.get(n, float("nan")) for n in nps]
        color, marker = app_style[app]
        ax2.plot(nps, memory, label=app, color=color, marker=marker)

    ax2.set_xlabel("np")
    ax2.set_ylabel("bytes/np")
    # ax2.set_yscale("log", base=1024)
    ax2.set_xscale("log", base=2)
    ax2.grid(True)
    if nps:
        mark_topology_sections(ax2, max(nps))

    fig.suptitle(f"Strong Scaling '{graph}'")
    fig.legend()
    fig.tight_layout()
    fig.savefig(cwd / f"{graph}.png", dpi=200)
    plt.close(fig)

    print(cwd / f"{graph}.png")

    fig, (ax2) = plt.subplots(1, 1, figsize=(10, 4))

    data = {
        app_graph[0]: values
        for app_graph, values in ecl_duration_data.items()
        if app_graph[1] == graph
    }

    # union of all nps across apps
    nps = sorted({n for values in data.values() for n in values.keys()})

    for app, values in data.items():
        # build durations aligned with union nps, use NaN for missing
        memory = [values.get(n, float("nan")) for n in nps]
        color, marker = app_style[app]
        ax2.plot(nps, memory, label=app, color=color, marker=marker)

    ax2.set_xlabel("np")
    ax2.set_ylabel("ecl %")
    ax2.set_xscale("log", base=2)
    ax2.grid(True)
    if nps:
        mark_topology_sections(ax2, max(nps))

    fig.suptitle(f"Strong Scaling '{graph}'")
    fig.legend()
    fig.tight_layout()
    fig.savefig(cwd / f"{graph}_ecl.png", dpi=200)
    plt.close(fig)

    print(cwd / f"{graph}_ecl.png")
