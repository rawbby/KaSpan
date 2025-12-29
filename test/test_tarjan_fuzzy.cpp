#include <debug/assert.hpp>
#include <debug/sub_process.hpp>
#include <scc/fuzzy.hpp>
#include <scc/scc.hpp>
#include <scc/tarjan.hpp>
#include <util/scope_guard.hpp>

#include <print>
#include <cstdio>

void
lowest_to_compact_inplace(vertex_t n, vertex_t* scc_id)
{
  if (n == 0)
    return;

  std::unordered_map<vertex_t, vertex_t> map;

  vertex_t next_id = 0;
  for (vertex_t v = 0; v < n; ++v) {
    auto id = scc_id[v];
    if (v == id) {
      map.emplace(id, next_id);
      scc_id[v] = next_id;
      ++next_id;
    } else {
      // must be inside due to lowest property
      scc_id[v] = map.find(id)->second;
    }
  }
}

void
any_to_lowest_inplace(vertex_t n, vertex_t* scc_id)
{
  if (n == 0)
    return;
  std::unordered_map<vertex_t, vertex_t> map;
  for (vertex_t v = 0; v < n; ++v) {
    auto id = scc_id[v];
    if (auto it = map.find(id); it == map.end()) {
      map.emplace(id, v);
      scc_id[v] = v;
    } else {
      scc_id[v] = it->second;
    }
  }
}

int
main(int argc, char** argv)
{
  constexpr i32 np = 1;
  mpi_sub_process(argc, argv, np, &np);
  KASPAN_DEFAULT_INIT();

  for (vertex_t n = 32; n < 128; n += 8) {
    auto const seed   = mpi_basic_allreduce_max_time();
    auto const graph  = fuzzy_global_scc_id_and_graph(seed, n);
    auto       buffer = make_buffer<vertex_t>(n);
    auto*      scc_id = static_cast<vertex_t*>(buffer.data());
    for (size_t k = 0; k < n; ++k) {
      scc_id[k] = -1;
    }

    vertex_t component_count = 0;
    tarjan(n, graph.fw_head, graph.fw_csr, [&component_count, scc_id](auto const* beg, auto const* end) -> void {
      for (auto const k : std::span{ beg, end }) {
        scc_id[k] = component_count;
      }
      ++component_count;
    });

    lowest_to_compact_inplace(n, graph.scc_id);
    // any_to_lowest_inplace(n, scc_id);

    std::stringstream ss;

    std::print(ss, "  index    :");
    for (vertex_t k = 0; k < n; ++k) {
      std::print(ss, " {:2}", k);
    }
    std::println(ss);

    std::print(ss, "  scc_id   :");
    for (vertex_t k = 0; k < n; ++k) {
      std::print(ss, " {:2}", graph.scc_id[k]);
    }
    std::println(ss);

    std::print(ss, "  scc_id_  :");
    for (vertex_t k = 0; k < n; ++k) {
      std::print(ss, " {:2}", scc_id[k]);
    }
    std::println(ss);

    std::print(ss, "            ");
    for (vertex_t k = 0; k < n; ++k) {
      if (graph.scc_id[k] == scc_id[k]) {
        std::print(ss, "   ");
      } else {
        std::print(ss, " ^^");
      }
    }
    std::println(ss);

    auto const status_str = ss.str();

    MPI_Barrier(MPI_COMM_WORLD);

    for (vertex_t k = 0; k < n; ++k) {
      ASSERT_EQ(graph.scc_id[k], scc_id[k], "k = {} \n{}", k, status_str);
    }
  }
}
