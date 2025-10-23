import itertools
import json
import math
import os.path
import re
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

pattern = re.compile(r"^horeka\.job\.(?P<name>.+?)_np(?P<np>\d+)_gnmd_N(?P<N>\d+)_d(?P<d>\d+)\.json$")

if __name__ == "__main__":
    directory = os.path.abspath(os.path.join(os.path.dirname(__file__), 'experiment_data', '2025-10-20-54236512'))

    result = defaultdict(lambda: defaultdict(list))
    variant_labels_set = set()  # collect all variants once during file scan

    for p in Path(directory).iterdir():
        if not p.is_file():
            continue

        m = pattern.match(p.name)
        if not m:
            continue

        name = m.group("name")
        big_np = int(math.log2(int(m.group("np"))))
        big_n = int(m.group("N"))
        d_per_cent = int(m.group("d"))
        big_n_per_np = big_n - big_np

        result[(big_n_per_np, d_per_cent)][name].append((big_np, p.name))

        # build global variant label right here for consistent color mapping
        name_match = re.match(r'([A-Za-z]+)(.*)', name)
        if name_match:
            variant_parts = [it for it in name_match.group(2).split('_') if it]
            base_name = name_match.group(1)
            full_label = base_name if not variant_parts else f"{base_name} ({','.join(variant_parts)})"
            variant_labels_set.add(full_label)

    # global, consistent color mapping across ALL plots and subplots
    variant_labels = sorted(variant_labels_set)
    cmap = plt.get_cmap('tab20')
    label_to_color = {label: cmap(i % cmap.N) for i, label in enumerate(variant_labels)}

    def extract_scc_seconds(output_obj, base_name_: str):
        return float(output_obj['data']['root'][base_name_][f"{base_name_}_scc"]['statistics']['max'][0])

    def extract_scc_fwbw_seconds(output_obj, base_name_: str):
        return float(output_obj['data']['root'][base_name_][f"{base_name_}_scc"][f"{base_name_}_fwbw"]['statistics']['max'][0])

    def extract_residual_seconds(output_obj, base_name_: str):
        return float(output_obj['data']['root'][base_name_][f"{base_name_}_scc"][f"{base_name_}_residual"]['statistics']['max'][0])

    def extract_residual_wcc_seconds(output_obj, base_name_: str):
        if f"{base_name_}_wcc" in output_obj['data']['root'][base_name_][f"{base_name_}_scc"].keys():
            return float(output_obj['data']['root'][base_name_][f"{base_name_}_scc"][f"{base_name_}_wcc"]['statistics']['max'][0])
        else:
            return 0.0

    def extract_residual_wcc_fwbw_seconds(output_obj, base_name_: str):
        if f"{base_name_}_wcc_fwbw" in output_obj['data']['root'][base_name_][f"{base_name_}_scc"].keys():
            return float(output_obj['data']['root'][base_name_][f"{base_name_}_scc"][f"{base_name_}_wcc_fwbw"]['statistics']['max'][0])
        else:
            return 0.0

    plot_strategies = {
        '': extract_scc_seconds,
        '_ForwardBackwardSearch': extract_scc_fwbw_seconds,
        '_ResidualExchange': extract_residual_seconds,
        '_ResidualWCC': extract_residual_wcc_seconds,
        '_ResidualForwardBackwardSearch': extract_residual_wcc_fwbw_seconds
    }

    for suffix, extract_seconds in plot_strategies.items():
        collage_series = {}

        # pass 1: collect all data for this suffix
        for (big_n_per_np, d_per_cent), names in result.items():
            series_data = defaultdict(list)

            for name, entries in names.items():
                name_match = re.match(r'([A-Za-z]+)(.*)', name)
                variant_parts = [it for it in name_match.group(2).split('_') if len(it) != 0]
                base_name = name_match.group(1)
                full_name = base_name if not variant_parts else f"{base_name} ({','.join(variant_parts)})"

                entries.sort()
                for big_np, filename in entries:
                    with open(os.path.join(directory, filename)) as fh:
                        output = json.load(fh)
                    seconds = extract_seconds(output, base_name)
                    series_data[full_name].append((2 ** big_np, float(seconds)))

            collage_series[(big_n_per_np, d_per_cent)] = series_data

            # per-config figure with global colors
            fig, ax = plt.subplots(figsize=(7, 4))
            for label, pts in series_data.items():
                pts.sort()
                xs = [x for x, _ in pts]
                ys = [y for _, y in pts]
                color = label_to_color.get(label, 'C0')
                ax.plot(xs, ys, marker='o', label=label, color=color)

            ax.set_title(f"WeakScaling{suffix} (gnm-directed;$n=2^{{(np+{big_n_per_np})}}$;$m={d_per_cent / 100.0}n$)")
            ax.set_xlabel('np')
            ax.set_ylabel('time(s)')
            ax.set_xscale('log', base=2)
            ax.set_yscale('log', base=2)
            ax.grid(True, linestyle='--', alpha=0.4)
            ax.legend()
            fig.tight_layout()

            out_path = os.path.join(directory, f"weak{suffix}_N{big_n_per_np}_d{d_per_cent}.png")
            fig.savefig(out_path, dpi=300)
            plt.close(fig)

        # collage figure (use the same global color mapping)
        rows = sorted({N for (N, _) in collage_series.keys()})
        cols = sorted({d for (_, d) in collage_series.keys()})

        fig, axes = plt.subplots(
            nrows=len(rows),
            ncols=len(cols),
            figsize=(4 * len(cols), 3 * len(rows)),
            sharex=True,
            sharey=True,
        )

        # normalize axes to 2D indexable form
        if len(rows) == 1 and len(cols) == 1:
            axes = [[axes]]
        elif len(rows) == 1:
            axes = [axes]
        elif len(cols) == 1:
            axes = [[ax] for ax in axes]

        # collect global ranges for synced limits
        all_xs, all_ys = [], []

        for i, N in enumerate(rows):
            for j, d in enumerate(cols):
                ax = axes[i][j]
                key = (N, d)
                sd = collage_series.get(key)
                if not sd:
                    ax.set_visible(False)
                    continue

                for label, pts in sd.items():
                    pts = sorted(pts)
                    xs = [x for x, _ in pts]
                    ys = [y for _, y in pts]
                    color = label_to_color.get(label, 'C0')
                    ax.plot(xs, ys, marker='o', label=label, color=color)
                    all_xs.extend(xs)
                    all_ys.extend(ys)

                if i == 0:
                    ax.set_title(f"$m={d / 100.0}n$")
                if j == 0:
                    ax.set_ylabel(f"$n=np\\times2^{{{N}}}$")

        # log2 scales and grid on all axes
        for ax in itertools.chain.from_iterable(axes):
            if not ax.get_visible():
                continue
            ax.set_xscale('log', base=2)
            ax.set_yscale('log', base=2)
            ax.grid(True, linestyle='--', alpha=0.4)

        # synced limits
        if all_xs and all_ys:
            x_min, x_max = min(all_xs), max(all_xs)
            y_min, y_max = min(all_ys), max(all_ys)
            for ax in itertools.chain.from_iterable(axes):
                if ax.get_visible():
                    ax.set_xlim(x_min, x_max)
                    ax.set_ylim(y_min, y_max)

        # label only outer axes
        for ax in axes[-1]:
            if ax.get_visible():
                ax.set_xlabel("np")
        for row_axes in axes:
            if row_axes[0].get_visible():
                row_axes[0].set_ylabel(row_axes[0].get_ylabel() + "\ntime(s)")

        # single collage legend with consistent colors (use proxies for all variants)
        legend_handles = [Line2D([0], [0], color=label_to_color[l], marker='o', linestyle='-') for l in variant_labels]
        fig.legend(legend_handles, variant_labels, loc='upper center',
                   ncol=max(1, len(variant_labels)), frameon=True, bbox_to_anchor=(0.5, 1.02))

        fig.tight_layout(rect=(0, 0, 1, 0.92))
        fig.savefig(os.path.join(directory, f"weak{suffix}_collage.png"), dpi=300, bbox_inches='tight')
        plt.close(fig)
