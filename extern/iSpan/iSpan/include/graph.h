#pragma once

#include "util.h"
#include "wtime.h"
#include "graph.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <map>
#include <mpi.h>

struct graph
{
  unsigned int*   fw_beg_pos = nullptr;
  vertex_t* fw_csr     = nullptr;
  unsigned int*   bw_beg_pos = nullptr;
  vertex_t* bw_csr     = nullptr;
  index_t   vert_count = 0;
  unsigned int    edge_count = 0;
  vertex_t* src_list   = nullptr;
  index_t   src_count  = 0;

  graph()  = default;
  ~graph() = default;
  graph(char const* fw_beg_file, char const* fw_csr_file, char const* bw_beg_file, char const* bw_csr_file)
  {
    double const         tm = wtime();
    typedef unsigned int index_tt;
    typedef int          vertex_tt;
    vert_count = fsize(fw_beg_file) / sizeof(index_tt) - 1;
    edge_count = fsize(fw_csr_file) / sizeof(vertex_tt);
    FILE* file = fopen(fw_beg_file, "rb");
    if (file == nullptr) {
      std::cout << fw_beg_file << " cannot open\n";
      exit(-1);
    }
    auto* tmp_beg_pos = new index_tt[vert_count + 1];
    index_tt  ret         = fread(tmp_beg_pos, sizeof(index_tt), vert_count + 1, file);
    assert(ret == vert_count + 1);
    fclose(file);
    file = fopen(fw_csr_file, "rb");
    if (file == nullptr) {
      std::cout << fw_csr_file << " cannot open\n";
      exit(-1);
    }
    auto* tmp_csr = new vertex_tt[edge_count];
    ret                = fread(tmp_csr, sizeof(vertex_tt), edge_count, file);
    assert(ret == edge_count);
    fclose(file);
    fw_beg_pos = new index_tt[vert_count + 1];
    fw_csr     = new vertex_t[edge_count];
    for (index_t i = 0; i < vert_count + 1; ++i)
      fw_beg_pos[i] = (index_tt)tmp_beg_pos[i];
    for (index_tt i = 0; i < edge_count; ++i)
      fw_csr[i] = (vertex_t)tmp_csr[i];
    file = fopen(bw_beg_file, "rb");
    if (file == nullptr) {
      std::cout << bw_beg_file << " cannot open\n";
      exit(-1);
    }
    tmp_beg_pos = new index_tt[vert_count + 1];
    ret         = fread(tmp_beg_pos, sizeof(index_tt), vert_count + 1, file);
    assert(ret == vert_count + 1);
    fclose(file);
    file = fopen(bw_csr_file, "rb");
    if (file == nullptr) {
      std::cout << bw_csr_file << " cannot open\n";
      exit(-1);
    }
    tmp_csr = new vertex_tt[edge_count];
    ret     = fread(tmp_csr, sizeof(vertex_tt), edge_count, file);
    assert(ret == edge_count);
    fclose(file);
    bw_beg_pos = new index_tt[vert_count + 1];
    bw_csr     = new vertex_t[edge_count];
    for (index_tt i = 0; i < vert_count + 1; ++i)
      bw_beg_pos[i] = tmp_beg_pos[i];
    for (index_tt i = 0; i < edge_count; ++i)
      bw_csr[i] = tmp_csr[i];
    delete[] tmp_beg_pos;
    delete[] tmp_csr;
    std::cout << "Graph load (success): " << vert_count << " verts, " << edge_count << " edges " << wtime() - tm << " second(s)\n";
  }
};

inline graph*
graph_load(char const* fw_beg_file, char const* fw_csr_file, char const* bw_beg_file, char const* bw_csr_file, double* avg_time)
{
  auto* g = new graph(fw_beg_file, fw_csr_file, bw_beg_file, bw_csr_file);
  for (index_t i = 0; i < 15; ++i)
    avg_time[i] = 0.0;
  return g;
}

inline void
get_scc_result(index_t const* scc_id, index_t vert_count)
{
  index_t                    size_1        = 0;
  index_t                    size_2        = 0;
  index_t                    size_3_type_1 = 0;
  index_t                    size_3_type_2 = 0;
  index_t                    largest       = 0;
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
  printf("\nResult:\nlargest, %d\ntrimmed size_1, %d\ntrimmed size_2, %d\ntrimmed size_3, %d\nothers, %lu\ntotal, %lu\n", largest, size_1, size_2 / 2, size_3_type_1 / 3 + size_3_type_2 / 3, mp.size(), (1 + size_1 + size_2 / 2 + size_3_type_1 / 3 + size_3_type_2 / 3 + mp.size()));
}

inline void
print_time_result(index_t run_times, double* avg_time)
{
  if (run_times > 0) {
    for (index_t i = 0; i < 15; ++i)
      avg_time[i] = avg_time[i] / run_times * 1000;
    printf("\nAverage Time Consumption for Running %d Times (ms)\n", run_times);
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
