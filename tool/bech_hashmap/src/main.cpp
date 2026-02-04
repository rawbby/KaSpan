#include "kaspan/graph/balanced_slice_part.hpp"

#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/memory/accessor/hash_index_map.hpp>
#include <kaspan/scc/partion_graph.hpp>
#include <kaspan/test/fuzzy.hpp>
#include <kaspan/util/mpi_basic.hpp>

#include <algorithm>
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
  auto const [scc_id, g] = fuzzy_global_scc_id_and_graph(13, 1'000'000, 48.0);

  std::cout << "=== LOADING GRAPH PARTITION ===" << std::endl;
  auto const part = balanced_slice_part{ g.n };
  auto const graph_part   = partition(g.view(), part);

  auto run_bench = [&](auto& map, std::string const& name, auto insert_fn, auto get_fn) {
    auto const start_insert = std::chrono::high_resolution_clock::now();
    graph_part.each_k([&](auto k) {
      graph_part.each_v(k, [&](auto v) {
        if (!graph_part.part.has_local(v)) insert_fn(map, v);
      });
      graph_part.each_bw_v(k, [&](auto v) {
        if (!graph_part.part.has_local(v)) insert_fn(map, v);
      });
    });
    auto const end_insert = std::chrono::high_resolution_clock::now();

    std::vector<vertex_t> active;
    active.reserve(graph_part.part.local_n());
    std::vector is_active(graph_part.part.local_n(), false);

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
            sum += get_fn(map, v);
          }
        });
      } while (!active.empty());
    });
    auto const end_query = std::chrono::high_resolution_clock::now();
    do_not_optimize(sum);

    auto const dur_insert = std::chrono::duration_cast<std::chrono::microseconds>(end_insert - start_insert).count();
    auto const dur_query  = std::chrono::duration_cast<std::chrono::microseconds>(end_query - start_query).count();

    std::cout << std::left << std::setw(20) << name << " | Insert: " << std::setw(10) << dur_insert << " us"
              << " | Query: " << std::setw(10) << dur_query << " us" << std::endl;
  };

  std::cout << "=== STARTING BENCHMARK ===" << std::endl;

  // 1. kaspan::hash_index_map
  {
    hash_index_map<vertex_t> hmap(graph_part.local_fw_m + graph_part.local_bw_m);
    run_bench(hmap, "hash_index_map", [](auto& m, auto v) { m.insert(v); }, [](auto const& m, auto v) { return m.get(v); });
  }

  // 2. std::unordered_map
  {
    std::unordered_map<vertex_t, vertex_t> umap;
    umap.reserve(graph_part.local_fw_m + graph_part.local_bw_m);
    vertex_t count = 0;
    run_bench(umap, "unordered_map", [&count](auto& m, auto v) { m[v] = count++; }, [](auto const& m, auto v) { return m.at(v); });
  }
}
