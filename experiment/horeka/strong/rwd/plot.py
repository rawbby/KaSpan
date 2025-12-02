import json
import os
import pathlib
import re
import traceback

import matplotlib.pyplot as plt

cwd = pathlib.Path(os.getcwd())
pattern = re.compile(r"((?:kaspan)|(?:ispan))_(.*)_np([0-9]+)\.json")

paths = [p for p in cwd.iterdir() if p.is_file()]
duration_data = {}
memory_data = {}

for path in paths:
    try:
        match = pattern.fullmatch(path.name)
        if not match:
            print(f"[SKIPPING] {path} (irrelevant)")
            continue

        app = match.group(1)
        graph = match.group(2)
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
