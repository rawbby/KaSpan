#pragma once

#include "util.h"
#include "wtime.h"
#include "graph.h"

#include <map>
#include <vector>

inline graph*
graph_load(
  const char* fw_beg_file,
  const char* fw_csr_file,
  const char* bw_beg_file,
  const char* bw_csr_file,
  std::vector<double>& avg_time)
{
  auto* g = new graph(fw_beg_file,
                      fw_csr_file,
                      bw_beg_file,
                      bw_csr_file);

  avg_time.assign(15, 0.0);
  return g;
}

void
scc_detection(
  const graph* g,
  int alpha,
  index_t thread_count,
  std::vector<double>& avg_time,
  int world_rank,
  int world_size,
  int run_time,
  std::vector<vertex_t>& assignment);

inline void
get_scc_result(
  const std::vector<index_t>& scc_id,
  index_t vert_count)
{
  index_t size_1 = 0;
  index_t size_2 = 0;
  index_t size_3_type_1 = 0;
  index_t size_3_type_2 = 0;
  index_t largest = 0;
  std::map<index_t, index_t> mp;
  for (index_t i = 0; i < vert_count + 1; ++i) {
    if (scc_id[i] == 1)
      largest++;
    else if (scc_id[i] == -1)
      size_1++;
    else if (scc_id[i] == -2)
      size_2++;
    else if (scc_id[i] == -3) {
      size_3_type_1++;
    } else if (scc_id[i] == -4)
      size_3_type_2++;
    else {
      mp[scc_id[i]]++;
    }
  }
  printf("\nResult:\nlargest, %lu\ntrimmed size_1, %lu\ntrimmed size_2, %lu\ntrimmed size_3, %lu\nothers, %lu\ntotal, %lu\n", largest, size_1, size_2 / 2, size_3_type_1 / 3 + size_3_type_2 / 3, mp.size(), (1 + size_1 + size_2 / 2 + size_3_type_1 / 3 + size_3_type_2 / 3 + mp.size()));
}

inline void
print_time_result(
  index_t run_times,
  std::vector<double>& avg_time)
{
  if (run_times > 0) {
    for (double& t : avg_time)
      t = t * 1000.0 / static_cast<double>(run_times);
    printf("\nAverage Time Consumption for Running %lu Times (ms)\n", run_times);
    printf("Trim, %.3lf\n", avg_time[0]);
    printf("Elephant SCC, %.3lf\n", avg_time[1]);

    printf("Mice SCC, %.3lf\n", avg_time[2]);
    printf("Total time, %.3lf\n", avg_time[3]);

    printf("\n------Details------\n");
    printf("Trim size_1, %.3lf\n", avg_time[4]);
    printf("Pivot selection, %.3lf\n", avg_time[6]);
    printf("FW BFS, %.3lf\n", avg_time[7]);
    printf("BW BFS, %.3lf\n", avg_time[8]);
    printf("Trim size_2, %.3lf\n", avg_time[5]);
    printf("Trim size_3, %.3lf\n", avg_time[13]);

    printf("Wcc, %.3lf\n", avg_time[9]);
    printf("Mice fw-bw, %.3lf\n", avg_time[10]);
  }
}

inline index_t
pivot_selection(
  const std::vector<index_t>& scc_id,
  const std::vector<long_t>& fw_beg_pos,
  const std::vector<long_t>& bw_beg_pos,
  index_t vert_beg,
  index_t vert_end,
  index_t tid)
{
  index_t max_pivot_thread = 0;
  index_t max_degree_thread = 0;

  for (vertex_t vert_id = vert_beg; vert_id < vert_end; ++vert_id) {
    if (scc_id[vert_id] == 0) {
      const long_t out_degree = fw_beg_pos[vert_id + 1] - fw_beg_pos[vert_id];
      const long_t in_degree = bw_beg_pos[vert_id + 1] - bw_beg_pos[vert_id];
      const long_t degree_mul = out_degree * in_degree;

      if (degree_mul > max_degree_thread) {
        max_degree_thread = degree_mul;
        max_pivot_thread = vert_id;
      }
    }
  }

  const index_t max_pivot = max_pivot_thread;
  const index_t max_degree = max_degree_thread;

  if (tid == 0)
    printf("max_pivot, %lu, max_degree, %lu\n", max_pivot, max_degree);

  return max_pivot;
}
