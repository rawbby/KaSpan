#!/usr/bin/env python3
import json
import math
from pathlib import Path
from typing import Any, Dict, List

import matplotlib.pyplot as plt
import numpy as np

from base import matrix_product


def normalize_json(json_path: Path):
    def merge_values(a: Any, b: Any) -> Any:
        if isinstance(a, dict) and isinstance(b, dict):
            out = dict(a)
            for k, v in b.items():
                out[k] = merge_values(out[k], v) if k in out else v
            return out
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


def pad_to(arr: List[float], L: int) -> np.ndarray:
    out = np.full(L, np.nan, dtype=float)
    l = min(len(arr), L)
    if l > 0:
        out[:l] = np.array(arr[:l], dtype=float)
    return out


def stats_lines(matrix: np.ndarray):
    with np.errstate(all="ignore"):
        mins = np.nanmin(matrix, axis=0)
        p25 = np.nanpercentile(matrix, 25, axis=0)
        mean = np.nanmean(matrix, axis=0)
        p75 = np.nanpercentile(matrix, 75, axis=0)
        maxs = np.nanmax(matrix, axis=0)
    return mins, p25, mean, p75, maxs


def main():
    matrix: Dict[str, List[Any]] = {
        'program': ['kaspan'],
        'N': [16, 18, 20],
        'd': [0.5, 1.0, 2.0, 4.0, 8.0, 16.0],
        'graph': ['gnm_directed'],
        'NP': [6, 8, 10, 12],
        'seed': [20804110],
    }

    for N, d, NP, program, graph, seed in matrix_product(matrix, 'N', 'd', 'NP', 'program', 'graph', 'seed'):
        try:
            t = 2 ** NP
            n = 2 ** (NP + N)
            m = math.ceil(d * n)

            name = f"{program}_np{t}_{graph}_n{n}_m{m}_s{seed}"

            script_dir = Path(__file__).resolve().parent
            downloaded_experiment_dir = script_dir / 'experiment_data' / '37ef00_d487'
            data = normalize_json(downloaded_experiment_dir / f"{name}.json")

            ranks = sorted(data.keys())
            fw_prog = [data[rank]["benchmark"]["scc"]["forward_search"]["processing"]["processed_count"] for rank in
                       ranks]
            fw_work = [data[rank]["benchmark"]["scc"]["forward_search"]["processing"]["duration"] for rank in ranks]
            fw_comm = [data[rank]["benchmark"]["scc"]["forward_search"]["communication"]["duration"] for rank in ranks]
            bw_prog = [data[rank]["benchmark"]["scc"]["backward_search"]["processing"]["processed_count"] for rank in
                       ranks]
            bw_work = [data[rank]["benchmark"]["scc"]["backward_search"]["processing"]["duration"] for rank in ranks]
            bw_comm = [data[rank]["benchmark"]["scc"]["backward_search"]["communication"]["duration"] for rank in ranks]

            fw_iterations = len(fw_prog[0])
            for l in (fw_prog, fw_work, fw_comm):
                for i in range(len(ranks)):
                    assert fw_iterations == len(l[i])

            bw_iterations = len(bw_prog[0])
            for l in (bw_prog, bw_work, bw_comm):
                for i in range(len(ranks)):
                    assert bw_iterations == len(l[i])

            max_iterations = max(fw_iterations, bw_iterations)
            x = np.arange(1, max_iterations + 1)  # x-axis

            # Build matrices [ranks x iterations], padded with NaN
            def build_matrix(series_list: List[List[int]], length: int) -> np.ndarray:
                if not series_list:
                    return np.full((0, length), np.nan)
                return np.vstack([pad_to(s, length) for s in series_list])

            fw_work_mat = build_matrix(fw_work, max_iterations)
            fw_comm_mat = build_matrix(fw_comm, max_iterations)
            bw_work_mat = build_matrix(bw_work, max_iterations)
            bw_comm_mat = build_matrix(bw_comm, max_iterations)

            # Throughput: nodes per (work + comm) time per iteration
            def build_throughput(prog_list: List[List[int]], work_list: List[List[int]], comm_list: List[List[int]],
                                 length: int) -> np.ndarray:
                mats = []
                for p, w, c in zip(prog_list, work_list, comm_list):
                    m = min(len(p), len(w), len(c))
                    if m == 0:
                        mats.append(pad_to([], length))
                        continue
                    p_arr = np.array(p[:m], dtype=float)
                    t_arr = np.array(w[:m], dtype=float) + np.array(c[:m], dtype=float)
                    with np.errstate(divide="ignore", invalid="ignore"):
                        thr = np.where(t_arr > 0, p_arr / t_arr, np.nan)
                    mats.append(pad_to(thr.tolist(), length))
                if not mats:
                    return np.full((0, length), np.nan)
                return np.vstack(mats)

            fw_thr_mat = build_throughput(fw_prog, fw_work, fw_comm, max_iterations)
            bw_thr_mat = build_throughput(bw_prog, bw_work, bw_comm, max_iterations)

            # Stats lines
            fw_work_min, fw_work_p25, fw_work_mean, fw_work_p75, fw_work_max = stats_lines(fw_work_mat)
            fw_comm_min, fw_comm_p25, fw_comm_mean, fw_comm_p75, fw_comm_max = stats_lines(fw_comm_mat)
            bw_work_min, bw_work_p25, bw_work_mean, bw_work_p75, bw_work_max = stats_lines(bw_work_mat)
            bw_comm_min, bw_comm_p25, bw_comm_mean, bw_comm_p75, bw_comm_max = stats_lines(bw_comm_mat)
            fw_thr_min, fw_thr_p25, fw_thr_mean, fw_thr_p75, fw_thr_max = stats_lines(fw_thr_mat)
            bw_thr_min, bw_thr_p25, bw_thr_mean, bw_thr_p75, bw_thr_max = stats_lines(bw_thr_mat)

            # Prepare plot
            plt.style.use("seaborn-v0_8")
            fig, axes = plt.subplots(2, 3, figsize=(15, 7), dpi=120, sharex=True)

            colors = {
                "min": "#1f77b4",
                "p25": "#17becf",
                "mean": "#2ca02c",
                "p75": "#bcbd22",
                "max": "#ff7f0e",
            }

            # Helper to plot lines
            def plot_lines(ax, lines, title, ylabel=None):
                ax.plot(x, lines["min"], label="Min", color=colors["min"])
                ax.plot(x, lines["p25"], label="25%", color=colors["p25"])
                ax.plot(x, lines["mean"], label="Mean", color=colors["mean"])
                ax.plot(x, lines["p75"], label="75%", color=colors["p75"])
                ax.plot(x, lines["max"], label="Max", color=colors["max"])
                ax.set_title(title)
                if ylabel:
                    ax.set_ylabel(ylabel)
                ax.set_xlabel("Iteration")

            plot_lines(
                axes[0, 0],
                {"min": fw_work_min, "p25": fw_work_p25, "mean": fw_work_mean, "p75": fw_work_p75, "max": fw_work_max},
                "Forward: Work time", "Time (ns)")
            plot_lines(
                axes[0, 1],
                {"min": fw_comm_min, "p25": fw_comm_p25, "mean": fw_comm_mean, "p75": fw_comm_p75, "max": fw_comm_max},
                "Forward: Comm time")
            plot_lines(
                axes[0, 2],
                {"min": fw_thr_min, "p25": fw_thr_p25, "mean": fw_thr_mean, "p75": fw_thr_p75, "max": fw_thr_max},
                "Forward: Nodes per (work+comm) time", "Nodes per ns")
            plot_lines(
                axes[1, 0],
                {"min": bw_work_min, "p25": bw_work_p25, "mean": bw_work_mean, "p75": bw_work_p75, "max": bw_work_max},
                "Backward: Work time", "Time (ns)")
            plot_lines(
                axes[1, 1],
                {"min": bw_comm_min, "p25": bw_comm_p25, "mean": bw_comm_mean, "p75": bw_comm_p75, "max": bw_comm_max},
                "Backward: Comm time")
            plot_lines(
                axes[1, 2],
                {"min": bw_thr_min, "p25": bw_thr_p25, "mean": bw_thr_mean, "p75": bw_thr_p75, "max": bw_thr_max},
                "Backward: Nodes per (work+comm) time", "Nodes per ns")

            y1_y2_max = np.nanmax(np.vstack([fw_work_max, bw_work_max, fw_comm_max, bw_comm_max]))
            y3_max = np.nanmax(np.vstack([fw_thr_max, bw_thr_max]))

            for ax in (axes[0, 0], axes[1, 0]):
                ax.set_ylim(0, y1_y2_max * 1.05 if np.isfinite(y1_y2_max) else 1)
            for ax in (axes[0, 1], axes[1, 1]):
                ax.set_ylim(0, y1_y2_max * 1.05 if np.isfinite(y1_y2_max) else 1)
            for ax in (axes[0, 2], axes[1, 2]):
                ax.set_ylim(0, y3_max * 1.05 if np.isfinite(y3_max) else 1)

            for j in range(3):
                axes[0, j].legend()

            fig.suptitle(
                "Forward/Backward over iterations: Work, Communication, and Nodes/(Work+Comm) — min/25%/mean/75%/max",
                fontsize=11)
            fig.tight_layout(rect=[0, 0, 1, 0.95])
            plt.savefig(script_dir / f"{name}.png")
        except BaseException:
            ...


if __name__ == "__main__":
    main()
