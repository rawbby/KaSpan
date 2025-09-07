#pragma once

#include <ispan/util.hpp>

#include <cassert>
#include <cstdio>
#include <iostream>

struct graph
{
  int*      fw_beg_pos = nullptr;
  vertex_t* fw_csr     = nullptr;
  int*      bw_beg_pos = nullptr;
  vertex_t* bw_csr     = nullptr;

  index_t vert_count = 0;
  int     edge_count = 0;

  graph() = default;

  ~graph()
  {
    delete[] fw_beg_pos;
    delete[] fw_csr;
    delete[] bw_beg_pos;
    delete[] bw_csr;

    fw_beg_pos = nullptr;
    fw_csr     = nullptr;
    bw_beg_pos = nullptr;
    bw_csr     = nullptr;

    vert_count = 0;
    edge_count = 0;
  }

  graph(char const* fw_beg_file, char const* fw_csr_file, char const* bw_beg_file, char const* bw_csr_file)
  {
    double const tm = wtime();
    vert_count      = static_cast<index_t>(fsize(fw_beg_file) / sizeof(index_t) - 1);
    edge_count      = static_cast<int>(fsize(fw_csr_file) / sizeof(vertex_t));

    FILE* file = fopen(fw_beg_file, "rb");
    if (file == nullptr) {
      std::cout << fw_beg_file << " cannot open\n";
      exit(-1);
    }

    {
      auto* tmp_beg_pos = new index_t[vert_count + 1]{};
      SCOPE_GUARD(delete[] tmp_beg_pos);

      auto ret = fread(tmp_beg_pos, sizeof(index_t), vert_count + 1, file);
      assert(ret == vert_count + 1);

      fclose(file);
      file = fopen(fw_csr_file, "rb");
      if (file == nullptr) {
        std::cout << fw_csr_file << " cannot open\n";
        exit(-1);
      }

      auto* tmp_csr = new vertex_t[edge_count]{};
      SCOPE_GUARD(delete[] tmp_csr);

      ret = fread(tmp_csr, sizeof(vertex_t), edge_count, file);
      assert(ret == edge_count);

      fclose(file);
      fw_beg_pos = new index_t[vert_count + 1]{};
      fw_csr     = new vertex_t[edge_count]{};
      for (index_t i = 0; i < vert_count + 1; ++i)
        fw_beg_pos[i] = tmp_beg_pos[i];
      for (index_t i = 0; i < edge_count; ++i)
        fw_csr[i] = tmp_csr[i];
      file = fopen(bw_beg_file, "rb");
      if (file == nullptr) {
        std::cout << bw_beg_file << " cannot open\n";
        exit(-1);
      }
    }

    {
      auto* tmp_beg_pos = new index_t[vert_count + 1]{};
      SCOPE_GUARD(delete[] tmp_beg_pos);

      auto ret = fread(tmp_beg_pos, sizeof(index_t), vert_count + 1, file);
      assert(ret == vert_count + 1);

      fclose(file);
      file = fopen(bw_csr_file, "rb");
      if (file == nullptr) {
        std::cout << bw_csr_file << " cannot open\n";
        exit(-1);
      }

      auto* tmp_csr = new vertex_t[edge_count]{};
      SCOPE_GUARD(delete[] tmp_csr);

      ret = fread(tmp_csr, sizeof(vertex_t), edge_count, file);
      assert(ret == edge_count);

      fclose(file);
      bw_beg_pos = new index_t[vert_count + 1]{};
      bw_csr     = new vertex_t[edge_count]{};
      for (index_t i = 0; i < vert_count + 1; ++i)
        bw_beg_pos[i] = tmp_beg_pos[i];
      for (index_t i = 0; i < edge_count; ++i)
        bw_csr[i] = tmp_csr[i];
    }

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
