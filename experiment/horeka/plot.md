# Plotting Specification

## Plotted Metrics

### Per Graph Instance

- **Duration**: Duration over the number of cores utilized. Compares variants (e.g., KaSpan, iSpan).
- **Speedup**: Speedup over the number of cores utilized. $Speedup = \frac{T_{baseline}}{T_{p}}$, where $T_{baseline}$
  is the duration of the fastest instance at the lowest number of cores that solved the problem.
- **Memory**: Memory allocation per core utilized (Memory Increase / Number of Cores).

### Per Variant (Side-by-Side Plots)

These two plots are saved in the same figure, sharing a single legend without duplicates:

- **Step Throughput**: Throughput per algorithmic substep (nodes per second) over the number of cores.
- **Step Duration**: Duration per algorithmic substep over the number of cores. Includes an "Other" category for time
  consumed in the algorithm outside of the explicitly tracked steps.

---

## Axis Constraints

### X-Axis (Number of Cores)

- **Scale**: Log 2.
- **Numbering**: Full integers, no scientific notation.
- **Limits**: Global minimum and maximum across **ALL** data.
- **Special Requirement**: Print tick labels at every X-value present in the data.

### Y-Axis (Metrics)

| Plot Type           | Scale  | Numbering / Formatting                                                                                                                     | Min Limit      | Max Limit      |
|:--------------------|:-------|:-------------------------------------------------------------------------------------------------------------------------------------------|:---------------|:---------------|
| **Duration**        | Log 10 | Real values, no scientific notation. Max 5 digits total (integer + decimal), max 3 integer digits. Use appropriate factor (ns, us, ms, s). | 0.9 * min data | 1.1 * max data |
| **Speedup**         | Linear | Real values, no scientific notation. Max 5 digits total                                                                                    | 0              | 1.1 * max data |
| **Memory**          | Linear | Real values, no scientific notation. Max 5 digits total, max 3 integer digits. Use appropriate factor (B, KB, MB, GB).                     | 0.9 * min data | 1.1 * max data |
| **Step Throughput** | Log 10 | exponential notation ($10^i$).                                                                                                             | 0.9 * min data | 1.1 * max data |
| **Step Duration**   | Log 10 | exponential notation ($10^i$). Use appropriate factor (ns, us, ms, s).                                                                     | 0.9 * min data | 1.1 * max data |

---

## General Requirements

- **Consistency**: Line styles must be consistent for specific steps and variants across all plots.
- **Topology Markers**: Vertical lines marking topology boundaries (single threaded, single socket, single node) on the
  X-axis.
- **Ideal Lines**:
    - Strong Scaling: Ideal speedup line.
    - Weak Scaling: Ideal efficiency line.
    - **Constraint**: Ideal lines must not influence the X-axis limits.
- **Legend**: Unified legend for side-by-side plots with no duplicate entries.
