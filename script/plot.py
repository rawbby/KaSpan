#!/usr/bin/env python3
import os
import sys
import matplotlib.pyplot as plt
import matplotlib as mpl

KIT_GREEN = "#00876C"
KIT_BLUE  = "#4664AA"
KIT_GREY  = "#404040"
PALETTE   = [KIT_BLUE, KIT_GREEN, KIT_GREY, "#77A200", "#A3107C"]

mpl.rcParams.update({
    "font.family":      ["Frutiger", "Arial", "sans-serif"],
    "font.size":        12,
    "axes.titlesize":   14,
    "axes.labelsize":   12,
    "axes.edgecolor":   KIT_GREY,
    "xtick.color":      KIT_GREY,
    "ytick.color":      KIT_GREY,
    "grid.color":       KIT_GREY,
    "grid.linestyle":   "--",
    "grid.linewidth":   0.6,
    "lines.linewidth":  2,
    "lines.markersize": 7,
})
plt.style.use("default")

PARAMS = ["alpha", "beta", "code", "gamma", "runs", "theta", "threads_per_node", "host_name"]

if len(sys.argv) < 3:
    sys.exit("usage: plot.py <out_dir> <res_dir1> ...")

output_dir, run_dirs = sys.argv[1], sys.argv[2:]

records = [({p: open(f"{d}/{p}").read().strip() for p in PARAMS},
            float(open(f"{d}/nodes").read()), float(open(f"{d}/total_time_ms").read()))
           for d in run_dirs]

shared_params  = {p for p in PARAMS if len({rec[0][p] for rec in records}) == 1}
varying_params = [p for p in PARAMS if p not in shared_params]

# group by varying-parameter combinations
groups = {}
for param_values, node_count, time_ms in records:
    key = tuple((p, param_values[p]) for p in varying_params)
    groups.setdefault(key, []).append((node_count, time_ms))

fig, ax = plt.subplots(figsize=(9, 5.5), facecolor="white")
for idx, (key, points) in enumerate(sorted(groups.items(), key=lambda x: x[0])):
    x_vals, y_vals = zip(*sorted(points))
    label = ", ".join(f"{p}={v}" for p, v in key) or None
    colour = PALETTE[idx % len(PALETTE)]
    ax.plot(x_vals, y_vals, "-o", label=label, color=colour, markeredgewidth=0.6, markeredgecolor="white")

ax.set_xlabel("Nodes")
ax.set_ylabel("Total time [ms]")
ax.set_title("Scaling")
ax.set_xticks(sorted({n for _, n, _ in records}))
ax.margins(x=0.02)
ax.grid(axis="y")

if varying_params:
    ax.legend(title="Variants", frameon=False, bbox_to_anchor=(1.02, 1), loc="upper left")

if shared_params:
    caption = "; ".join(f"{p}={next(iter({rec[0][p] for rec in records}))}" for p in sorted(shared_params))
    fig.text(0.5, -0.10, f"{caption};", ha="center", fontsize=10, color=KIT_GREY)

fig.tight_layout()
os.makedirs(output_dir, exist_ok=True)
fig.savefig(f"{output_dir}/plot.png", dpi=300, bbox_inches="tight")
