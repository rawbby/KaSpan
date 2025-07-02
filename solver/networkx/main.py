from typing import Tuple, List

import networkx as nx
import sys


def main():
    # load the graph from an edgelist
    graph = nx.read_edgelist(sys.argv[1], comments='%', create_using=nx.DiGraph(), nodetype=int, edgetype=int)

    # assignments is a sorted list of tuples (node_id, representative)
    # where a representative must be -1 for single element components
    # or the smallest node_id within the component
    assignments: List[Tuple[int, int]] = list()
    for comp in nx.strongly_connected_components(graph):
        if len(comp) == 1:
            v, = comp
            assignments.append((v, -1))
        else:
            rep = min(comp)
            for v in comp:
                assignments.append((v, rep))
    assignments.sort()

    # write the result into an assignment file
    with open(sys.argv[2], mode='w') as f:
        f.write(f"% {graph.number_of_nodes()}\n")
        f.write(f"% {graph.number_of_edges()}\n")
        for v, rep in assignments:
            f.write(f"{v} {rep}\n")


if __name__ == '__main__':
    main()
