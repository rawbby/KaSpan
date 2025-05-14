#!/usr/bin/env python3
import os, glob, re
import matplotlib.pyplot as plt

def collect_data():
    """
    Walk results/{strong,weak}/* and build:
      data[machine][scaling][code] = [(np, time), …]
    """
    data = {}
    for scaling in ("strong", "weak"):
        base = os.path.join("results", scaling)
        if not os.path.isdir(base):
            continue
        for machine in os.listdir(base):
            mdir = os.path.join(base, machine)
            if not os.path.isdir(mdir):
                continue
            for fn in glob.glob(os.path.join(mdir, "*.log")):
                name = os.path.basename(fn)
                m = re.match(r'(.+)_np(\d+)\.log$', name)
                if not m:
                    continue
                code, np = m.group(1), int(m.group(2))
                t = None
                with open(fn) as f:
                    for line in f:
                        if 'total time' in line.lower():
                            pm = re.search(r'total time[,:]?\s*([0-9]*\.?[0-9]+)', line)
                            if pm:
                                t = float(pm.group(1))
                                break
                if t is None:
                    print(f"Warning: no total time in {fn}")
                    continue

                data.setdefault(machine, {}) \
                    .setdefault(scaling, {}) \
                    .setdefault(code, []) \
                    .append((np, t))
    return data

def make_plots(data):
    """
    For each machine/scaling/code, plot np vs time,
    skipping existing files.
    """
    for machine, by_scaling in data.items():
        outdir = os.path.join("results", "plots", machine)
        os.makedirs(outdir, exist_ok=True)

        for scaling, by_code in by_scaling.items():
            for code, pts in by_code.items():
                pts.sort(key=lambda x: x[0])
                xs, ys = zip(*pts)
                outfn = os.path.join(outdir, f"{scaling}_{code}.png")
                if os.path.exists(outfn):
                    print(f"Skipping existing plot {outfn}")
                    continue

                plt.figure()
                plt.plot(xs, ys, marker='o', linestyle='-')
                plt.xlabel('MPI ranks')
                plt.ylabel('Total time (s)')
                plt.title(f"{machine} — {scaling.capitalize()} scaling: {code}")
                plt.grid(True)
                plt.tight_layout()
                plt.savefig(outfn)
                plt.close()
                print(f"Wrote plot {outfn}")

if __name__ == "__main__":
    data = collect_data()
    make_plots(data)
