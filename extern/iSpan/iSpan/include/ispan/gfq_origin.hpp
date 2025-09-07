#pragma once

#include <ispan/util.hpp>

#include <iostream>

static void
gfq_origin(
  index_t const  n,
  index_t const* scc_id,
  index_t*       frontier_queue,

  int const*      fw_beg_pos,
  vertex_t const* fw_csr,
  int const*      bw_beg_pos,
  vertex_t const* bw_csr,

  vertex_t* sub_fw_beg,
  vertex_t* sub_fw_csr,
  vertex_t* sub_bw_beg,
  vertex_t* sub_bw_csr,

  vertex_t* front_comm,
  int*      work_comm,
  vertex_t  world_rank,
  vertex_t* vert_map)
{
  index_t index       = 0;
  index_t fw_edge_num = 0;
  index_t bw_edge_num = 0;
  for (vertex_t vert_id = 0; vert_id < n; ++vert_id) {
    if (scc_id[vert_id] == 0) {
      frontier_queue[index] = vert_id;
      vert_map[vert_id]     = index;
      index++;
    }
  }
  for (vertex_t v_index = 0; v_index < index; ++v_index) {
    vertex_t vert_id = frontier_queue[v_index];
    int      my_beg  = fw_beg_pos[vert_id];
    int      my_end  = fw_beg_pos[vert_id + 1];
    for (int w_index = my_beg; w_index < my_end; ++w_index) {
      vertex_t w = fw_csr[w_index];
      if (scc_id[w] == 0) {
        sub_fw_csr[fw_edge_num] = vert_map[w];
        fw_edge_num++;
      }
    }
    sub_fw_beg[v_index + 1] = fw_edge_num;
    my_beg                  = bw_beg_pos[vert_id];
    my_end                  = bw_beg_pos[vert_id + 1];
    for (int w_index = my_beg; w_index < my_end; ++w_index) {
      vertex_t w = bw_csr[w_index];
      if (scc_id[w] == 0) {
        sub_bw_csr[bw_edge_num] = vert_map[w];
        bw_edge_num++;
      }
    }
    sub_bw_beg[v_index + 1] = bw_edge_num;
  }
  front_comm[world_rank] = index;
  work_comm[world_rank]  = fw_edge_num;
  std::cout << "sub v_count, " << front_comm[world_rank] << ", sub e_count, " << work_comm[world_rank] << "," << bw_edge_num << "\n";
}
