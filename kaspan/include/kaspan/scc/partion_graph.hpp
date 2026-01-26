#pragma once

#include <kaspan/graph/base.hpp>
#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/graph_part.hpp>

namespace kaspan {

/// from a global_graph get the degree of a partition
template<part_view_concept Part>
  requires(Part::continuous)
auto
partition_local_m(
  graph_view g,
  Part       part) -> index_t
{
  return part.local_n() > 0 ? g.head[part.end()] - g.head[part.begin()] : 0;
}

/// from a global_graph get the degree of a partition
template<part_view_concept Part>
  requires(!Part::continuous)
auto
partition_local_m(
  graph_view g,
  Part       part) -> index_t
{
  auto const local_n = part.local_n();

  index_t m = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    m += g.outdegree(part.to_global(k));
  }
  return m;
}

/// from a global_graph get the degree of a partition
template<part_view_concept Part>
auto
partition_local_m(
  bidi_graph_view bg,
  Part            part)
{
  auto const local_fw_m = partition_local_m(bg.fw_view(), part);
  auto const local_bw_m = partition_local_m(bg.bw_view(), part);
  return PACK(local_fw_m, local_bw_m);
}

template<part_view_concept Part>
auto
partition(
  graph_view g,
  Part       part) -> graph_part<Part>
{
  g.debug_validate();
  auto const local_n = part.local_n();

  auto const local_m = partition_local_m(g, part);
  auto       gp      = graph_part<Part>{ std::move(part), local_m };

  index_t pos = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    gp.head[k] = pos;
    g.each_v(gp.part.to_global(k), [&](auto v) { gp.csr[pos++] = v; });
  }
  if (local_n > 0) gp.head[local_n] = pos;

  gp.debug_validate();
  return gp;
}

template<part_view_concept Part>
auto
partition(
  bidi_graph_view bg,
  Part            part) -> bidi_graph_part<Part>
{
  bg.debug_validate();
  auto const local_n                  = part.local_n();
  auto const [local_fw_m, local_bw_m] = partition_local_m(bg, part);

  auto bgp = bidi_graph_part<Part>{ std::move(part), local_fw_m, local_bw_m };

  index_t fw_offset = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    bgp.fw.head[k] = fw_offset;
    bg.each_v(bgp.part.to_global(k), [&](auto v) { bgp.fw.csr[fw_offset++] = v; });
  }
  if (local_n > 0) bgp.fw.head[local_n] = fw_offset;

  index_t bw_offset = 0;
  for (vertex_t k = 0; k < local_n; ++k) {
    bgp.bw.head[k] = bw_offset;
    bg.each_bw_v(bgp.part.to_global(k), [&](auto v) { bgp.bw.csr[bw_offset++] = v; });
  }
  if (local_n > 0) bgp.bw.head[local_n] = bw_offset;

  bgp.debug_validate();
  return bgp;
}

} // namespace kaspan
