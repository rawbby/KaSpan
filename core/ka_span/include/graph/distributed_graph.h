#pragma once

#include "../index.h"

#include <cassert>
#include <span>

namespace ka_span {

template<is_index GV, is_index GI, is_index LI>
struct distributed_graph
{
  using global_vertex_type = GV;
  using global_index_type  = GI;
  using local_index_type   = LI;

  global_vertex_type  num_global_vertices;
  global_index_type   num_global_edges;
  global_vertex_type  num_local_vertices;
  local_index_type    num_local_edges;
  global_vertex_type  skipped_vertices;
  local_index_type*   csr_offset_buffer;
  global_vertex_type* csr_edge_buffer;

  std::span<local_index_type> operator[](global_vertex_type global_index)
  {
    assert(global_index >= skipped_vertices);
    assert(global_index - skipped_vertices < num_local_vertices);
    return { csr_edge_buffer + csr_offset_buffer[global_index - skipped_vertices],
             csr_edge_buffer + csr_offset_buffer[global_index - skipped_vertices + 1] };
  }
};

}
