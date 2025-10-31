#!/usr/bin/env python3
import json
import math
from pathlib import Path
from typing import Any, Dict, List

import matplotlib.pyplot as plt

from base import matrix_product


def normalize_json(json_path: Path):
    def merge_values(a: Any, b: Any) -> Any:
        if isinstance(a, dict) and isinstance(b, dict):
            return {k: merge_values(a[k], b[k]) for k in b.keys()}
        if not isinstance(a, list):
            a = [a]
        if not isinstance(b, list):
            b = [b]
        return a + b

    def collect_deep(pairs: List[tuple]) -> Dict:
        out: Dict[str, Any] = {}
        for k, v in pairs:
            out[k] = merge_values(out[k], v) if k in out else v
        return out

    with open(json_path) as json_file:
        return json.load(json_file, object_pairs_hook=collect_deep)


def deep_get(data: Dict[str, Any], *path: str, default=math.nan) -> Any:
    try:
        for k in path:
            data = data[k]
        return data
    except KeyError:
        return default


def ranks_max(data: Dict[str, Any], *path: str) -> int:
    return max([deep_get(v, *path) for v in data.values()])


def main():
    matrix: Dict[str, List[Any]] = {
        'program': ['kaspan'],
        'N': [16, 18, 20],
        'd': [0.5, 1.0, 2.0, 4.0, 8.0, 16.0],
        'graph': ['gnm_directed'],
        'NP': [6, 8, 10, 12],
        'seed': [20804110],
    }

    for N, d, program, graph, seed in matrix_product(matrix, 'N', 'd', 'program', 'graph', 'seed'):
        script_dir = Path(__file__).resolve().parent
        downloaded_experiment_dir = script_dir / 'experiment_data' / '37ef00_d487'

        x: List[int] = []
        ys: List[List[float]] = [[] for _ in range(7)]

        for NP, in sorted(matrix_product(matrix, 'NP')):
            t = 2 ** NP
            n = 2 ** (NP + N)
            m = math.ceil(d * n)

            try:
                name = f"{program}_np{t}_{graph}_n{n}_m{m}_s{seed}"
                data = normalize_json(downloaded_experiment_dir / f"{name}.json")
            except BaseException:
                x.append(n)
                ys[0].append(math.nan)
                ys[1].append(math.nan)
                ys[2].append(math.nan)
                ys[3].append(math.nan)
                ys[4].append(math.nan)
                ys[5].append(math.nan)
                ys[6].append(math.nan)
                continue

            x.append(n)
            ys[0].append(ranks_max(data, 'benchmark', 'scc', 'trim_1_first', 'duration'))
            # ys[1].append(ranks_max(data, 'benchmark', 'scc', 'pivot_selection', 'duration'))
            ys[1].append(ranks_max(data, 'benchmark', 'scc', 'forward_search', 'duration'))
            ys[2].append(ranks_max(data, 'benchmark', 'scc', 'backward_search', 'duration'))
            ys[3].append(ranks_max(data, 'benchmark', 'scc', 'residual', 'duration'))
            ys[4].append(ranks_max(data, 'benchmark', 'scc', 'residual', 'residual_wcc', 'duration'))
            ys[5].append(ranks_max(data, 'benchmark', 'scc', 'residual', 'residual_scc', 'duration'))
            ys[6].append(ranks_max(data, 'benchmark', 'scc', 'residual', 'post_processing', 'duration'))

        print(x)
        print(ys)
        labels = [
            "trim_1_first",
            "pivot_selection",
            "forward_search",
            "backward_search",
            "residual_gather",
            "residual_wcc",
            "residual_scc",
            "residual_post_processing",
        ]
        colors = [
            "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728",
            "#9467bd", "#8c564b", "#e377c2", "#7f7f7f"
        ]
        markers = ["o", "s", "v", "^", "D", "P", "X", "*"]

        plt.style.use("seaborn-v0_8")
        fig, ax = plt.subplots(1, 1, figsize=(10, 6), dpi=120)

        for i in range(7):
            ax.plot(x, ys[i], label=labels[i], color=colors[i], marker=markers[i], linewidth=1.6)

        ax.set_xlabel("n (nodes)")
        ax.set_ylabel("Phase time (ns)")
        ax.set_title(f"Phases (max across ranks) — {program}, {graph}, N={N}, d={d}, seed={seed}")
        ax.grid(True, linestyle="--", alpha=0.4)
        ax.legend(ncol=2, fontsize=9)

        fig.tight_layout()
        plt.savefig(script_dir / f"{program}_{graph}_N{N}_d{d}_s{seed}_phases.png")
        plt.close(fig)


if __name__ == "__main__":
    main()
