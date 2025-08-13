# Distributed Graph Format and Utilities

> Definitions and notation:
> - Node IDs are `0..n-1` (contiguous)
> - `n` — number of nodes
> - `m` — number of edges
> - `i`, `j`, `k` — integer indices used as Node IDs
> - `u`, `v` — endpoints of an edge u → v
> - head — CSR offsets array of length `n+1`; `head[i]` is the start of node `i`’s adjacency; `head[i+1]` is the end
    (`head[0] = 0`, `head[n] = m`)
> - csr — CSR adjacency array of length `m`; The neighbors of node `i` occupy `csr[head[i] ... head[i+1])`

This project defines compact, portable formats for both on-disk and in RAM, plus a small set of utilities for working
with very large directed graphs (preserving self-loops and duplicate edges) in distributed environments. The core is a
CSR-based representation for both forward and backward graphs and a manifest-driven loader that supports many
partitioning schemes (e.g., contiguous, cyclic, explicit).

> [!IMPORTANT]
> Node IDs must be zero-based and contiguous (0..n‑1). If IDs are sparse, they are treated as if n = max(ID) + 1. The
> head file is a CSR offsets array of length n+1: head[i] is the start of node i’s adjacency; head[i+1] is the end.
> head[0] = 0 and head[n] = m.
>
> Consequences of sparse/non‑contiguous IDs:
> - Head storage scales with n = max(ID) + 1, even if many IDs are unused.
> - CSR element width is determined by the maximum node ID, potentially forcing wider elements and larger files.
> - Range‑based partitioning splits by node index; many isolated nodes can lead to uneven edge/workload distribution.

## Vision and scope

The framework provides a single, manifest-governed binary schema for forward and backward CSR graphs and a set of
utilities to convert edge lists, load partitions in MPI-like setups, and traverse adjacency efficiently. Datasets can
exceed RAM; conversion is designed to be external-memory friendly, and loading brings in only a partition of the graph
(which must fit in RAM). All fallible operations return Result; no exceptions are thrown. Storage uses fixed-width,
little-endian integers, interpreted as raw bytes to allow compact encodings and optional in-memory widening.

## Requirements

- Platform: Linux (POSIX) due to `mmap` and stuff like paging utilities
- Language: C++20
- Dependencies:
    - STXXL (used by the converter for external sorting)
- Integration: header-only — add the include directory to your project or copy the headers into your source tree.

## On-disk schema

### Converter (edge list → manifest + CSR)

Reads lines `u v`, externally sorts edges in O(m log m), and writes the four binary CSR files in a single streaming pass
per direction (once for the forward and once for the backward graph). Chooses minimal byte widths per file and records
them in the manifest. All entries are little‑endian, independent of host endianness. Per‑node adjacency is sorted by
definition. inally stores the manifest file.

> [!NOTICE]
> Empty lines or lines starting with “%” are ignored.

### Manifest (text, key–value)

Each non‑empty line is either a comment starting with “%” or a key followed by a single space and the value. Paths are
resolved relative to the manifest’s directory.

Required keys:

- schema.version: unsigned integer
- graph.code: short code used as basename
- graph.name: human-readable name
- graph.node_count: n
- graph.edge_count: m
- graph.contains_self_loops: bool
- graph.contains_duplicate_edges: bool
- graph.head.bytes
- graph.csr.bytes
- fw.head.path
- fw.csr.path
- bw.head.path
- bw.csr.path

### Head (binary)

CSR offsets array (n+1 elements), stored as fixed‑width little‑endian unsigned integers. The width is given by
fw.head.bytes or bw.head.bytes. No file header.

### CSR (binary)

Like Head, a fixed‑width little‑endian unsigned‑integer array. The CSR adjacency array has m elements. The width is
given by fw.csr.bytes or bw.csr.bytes. No file header.

## In-RAM schema

### Loading in distributed settings

Loading uses two buffer kinds:

- File buffer: lazily maps a file into memory via mmap (read-only, non-owning).
- Local buffer: preallocates and persistently holds data in RAM (owning).

Strategy (per rank/partition):

1. Parse the manifest file.
2. Open file buffers for the relevant head and CSR files.
3. Allocate local buffers sized to the partition elements.
4. Use global offsets from the head file buffer to locate the CSR ranges for nodes owned by this rank; stream those
   ranges into the local CSR buffer. While streaming, build the local head buffer as offsets into the local CSR buffer.
5. Release file-backed mappings; keep only the local buffers.

Notes:

- Degrees are derived: `degree(i) = head[i+1] - head[i]`.
- Optional widening: on load, elements may be widened to the next power-of-two byte width greater than or equal to the
  on-disk width (e.g., 3 → 4, 5–8 → 8).

### Distributed Graph

The DistributedGraph holds:

- local forward and backward buffers (head + CSR)
- the partition descriptor used to decide ownership and index translation

Access is restricted to local data. The graph provides adjacency and degree queries for locally owned nodes and allows
checking whether a node is local; it does not fetch or expose remote data.
