#!/usr/bin/env python3
"""
scripts/validate_scc.py

Usage:
    ./scripts/validate_scc.py <scc_assignment.txt> <code>_edgelist.txt

This script reads:
  1) The SCC assignment dump (vertex -> component)
  2) An edge-list file of the form "<u> <v>" per line

It builds a NetworkX directed graph from the edgelist and recomputes
the “true” SCCs, then compares them to your solver’s output.
All data are normalized to 1-based IDs internally.
"""

import sys
from collections import defaultdict

import networkx as nx

def load_assignment(fname):
    """
    Reads lines "v c" into a dict {v: c}.
    Detects 0-based (min_v == 0) vs 1-based (min_v == 1);
    shifts everything to 1-based if needed.
    Returns the normalized dict and a flag indicating shift.
    """
    assign = {}
    with open(fname) as f:
        for line in f:
            v, c = map(int, line.split())
            if c == -1:
                assign[v] = v
            else:
                assign[v] = c

    min_v = min(assign.keys())
    shift_done = False
    if min_v == 0:
        # 0-based detected → shift to 1-based
        assign = {v+1: c+1 for v, c in assign.items()}
        shift_done = True
    elif min_v > 1:
        print(f"Warning: assignment min vertex ID = {min_v}, unexpected (expected 1 or 0)")
    # else min_v == 1: already 1-based

    print(f"Loaded assignment from '{fname}' ({len(assign)} vertices); "
          f"{'shifted 0→1' if shift_done else 'already 1-based'}")
    return assign

def load_edgelist(fname):
    """
    Reads "<u> <v>" edges, returns a DiGraph with 1-based node IDs.
    Detects 0-based vs 1-based by min node ID and shifts if needed.
    """
    edges = []
    min_node = None
    with open(fname) as f:
        for line in f:
            if not line.strip() or line.startswith('%'):
                continue
            u, v = map(int, line.split())
            edges.append((u, v))
            if min_node is None or u < min_node:
                min_node = u
            if v < min_node:
                min_node = v

    shift_done = False
    if min_node == 0:
        # shift all edges to 1-based
        edges = [(u+1, v+1) for u, v in edges]
        shift_done = True
    elif min_node > 1:
        print(f"Warning: edgelist min node ID = {min_node}, unexpected")

    G = nx.DiGraph()
    G.add_edges_from(edges)
    print(f"Loaded edgelist '{fname}' with {G.number_of_nodes()} nodes, "
          f"{G.number_of_edges()} edges; "
          f"{'shifted 0→1' if shift_done else 'already 1-based'}")
    return G

def build_comp_map_from_parts(parts):
    """
    Given {v: rep}, invert to {rep: sorted list of vertices},
    then stringify each list "v1 v2 v3 ...".
    """
    comp = defaultdict(list)
    for v, r in parts.items():
        comp[r].append(v)
    out = {}
    for r, vs in comp.items():
        vs_sorted = sorted(vs)
        out[r] = " ".join(map(str, vs_sorted))
    return out

def main():
    if len(sys.argv) != 3:
        print("Usage: validate_scc.py <scc_dump.txt> <code>_edgelist.txt>")
        sys.exit(1)

    dump_file, edgelf = sys.argv[1], sys.argv[2]

    print(f"\n=== Validation run ===")
    print(f"Solver dump : {dump_file}")
    print(f"Edge-list   : {edgelf}\n")

    # 1. Load and normalize assignment
    sol = load_assignment(dump_file)

    # 2. Load and normalize graph to 1-based
    G = load_edgelist(edgelf)

    # 3. Compute true SCCs (1-based IDs)
    print("Computing true SCCs with NetworkX...")
    true_parts = {}
    for comp in nx.strongly_connected_components(G):
        rep = min(comp)
        for v in comp:
            true_parts[v] = rep
    print(f"Found {len(set(true_parts.values()))} true SCCs\n")

    # 4. Build comp→vertex-list maps
    print("Building component maps for comparison...")
    true_map   = build_comp_map_from_parts(true_parts)
    loaded_map = build_comp_map_from_parts(sol)
    print(f"  True map size  : {len(true_map)} components")
    print(f"  Loaded map size: {len(loaded_map)} components\n")

    # 5. Compare and report up to 10 diffs
    print("Comparing components…")
    all_keys = set(true_map) | set(loaded_map)
    diffs = []
    for k in sorted(all_keys):
        t = true_map.get(k)
        l = loaded_map.get(k)
        if t is None:
            diffs.append((k, 'only_loaded', l))
        elif l is None:
            diffs.append((k, 'only_true',   t))
        elif t != l:
            diffs.append((k, 'differs', (t, l)))
        if len(diffs) >= 10:
            break

    if not diffs:
        print("\n✅ Validation PASSED: all components match exactly.")
        sys.exit(0)

    print("\n❌ Validation FAILED: first mismatches:")
    for k, kind, data in diffs:
        if kind == 'only_true':
            print(f"\nComponent rep={k} only in TRUE")
            print(data[:100] + ('…' if len(data) > 100 else ''))
        elif kind == 'only_loaded':
            print(f"\nComponent rep={k} only in LOADED")
            print(data[:100] + ('…' if len(data) > 100 else ''))
        else:  # differs
            t, l = data
            t_snip = t[:100] + ('…' if len(t) > 100 else '')
            l_snip = l[:100] + ('…' if len(l) > 100 else '')
            print(f"\nComponent rep={k} differs")
            print(f"  true  : {t_snip}")
            print(f"  loaded: {l_snip}")
    sys.exit(1)

if __name__ == "__main__":
    main()
