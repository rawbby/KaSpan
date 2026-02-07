#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/bidi_graph.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/memory/hash_map.hpp>
#include <kaspan/memory/hash_set.hpp>
#include <kaspan/scc/partion_graph.hpp>
#include <kaspan/test/fuzzy.hpp>
#include <kaspan/util/mpi_basic.hpp>

#include <absl/container/flat_hash_map.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <numeric>
#include <print>
#include <ranges>
#include <unordered_map>
#include <vector>

using namespace kaspan;

static constexpr double TIMEOUT_SECONDS = 5.0;

template<typename T>
void
do_not_optimize(
  T const& value)
{
  asm volatile("" : : "r,m"(value) : "memory");
}

void
bench_variant(
  i32      rank,
  i32      size,
  vertex_t n,
  double   d)
{
  using clock = std::chrono::high_resolution_clock;

  mpi_basic::world_rank = rank;
  mpi_basic::world_size = size;

  std::println("=== GENERATING RANDOM GRAPH ===");
  std::println("(this might take up to minutes)");
  auto const [scc_id, graph] = fuzzy_global_scc_id_and_graph(13, n, d);
  std::println(" n = {}", graph.n);
  std::println(" m = {}", graph.m);
  std::println(" d = {:.2f}", static_cast<double>(static_cast<u64>(graph.m) * 100 / graph.n) / 100.0);

  std::println("=== LOADING GRAPH PARTITION ===");
  auto const part       = balanced_slice_part{ graph.n };
  auto const graph_part = partition(graph.view(), part);
  std::println(" world_size (fictional) = {}", mpi_basic::world_size);
  std::println(" world_rank (fictional) = {}", mpi_basic::world_rank);
  std::println(" local_n                = {}", graph_part.part.local_n());
  std::println(" local_m (fw)           = {}", graph_part.local_fw_m);
  std::println(" local_m (bw)           = {}", graph_part.local_bw_m);

  std::println("=== ANALYSING BENCHMARK ===");
  u64 insert_count         = 0;
  u64 insert_fw_count      = 0;
  u64 insert_bw_count      = 0;
  u64 query_count          = 0;
  u64 extern_edge_count    = 0;
  u64 extern_fw_edge_count = 0;
  u64 extern_bw_edge_count = 0;
  {
    hash_set<vertex_t> unique{ graph_part.local_fw_m + graph_part.local_bw_m };
    hash_set<vertex_t> unique_fw{ graph_part.local_fw_m };
    hash_set<vertex_t> unique_bw{ graph_part.local_bw_m };

    graph_part.each_v([&](auto v) {
      if (!graph_part.part.has_local(v)) {
        unique.try_insert(v);
        unique_fw.try_insert(v);
        ++insert_count;
        ++insert_fw_count;
      }
    });
    graph_part.each_bw_v([&](auto v) {
      if (!graph_part.part.has_local(v)) {
        unique.try_insert(v);
        unique_bw.try_insert(v);
        ++insert_count;
        ++insert_bw_count;
      }
    });

    extern_edge_count    = unique.count();
    extern_fw_edge_count = unique_fw.count();
    extern_bw_edge_count = unique_bw.count();

    std::vector<vertex_t> active;
    active.reserve(graph_part.part.local_n());
    std::vector is_active(graph_part.part.local_n(), false);
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
            ++query_count;
          }
        });
      } while (!active.empty());
    });
  }
  std::println(" extern vertex count / insert count = {}", insert_count);
  std::println(" extern vertex count (fw)           = {}", insert_fw_count);
  std::println(" extern vertex count (bw)           = {}", insert_bw_count);
  std::println(" extern unique vertex count         = {}", extern_edge_count);
  std::println(" extern unique vertex count (fw)    = {}", extern_fw_edge_count);
  std::println(" extern unique vertex count (bw)    = {}", extern_bw_edge_count);
  std::println(" query count                        = {}", query_count);

  auto run_bench = [&](auto&& insert, auto&& get, auto&& clear, auto&& get_or_insert) {
    u64 dur_insert;
    u64 dur_query;
    u64 dur_mixed;

    {
      auto const start_insert = clock::now();
      graph_part.each_k([&](auto k) {
        graph_part.each_v(k, [&](auto v) {
          if (!graph_part.part.has_local(v)) insert(v);
        });
        graph_part.each_bw_v(k, [&](auto v) {
          if (!graph_part.part.has_local(v)) insert(v);
        });
      });
      dur_insert = std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - start_insert).count();
    }

    {
      std::vector<vertex_t> active;
      active.reserve(graph_part.part.local_n());
      std::vector is_active(graph_part.part.local_n(), false);

      auto const start_query = clock::now();
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
            }
          });
        } while (!active.empty());
      });
      dur_query = std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - start_query).count();
      do_not_optimize(sum);
    }

    clear();

    {
      std::vector<vertex_t> active;
      active.reserve(graph_part.part.local_n());
      std::vector is_active(graph_part.part.local_n(), false);

      auto const start_query = clock::now();
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
              sum += get_or_insert(v);
            }
          });
        } while (!active.empty());
      });
      dur_mixed = std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - start_query).count();
      do_not_optimize(sum);
    }

    return std::make_tuple(dur_insert, dur_query, dur_mixed);
  };

  std::println("=== STARTING BENCHMARK ===");

  std::vector<u64> std_insert_times;
  std::vector<u64> std_query_times;
  std::vector<u64> std_mixed_times;
  u64              std_iterations = 0;

  {
    auto umap = std::unordered_map<vertex_t, vertex_t>{};
    umap.reserve(graph_part.local_fw_m + graph_part.local_bw_m);
    auto const insert        = [&](auto v) { umap.insert({ v, v }); };
    auto const get           = [&](auto v) { return umap.find(v)->second; };
    auto const clear         = [&] { umap.clear(); };
    auto const get_or_insert = [&](auto v) { return umap.try_emplace(v, v).first->second; };

    auto const start = clock::now();
    do {
      auto [i, q, m] = run_bench(insert, get, clear, get_or_insert);
      std_insert_times.push_back(i);
      std_query_times.push_back(q);
      std_mixed_times.push_back(m);
      ++std_iterations;
    } while (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() < TIMEOUT_SECONDS);
  }

  std::vector<u64> absl_insert_times;
  std::vector<u64> absl_query_times;
  std::vector<u64> absl_mixed_times;
  u64              absl_iterations = 0;

  {
    auto amap = absl::flat_hash_map<vertex_t, vertex_t>{};
    amap.reserve(graph_part.local_fw_m + graph_part.local_bw_m);
    auto const insert        = [&](auto v) { amap.insert({ v, v }); };
    auto const get           = [&](auto v) { return amap.find(v)->second; };
    auto const clear         = [&] { amap.clear(); };
    auto const get_or_insert = [&](auto v) { return amap.try_emplace(v, v).first->second; };

    auto const start = clock::now();
    do {
      auto [i, q, m] = run_bench(insert, get, clear, get_or_insert);
      absl_insert_times.push_back(i);
      absl_query_times.push_back(q);
      absl_mixed_times.push_back(m);
      ++absl_iterations;
    } while (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() < TIMEOUT_SECONDS);
  }

  std::vector<u64> kaspan_insert_times;
  std::vector<u64> kaspan_query_times;
  std::vector<u64> kaspan_mixed_times;
  u64              kaspan_iterations = 0;

  {
    auto       hmap          = hash_map<vertex_t>{ graph_part.local_fw_m + graph_part.local_bw_m };
    auto const insert        = [&](auto v) { hmap.try_insert(v, v); };
    auto const get           = [&](auto v) { return hmap.get(v); };
    auto const clear         = [&] { hmap.clear(); };
    auto const get_or_insert = [&](auto v) { return hmap.get_or_insert(v, v); };

    auto const start = clock::now();
    do {
      auto [i, q, m] = run_bench(insert, get, clear, get_or_insert);
      kaspan_insert_times.push_back(i);
      kaspan_query_times.push_back(q);
      kaspan_mixed_times.push_back(m);
      ++kaspan_iterations;
    } while (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() < TIMEOUT_SECONDS);
  }

  u64    std_total_insert_time = std::accumulate(std_insert_times.begin(), std_insert_times.end(), 0ULL);
  u64    std_total_query_time  = std::accumulate(std_query_times.begin(), std_query_times.end(), 0ULL);
  u64    std_total_mixed_time  = std::accumulate(std_mixed_times.begin(), std_mixed_times.end(), 0ULL);
  double std_avg_insert        = static_cast<double>(std_total_insert_time) / std_iterations;
  double std_avg_query         = static_cast<double>(std_total_query_time) / std_iterations;
  double std_avg_mixed         = static_cast<double>(std_total_mixed_time) / std_iterations;
  double std_avg               = std_avg_insert + std_avg_query;

  u64    absl_total_insert_time = std::accumulate(absl_insert_times.begin(), absl_insert_times.end(), 0ULL);
  u64    absl_total_query_time  = std::accumulate(absl_query_times.begin(), absl_query_times.end(), 0ULL);
  u64    absl_total_mixed_time  = std::accumulate(absl_mixed_times.begin(), absl_mixed_times.end(), 0ULL);
  double absl_avg_insert        = static_cast<double>(absl_total_insert_time) / absl_iterations;
  double absl_avg_query         = static_cast<double>(absl_total_query_time) / absl_iterations;
  double absl_avg_mixed         = static_cast<double>(absl_total_mixed_time) / absl_iterations;
  double absl_avg               = absl_avg_insert + absl_avg_query;

  u64    kaspan_total_insert_time = std::accumulate(kaspan_insert_times.begin(), kaspan_insert_times.end(), 0ULL);
  u64    kaspan_total_query_time  = std::accumulate(kaspan_query_times.begin(), kaspan_query_times.end(), 0ULL);
  u64    kaspan_total_mixed_time  = std::accumulate(kaspan_mixed_times.begin(), kaspan_mixed_times.end(), 0ULL);
  double kaspan_avg_insert        = static_cast<double>(kaspan_total_insert_time) / kaspan_iterations;
  double kaspan_avg_query         = static_cast<double>(kaspan_total_query_time) / kaspan_iterations;
  double kaspan_avg_mixed         = static_cast<double>(kaspan_total_mixed_time) / kaspan_iterations;
  double kaspan_avg               = kaspan_avg_insert + kaspan_avg_query;

  auto print_row =
    [&](std::string_view name, double avg, double insert, double query, double mixed, double ref_avg, double ref_insert, double ref_query, double ref_mixed) {
      std::println("| {:<19} | {:>7.2f} us ({:>3.0f}%) | {:>7.2f} us ({:>3.0f}%) | {:>7.2f} us ({:>3.0f}%) | {:>7.2f} us ({:>3.0f}%) |",
                   name,
                   avg,
                   ref_avg / avg * 100.0,
                   insert,
                   ref_insert / insert * 100.0,
                   query,
                   ref_query / query * 100.0,
                   mixed,
                   ref_mixed / mixed * 100.0);
    };

  std::println("| Name                | Avg (Ins+Que)     | Avg Insert        | Avg Query         | Avg Mixed         |");
  print_row("std::unordered_map", std_avg, std_avg_insert, std_avg_query, std_avg_mixed, std_avg, std_avg_insert, std_avg_query, std_avg_mixed);
  print_row("absl::flat_hash_map", absl_avg, absl_avg_insert, absl_avg_query, absl_avg_mixed, std_avg, std_avg_insert, std_avg_query, std_avg_mixed);
  print_row("kaspan::hash_map", kaspan_avg, kaspan_avg_insert, kaspan_avg_query, kaspan_avg_mixed, std_avg, std_avg_insert, std_avg_query, std_avg_mixed);

  std::println();
}

int
main()
{
  std::vector<std::tuple<i32, i32, vertex_t, double>> configs;

  configs.emplace_back(54, 512, 8'000'000, 2.0);
  configs.emplace_back(54, 512, 16'000'000, 2.0);
  configs.emplace_back(54, 512, 32'000'000, 2.0);

  configs.emplace_back(54, 512, 4'000'000, 4.0);
  configs.emplace_back(54, 512, 8'000'000, 4.0);
  configs.emplace_back(54, 512, 16'000'000, 4.0);

  configs.emplace_back(54, 512, 2'000'000, 8.0);
  configs.emplace_back(54, 512, 4'000'000, 8.0);
  configs.emplace_back(54, 512, 8'000'000, 8.0);

  configs.emplace_back(54, 512, 1'000'000, 16.0);
  configs.emplace_back(54, 512, 2'000'000, 16.0);
  configs.emplace_back(54, 512, 4'000'000, 16.0);

  configs.emplace_back(54, 512, 500'000, 32.0);
  configs.emplace_back(54, 512, 1'000'000, 32.0);
  configs.emplace_back(54, 512, 2'000'000, 32.0);

  configs.emplace_back(54, 512, 250'000, 64.0);
  configs.emplace_back(54, 512, 500'000, 64.0);
  configs.emplace_back(54, 512, 1'000'000, 64.0);

  configs.emplace_back(54, 512, 125'000, 128.0);
  configs.emplace_back(54, 512, 250'000, 128.0);
  configs.emplace_back(54, 512, 500'000, 128.0);

  for (auto const [rank, size, n, d] : configs) {
    bench_variant(rank, size, n, d);
  }
}
