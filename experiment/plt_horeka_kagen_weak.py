#!/bin/python
import json

from base import *
from kaspan import *

import re
from typing import Any, Iterable, Union

PatternLike = Union[str, re.Pattern]

def collapse_sum_inplace(data: dict, path: Iterable[PatternLike]) -> None:
    path = list(path)
    if not path:
        return

    def matches(key: Any, pat: PatternLike) -> bool:
        if isinstance(pat, re.Pattern):
            return pat.fullmatch(str(key)) is not None
        return key == pat

    def recurse(node: Any, idx: int) -> None:
        if not isinstance(node, dict):
            return
        pat = path[idx]
        # iterate over a snapshot of keys to avoid mutation issues
        for k in list(node.keys()):
            if not matches(k, pat):
                continue
            v = node[k]
            if idx == len(path) - 1:
                if isinstance(v, list):
                    node[k] = sum(v)
            else:
                recurse(v, idx + 1)

    recurse(data, 0)


downloaded_experiment_dir: Path = script_dir / 'experiment_data' / '37ef00_d487'


def sum_processed_counts(data):
    sums = {}
    for k, v in data.items():
        if re.fullmatch(r"\d+", k):
            try:
                arr = v["benchmark"]["scc"]["forward_search"]["processed_count"]
            except (KeyError, TypeError):
                continue
            sums[k] = sum(arr) if isinstance(arr, list) else int(arr)
    return sums

def collapse_processed_counts_inplace(data):
    """
    Replace the processed_count list with its sum at the same path, in place.
    """
    for k, v in data.items():
        if re.fullmatch(r"\d+", k) and isinstance(v, dict):
            node = v.get("benchmark", {}).get("scc", {}).get("forward_search", {})
            arr = node.get("processed_count")
            if isinstance(arr, list):
                node["processed_count"] = sum(arr)

def normalize_json(json_path: Path):
    def merge_values(a: Any, b: Any) -> Any:
        # both dicts -> merge recursively
        if isinstance(a, dict) and isinstance(b, dict):
            out = dict(a)  # shallow copy
            for k, v in b.items():
                if k in out:
                    out[k] = merge_values(out[k], v)
                else:
                    out[k] = v
            return out
        # both lists -> concat
        if isinstance(a, list) and isinstance(b, list):
            return a + b
        # a is list -> append b
        if isinstance(a, list):
            return a + [b]
        # b is list -> prepend a
        if isinstance(b, list):
            return [a] + b
        # both scalars -> make list of both
        return [a, b]

    def collect_deep(pairs: List[tuple]) -> Dict:
        out: Dict[str, Any] = {}
        for k, v in pairs:
            if k in out:
                out[k] = merge_values(out[k], v)
            else:
                out[k] = v
        return out

    with open(json_path) as json_file:
        return json.load(json_file, object_pairs_hook=collect_deep)


def main():
    kaspan = normalize_json(downloaded_experiment_dir / 'kaspan_np256_gnm_directed_n16777216_m33554432_s20804110.json')
    kaspan_async = normalize_json(downloaded_experiment_dir / 'kaspan_async_np256_gnm_directed_n16777216_m33554432_s20804110.json')
    kaspan_async_indirect = normalize_json(downloaded_experiment_dir / 'kaspan_async_indirect_np256_gnm_directed_n16777216_m33554432_s20804110.json')
    ispan = normalize_json(downloaded_experiment_dir / 'ispan_np256_gnm_directed_n16777216_m33554432_s20804110.json')

    collapse_sum_inplace(kaspan_async, [re.compile(r"\d+"), "benchmark", "scc", "forward_search", "processed_count"])
    collapse_sum_inplace(kaspan_async_indirect, [re.compile(r"\d+"), "benchmark", "scc", "forward_search", "processing"])

    collapse_sum_inplace(kaspan_async, [re.compile(r"\d+"), "benchmark", "scc", "forward_search", "processed_count"])
    collapse_sum_inplace(kaspan_async_indirect, [re.compile(r"\d+"), "benchmark", "scc", "forward_search", "processing"])

    collapse_sum_inplace(kaspan_async, [re.compile(r"\d+"), "benchmark", "scc", "backward_search", "processed_count"])
    collapse_sum_inplace(kaspan_async_indirect, [re.compile(r"\d+"), "benchmark", "scc", "backward_search", "processing"])

    collapse_sum_inplace(kaspan_async, [re.compile(r"\d+"), "benchmark", "scc", "backward_search", "processed_count"])
    collapse_sum_inplace(kaspan_async_indirect, [re.compile(r"\d+"), "benchmark", "scc", "backward_search", "processing"])

    print(kaspan)
    print(kaspan_async)
    print(kaspan_async_indirect)
    print(ispan)

if __name__ == '__main__':
    main()
