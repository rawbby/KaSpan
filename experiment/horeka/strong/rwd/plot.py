import json
import os
import pathlib
import re
import traceback

import matplotlib.pyplot as plt

cwd = pathlib.Path(os.getcwd())
pattern = re.compile(r"((?:kaspan)|(?:ispan))_(.*)_np([0-9]+)\.json")

paths = [p for p in cwd.iterdir() if p.is_file()]

apps = set()
graphs = set()

duration_data = {}
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
            memory_data.setdefault((app, graph), {})[n] = max(
                int(output[str(i)]["benchmark"]["scc"]["duration"]) for i in range(n)
            )

    except (KeyError, json.JSONDecodeError):
        print(f"[SKIPPING] {path} (invalid)")
        print(traceback.format_exc())
        continue

for app_graph, values in duration_data.items():
    app, graph = app_graph
    nps = sorted(values.keys())
    durations = [float(values[n]) * 10e-9 for n in nps]

    plt.figure()
    plt.plot(nps, durations, marker="o")
    plt.xlabel("np")
    plt.ylabel("seconds")
    plt.title(f"Strong Scaling {app} '{graph}'")
    plt.grid(True)
    plt.xscale("log", base=2)
    plt.tight_layout()

    plt.savefig(cwd / f"{app}_{graph}.png", dpi=200)
    plt.close()

    print(cwd / f"{app}_{graph}.png")

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

    plt.figure()
    for app, values in data.items():
        # build durations aligned with union nps, use NaN for missing
        durations = [values.get(n, float("nan")) for n in nps]
        plt.plot(nps, durations, marker="o", label=app)

    plt.xlabel("np")
    plt.ylabel("seconds")
    plt.title(f"Strong Scaling '{graph}'")
    plt.grid(True)
    plt.yscale("log", base=10)  # use base-10 log scale on seconds
    plt.xscale("log", base=2)
    plt.legend()
    plt.tight_layout()

    plt.savefig(cwd / f"{graph}.png", dpi=200)
    plt.close()

    print(cwd / f"{graph}.png")
