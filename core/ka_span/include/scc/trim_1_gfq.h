#pragma once

#include "../../src/util.h"
#include <vector>

inline void
trim_1_first(
  std::vector<index_t> scc_id,
  const std::vector<long_t>& fw_beg_pos,
  const std::vector<long_t>& bw_beg_pos,
  index_t vert_beg,
  index_t vert_end)
{
  for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (fw_beg_pos[vert_id + 1] - fw_beg_pos[vert_id] == 0) {
      scc_id[vert_id] = -1;
    } else {
      if (bw_beg_pos[vert_id + 1] - bw_beg_pos[vert_id] == 0) {
        scc_id[vert_id] = -1;
      }
    }
  }
}

inline void
trim_1_normal(
  std::vector<index_t>& scc_id,
  const std::vector<long_t>& fw_beg_pos,
  const std::vector<long_t>& bw_beg_pos,
  index_t vert_beg,
  index_t vert_end,
  const std::vector<vertex_t>& fw_csr,
  const std::vector<vertex_t>& bw_csr)
{
  for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (scc_id[vert_id] == 0) {
      long_t my_beg = fw_beg_pos[vert_id];
      long_t my_end = fw_beg_pos[vert_id + 1];
      index_t out_degree = 0;
      for (; my_beg < my_end; ++my_beg) {
        index_t w = fw_csr[my_beg];
        if (scc_id[w] == 0 && w != vert_id) {
          out_degree = 1;
          break;
        }
      }
      if (out_degree == 0) {
        scc_id[vert_id] = -1;
        continue;
      }
      index_t in_degree = 0;
      my_beg = bw_beg_pos[vert_id];
      my_end = bw_beg_pos[vert_id + 1];
      for (; my_beg < my_end; ++my_beg) {
        index_t w = bw_csr[my_beg];
        if (scc_id[w] == 0 && w != vert_id) {
          in_degree = 1;
          break;
        }
      }
      if (in_degree == 0) {
        scc_id[vert_id] = -1;
      }
    }
  }
}

static void
gfq_origin(
  const index_t vert_count,
  const std::vector<index_t>& scc_id,
  std::vector<index_t>& frontier_queue,
  const std::vector<long_t>& fw_beg_pos,
  const std::vector<vertex_t>& fw_csr,
  const std::vector<long_t>& bw_beg_pos,
  const std::vector<vertex_t>& bw_csr,
  std::vector<vertex_t>& sub_fw_beg,
  std::vector<vertex_t>& sub_fw_csr,
  std::vector<vertex_t>& sub_bw_beg,
  std::vector<vertex_t>& sub_bw_csr,
  std::vector<vertex_t>& front_comm,
  std::vector<long_t>& work_comm,
  vertex_t world_rank,
  std::vector<vertex_t>& vert_map)
{
  index_t index = 0;
  index_t fw_edge_num = 0;
  index_t bw_edge_num = 0;
  for (vertex_t vert_id = 0; vert_id < vert_count; ++vert_id) {
    if (scc_id[vert_id] == 0) {
      frontier_queue[index] = vert_id;
      vert_map[vert_id] = index;
      index++;
    }
  }

  for (vertex_t v_index = 0; v_index < index; ++v_index) {
    const vertex_t vert_id = frontier_queue[v_index];

    long_t my_beg = fw_beg_pos[vert_id];
    long_t my_end = fw_beg_pos[vert_id + 1];
    for (long_t w_index = my_beg; w_index < my_end; ++w_index) {
      const vertex_t w = fw_csr[w_index];
      if (scc_id[w] == 0) {
        sub_fw_csr[fw_edge_num] = vert_map[w];
        fw_edge_num++;
      }
    }
    sub_fw_beg[v_index + 1] = fw_edge_num;

    my_beg = bw_beg_pos[vert_id];
    my_end = bw_beg_pos[vert_id + 1];
    for (long_t w_index = my_beg; w_index < my_end; ++w_index) {
      const vertex_t w = bw_csr[w_index];
      if (scc_id[w] == 0) {
        sub_bw_csr[bw_edge_num] = vert_map[w];
        bw_edge_num++;
      }
    }
    sub_bw_beg[v_index + 1] = bw_edge_num;
  }

  front_comm[world_rank] = index;
  work_comm[world_rank] = fw_edge_num;
  std::cout << "sub v_count, " << front_comm[world_rank] << ", sub e_count, " << work_comm[world_rank] << "," << bw_edge_num << "\n";
}

// inline void
// trim_1_first_gfq(
//   index_t* scc_id,
//   const index_t* fw_beg_pos,
//   const index_t* bw_beg_pos,
//   index_t vert_beg,
//   index_t vert_end,
//   std::vector<index_t>& frontier_queue,
//   index_t* thread_bin,
//   index_t* prefix_sum,
//   index_t tid)
// {
//   index_t thread_bin_size = 0;
//
//   for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
//     if (fw_beg_pos[vert_id + 1] - fw_beg_pos[vert_id] == 0) {
//       scc_id[vert_id] = -1;
//     } else if (bw_beg_pos[vert_id + 1] - bw_beg_pos[vert_id] == 0) {
//       scc_id[vert_id] = -1;
//     } else {
//       thread_bin_size++;
//     }
//   }
//
//   thread_bin[tid] = thread_bin_size;
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
//
// #pragma omp barrier
//
//   vertex_t start_pos = prefix_sum[tid];
//
//   for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
//     if (scc_id[vert_id] == 0) {
//       frontier_queue[start_pos++] = vert_id;
//     }
//   }
// }

// inline void
// trim_1_from_fq(
//   index_t* scc_id,
//   const index_t* fw_beg_pos,
//   const index_t* bw_beg_pos,
//   index_t vert_beg,
//   index_t vert_end,
//   const vertex_t* fw_csr,
//   const vertex_t* bw_csr,
//   const index_t* small_queue)
// {
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = small_queue[fq_vert_id];
//     if (scc_id[vert_id] == 0) {
//       index_t my_beg = fw_beg_pos[vert_id];
//       index_t my_end = fw_beg_pos[vert_id + 1];
//       index_t out_degree = 0;
//       for (; my_beg < my_end; ++my_beg) {
//         index_t w = fw_csr[my_beg];
//         if (scc_id[w] == 0 && w != vert_id) {
//           out_degree = 1;
//           break;
//         }
//       }
//       if (out_degree == 0) {
//         scc_id[vert_id] = -1;
//         continue;
//       }
//       index_t in_degree = 0;
//       my_beg = bw_beg_pos[vert_id];
//       my_end = bw_beg_pos[vert_id + 1];
//       for (; my_beg < my_end; ++my_beg) {
//         index_t w = bw_csr[my_beg];
//         if (scc_id[w] == 0 && w != vert_id) {
//           in_degree = 1;
//           break;
//         }
//       }
//       if (in_degree == 0) {
//         scc_id[vert_id] = -1;
//       }
//     }
//   }
// }

// inline void
// trim_1_normal_only_size(
//   index_t* scc_id,
//   const index_t* fw_beg_pos,
//   const index_t* bw_beg_pos,
//   index_t vert_beg,
//   index_t vert_end,
//   const vertex_t* fw_csr,
//   const vertex_t* bw_csr,
//   const index_t thread_count,
//   index_t* thread_bin,
//   index_t* prefix_sum,
//   index_t tid)
// {
//
//   index_t thread_bin_size = 0;
//   for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
//     if (scc_id[vert_id] == 0) {
//       index_t my_beg = fw_beg_pos[vert_id];
//       index_t my_end = fw_beg_pos[vert_id + 1];
//       index_t out_degree = 0;
//       for (; my_beg < my_end; ++my_beg) {
//         index_t w = fw_csr[my_beg];
//         if (scc_id[w] == 0 && w != vert_id) {
//           out_degree = 1;
//           break;
//         }
//       }
//       if (out_degree == 0) {
//         scc_id[vert_id] = -1;
//         thread_bin_size++;
//         continue;
//       }
//       index_t in_degree = 0;
//       my_beg = bw_beg_pos[vert_id];
//       my_end = bw_beg_pos[vert_id + 1];
//       for (; my_beg < my_end; ++my_beg) {
//         index_t w = bw_csr[my_beg];
//         if (scc_id[w] == 0 && w != vert_id) {
//           in_degree = 1;
//           break;
//         }
//       }
//       if (in_degree == 0) {
//         scc_id[vert_id] = -1;
//         thread_bin_size++;
//         continue;
//       }
//     }
//   }
//
//   thread_bin[tid] = thread_bin_size;
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
// }

// inline void
// trim_1_normal_gfq(
//   index_t* scc_id,
//   const index_t* fw_beg_pos,
//   const index_t* bw_beg_pos,
//   index_t vert_beg,
//   index_t vert_end,
//   const vertex_t* fw_csr,
//   const vertex_t* bw_csr,
//   const index_t thread_count,
//   index_t* frontier_queue,
//   index_t* thread_bin,
//   index_t* prefix_sum,
//   index_t tid)
// {
//
//   index_t thread_bin_size = 0;
//   for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
//     if (scc_id[vert_id] == 0) {
//       index_t my_beg = fw_beg_pos[vert_id];
//       index_t my_end = fw_beg_pos[vert_id + 1];
//       index_t out_degree = 0;
//       for (; my_beg < my_end; ++my_beg) {
//         index_t w = fw_csr[my_beg];
//         if (scc_id[w] == 0 && w != vert_id) {
//           out_degree = 1;
//           break;
//         }
//       }
//       if (out_degree == 0) {
//         scc_id[vert_id] = -1;
//         continue;
//       }
//       index_t in_degree = 0;
//       my_beg = bw_beg_pos[vert_id];
//       my_end = bw_beg_pos[vert_id + 1];
//       for (; my_beg < my_end; ++my_beg) {
//         index_t w = bw_csr[my_beg];
//         if (scc_id[w] == 0 && w != vert_id) {
//           in_degree = 1;
//           break;
//         }
//       }
//       if (in_degree == 0) {
//         scc_id[vert_id] = -1;
//         continue;
//       }
//
//       thread_bin_size++;
//     }
//   }
//
//   thread_bin[tid] = thread_bin_size;
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
//
//   vertex_t start_pos = prefix_sum[tid];
//   for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
//     if (scc_id[vert_id] == 0) {
//       frontier_queue[start_pos++] = vert_id;
//     }
//   }
// }

// inline void
// trim_1_from_fq_gfq(
//   index_t* scc_id,
//   const index_t* fw_beg_pos,
//   const index_t* bw_beg_pos,
//   index_t vert_beg,
//   index_t vert_end,
//   const vertex_t* fw_csr,
//   const vertex_t* bw_csr,
//   const index_t thread_count,
//   index_t* frontier_queue,
//   index_t* thread_bin,
//   index_t* prefix_sum,
//   index_t tid,
//   index_t* temp_queue)
// {
//
//   index_t thread_bin_size = 0;
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = frontier_queue[fq_vert_id];
//     if (scc_id[vert_id] == 0) {
//       index_t my_beg = fw_beg_pos[vert_id];
//       index_t my_end = fw_beg_pos[vert_id + 1];
//       index_t out_degree = 0;
//       for (; my_beg < my_end; ++my_beg) {
//         index_t w = fw_csr[my_beg];
//         if (scc_id[w] == 0 && w != vert_id) {
//           out_degree = 1;
//           break;
//         }
//       }
//       if (out_degree == 0) {
//         scc_id[vert_id] = -1;
//         continue;
//       }
//       index_t in_degree = 0;
//       my_beg = bw_beg_pos[vert_id];
//       my_end = bw_beg_pos[vert_id + 1];
//       for (; my_beg < my_end; ++my_beg) {
//         index_t w = bw_csr[my_beg];
//         if (scc_id[w] == 0 && w != vert_id) {
//           in_degree = 1;
//           break;
//         }
//       }
//       if (in_degree == 0) {
//         scc_id[vert_id] = -1;
//         continue;
//       }
//       thread_bin_size++;
//     }
//   }
//   thread_bin[tid] = thread_bin_size;
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
//
//   vertex_t start_pos = prefix_sum[tid];
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = frontier_queue[fq_vert_id];
//     if (scc_id[vert_id] == 0) {
//       temp_queue[start_pos++] = vert_id;
//     }
//   }
// #pragma omp barrier
//   for (index_t i = prefix_sum[tid]; i < prefix_sum[tid] + thread_bin[tid]; ++i) {
//     frontier_queue[i] = temp_queue[i];
//   }
// }

// static void
// get_queue(
//   const vertex_t* thread_queue,
//   const vertex_t* thread_bin,
//   index_t* prefix_sum,
//   index_t tid,
//   vertex_t* temp_queue)
// {
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
// #pragma omp barrier
//
//   vertex_t start_pos = prefix_sum[tid];
//   for (vertex_t vert_id = start_pos; vert_id < start_pos + thread_bin[tid]; ++vert_id) {
//     temp_queue[vert_id] = thread_queue[vert_id - start_pos];
//   }
// }

// static void
// generate_frontier_queue(
//   const index_t vert_count,
//   const index_t* scc_id,
//   const index_t thread_count,
//   index_t* frontier_queue,
//   index_t* thread_bin,
//   index_t* prefix_sum,
//   index_t vert_beg,
//   index_t vert_end,
//   index_t tid)
// {
//
//   index_t thread_bin_size = 0;
//   for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
//     if (scc_id[vert_id] == 0) {
//       thread_bin_size++;
//     }
//   }
//   thread_bin[tid] = thread_bin_size;
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
//
//   vertex_t start_pos = prefix_sum[tid];
//   for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
//     if (scc_id[vert_id] == 0) {
//       frontier_queue[start_pos++] = vert_id;
//     }
//   }
// }

// static void
// gfq_from_queue(
//   const index_t vert_count,
//   const index_t* scc_id,
//   const index_t thread_count,
//   index_t* small_queue,
//   index_t* thread_bin,
//   index_t* prefix_sum,
//   index_t vert_beg,
//   index_t vert_end,
//   index_t tid,
//   index_t* temp_queue)
// {
//
//   index_t thread_bin_size = 0;
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = small_queue[fq_vert_id];
//     if (scc_id[vert_id] == 0) {
//       thread_bin_size++;
//     }
//   }
//   thread_bin[tid] = thread_bin_size;
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
//
//   vertex_t start_pos = prefix_sum[tid];
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = small_queue[fq_vert_id];
//     if (scc_id[vert_id] == 0) {
//       temp_queue[start_pos++] = vert_id;
//     }
//   }
// #pragma omp barrier
//
//   for (index_t i = prefix_sum[tid]; i < prefix_sum[tid] + thread_bin[tid]; ++i) {
//     small_queue[i] = temp_queue[i];
//   }
// }

// static void
// bw_gfq_from_fw(
//   const index_t* fw_sa,
//   const index_t thread_count,
//   const index_t* small_queue,
//   index_t* thread_bin,
//   index_t* prefix_sum,
//   index_t vert_beg,
//   index_t vert_end,
//   index_t tid,
//   index_t* temp_queue)
// {
//
//   index_t thread_bin_size = 0;
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = small_queue[fq_vert_id];
//     if (fw_sa[vert_id] != -1) {
//       thread_bin_size++;
//     }
//   }
//   thread_bin[tid] = thread_bin_size;
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
//
// #pragma omp barrier
//
//   vertex_t start_pos = prefix_sum[tid];
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = small_queue[fq_vert_id];
//     if (fw_sa[vert_id] != -1) {
//       temp_queue[start_pos++] = vert_id;
//     }
//   }
// }

// static void
// gfq_fw_bw_from_queue(
//   const index_t* sa,
//   const index_t thread_count,
//   const index_t* small_queue,
//   index_t* thread_bin,
//   index_t* prefix_sum,
//   index_t vert_beg,
//   index_t vert_end,
//   index_t tid,
//   index_t* temp_queue)
// {
//
//   index_t thread_bin_size = 0;
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = small_queue[fq_vert_id];
//     if (sa[vert_id] == -1) {
//       thread_bin_size++;
//     }
//   }
//   thread_bin[tid] = thread_bin_size;
//
//   prefix_sum[tid] = 0;
// #pragma omp barrier
//   for (index_t i = 0; i < tid; ++i) {
//     prefix_sum[tid] += thread_bin[i];
//   }
//
//   vertex_t start_pos = prefix_sum[tid];
//   for (vertex_t fq_vert_id = vert_beg; fq_vert_id < vert_end; ++fq_vert_id) {
//     vertex_t vert_id = small_queue[fq_vert_id];
//     if (sa[vert_id] == -1) {
//       temp_queue[start_pos++] = vert_id;
//     }
//   }
// }
