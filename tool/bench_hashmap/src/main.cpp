#include "kaspan/graph/balanced_slice_part.hpp"

#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/memory/accessor/hash_index_map.hpp>
#include <kaspan/scc/partion_graph.hpp>
#include <kaspan/test/fuzzy.hpp>
#include <kaspan/util/mpi_basic.hpp>

#include <absl/container/flat_hash_map.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace kaspan;

template<typename T>
void
do_not_optimize(
  T const& value)
{
  asm volatile("" : : "r,m"(value) : "memory");
}

int
main()
{
  mpi_basic::world_size = 1000;
  mpi_basic::world_rank = 513;

  std::cout << "=== GENERATING RANDOM GRAPH ===" << std::endl;
  std::cout << "(this might take up to minutes)" << std::endl;
  auto const [scc_id, graph] = fuzzy_global_scc_id_and_graph(13, 1'000'000, 64.0);
  std::cout << " n = " << graph.n << std::endl;
  std::cout << " m = " << graph.m << std::endl;
  std::cout << " d = " << static_cast<double>(static_cast<u64>(graph.m) * 100 / graph.n) / 100.0 << std::endl;

  std::cout << "=== LOADING GRAPH PARTITION ===" << std::endl;
  auto const part       = balanced_slice_part{ graph.n };
  auto const graph_part = partition(graph.view(), part);
  std::cout << " world_size (fictional) = " << mpi_basic::world_size << std::endl;
  std::cout << " world_rank (fictional) = " << mpi_basic::world_rank << std::endl;
  std::cout << " local_n                = " << graph_part.part.local_n() << std::endl;
  std::cout << " local_m (fw)           = " << graph_part.local_fw_m << std::endl;
  std::cout << " local_m (bw)           = " << graph_part.local_bw_m << std::endl;

  auto run_bench = [&](std::string const& name, auto&& insert, auto&& get, auto&& clear, auto&& count) {
    u64 insert_count = 0;

    auto const start_insert = std::chrono::high_resolution_clock::now();
    graph_part.each_k([&](auto k) {
      graph_part.each_v(k, [&](auto v) {
        if (!graph_part.part.has_local(v)) {
          insert(v);
          ++insert_count;
        }
      });
      graph_part.each_bw_v(k, [&](auto v) {
        if (!graph_part.part.has_local(v)) {
          insert(v);
          ++insert_count;
        }
      });
    });
    auto const end_insert = std::chrono::high_resolution_clock::now();

    std::vector<vertex_t> active;
    active.reserve(graph_part.part.local_n());
    std::vector is_active(graph_part.part.local_n(), false);

    u64 query_count = 0;

    auto const start_query = std::chrono::high_resolution_clock::now();
    uint64_t   sum         = 0;
    graph_part.each_k([&](auto r) {
      active.push_back(r);
      do {
        auto const k = active.back();
        active.pop_back();
        graph_part.each_v(k, [&](auto v) {
          if (graph_part.part.has_local(v)) {
            auto const l = graph_part.part.to_local(v);
            if (!is_active[l]) {
              is_active[l] = true;
              active.push_back(l);
            }
          } else {
            sum += get(v);
            ++query_count;
          }
        });
      } while (!active.empty());
    });
    auto const end_query = std::chrono::high_resolution_clock::now();
    do_not_optimize(sum);

    auto const dur_insert = std::chrono::duration_cast<std::chrono::microseconds>(end_insert - start_insert).count();
    auto const dur_query  = std::chrono::duration_cast<std::chrono::microseconds>(end_query - start_query).count();

    std::cout << " Name: " << std::left << std::setw(25) << name;
    std::cout << " | Insert: " << std::right << std::setw(8) << dur_insert << " us (" << insert_count << ")";
    std::cout << " | Query: " << std::right << std::setw(6) << dur_query << " us (" << query_count << ")";
    std::cout << " | Items: " << std::right << std::setw(6) << count() << std::endl;

    clear();
  };

  std::cout << "=== STARTING BENCHMARK ===" << std::endl;
  auto hmap = hash_map<vertex_t>(graph_part.local_fw_m + graph_part.local_bw_m);
  auto umap = std::unordered_map<vertex_t, vertex_t>();
  umap.reserve(graph_part.local_fw_m + graph_part.local_bw_m);
  auto amap = absl::flat_hash_map<vertex_t, vertex_t>{};
  amap.reserve(graph_part.local_fw_m + graph_part.local_bw_m);

  for (int i = 0; i < 10; ++i) {
    run_bench("kaspan::hash_index_map", [&](auto v) { hmap.try_insert(v, v); }, [&](auto v) { return hmap.get(v); }, [&] { hmap.clear(); }, [&] { return hmap.count(); });
    run_bench("std::unordered_map", [&](auto v) { umap[v] = v; }, [&](auto v) { return umap[v]; }, [&] { umap.clear(); }, [&] { return umap.size(); });
    run_bench("absl::flat_hash_map", [&](auto v) { amap[v] = v; }, [&](auto v) { return amap[v]; }, [&] { amap.clear(); }, [&] { return amap.size(); });
  }
}
