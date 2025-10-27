#pragma once

#include <debug/Assert.hpp>
#include <debug/Debug.hpp>
#include <pp/PP.hpp>
#include <util/Arithmetic.hpp>
#include <util/ScopeGuard.hpp>

#include <chrono>
#include <fstream>
#include <mpi.h>
#include <ostream>
#include <vector>

#include "comm/MpiBasic.hpp"

#ifndef KASPAN_STATISTIC
#define KASPAN_STATISTIC KASPAN_DEBUG
#endif

struct statistic_entry
{
  size_t      parent;
  char const* name;
  u64         value;
};

struct statistic_node
{
  using clock     = std::chrono::steady_clock;
  using unit      = std::chrono::nanoseconds;
  using timestamp = unit::rep;

  size_t      parent;
  size_t      end;
  char const* name;
  timestamp   duration;

  statistic_node(size_t parent, size_t end, char const* name)
    : parent(parent)
    , end(end)
    , name(name)
    , duration(now())
  {
  }

  static timestamp now()
  {
    return std::chrono::duration_cast<unit>(clock::now().time_since_epoch()).count();
  }

  void finish()
  {
    duration = now() - duration;
  }
};

#if KASPAN_STATISTIC
inline auto g_kaspan_statistic_nodes = [] {
  auto result = std::vector<statistic_node>{};
  result.reserve(1024);
  return result;
}();
inline auto g_kaspan_statistic_entries = [] {
  auto result = std::vector<statistic_entry>{};
  result.reserve(1024);
  return result;
}();
inline auto g_kaspan_statistic_stack = [] {
  auto result = std::vector<size_t>{};
  result.reserve(64);
  result.emplace_back(SIZE_MAX); // sentinel
  return result;
}();
#endif

/// Adds a key (string literal), value (unsigned 64-bit integer) statistic to the current scope.
/// Ensure that `KASPAN_STATISTIC_PUSH` or `KASPAN_STATISTIC_SCOPE` was called before,
/// else this macro may lead to undefined behaviour.
/// note: Dont use with multithreaded applications!
inline void
kaspan_statistic_add(char const* name, u64 value)
{
#if KASPAN_STATISTIC
  DEBUG_ASSERT(not g_kaspan_statistic_stack.empty());
  auto const parent = g_kaspan_statistic_stack.back();
  g_kaspan_statistic_entries.emplace_back(parent, name, value);
#endif
}

/// Start a new scope. Ensure every scope is finalized (via `KASPAN_STATISTIC_POP`) when the program is finished.
/// note: Dont use with multithreaded applications!
inline void
kaspan_statistic_push(char const* name)
{
#if KASPAN_STATISTIC
  auto const new_parent = g_kaspan_statistic_nodes.size();
  auto const old_parent = g_kaspan_statistic_stack.back();
  g_kaspan_statistic_nodes.emplace_back(old_parent, new_parent, name);
  g_kaspan_statistic_stack.emplace_back(new_parent);
#endif
}

/// Finalizes the current scope. Ensure every scope is finalized when the program is finished.
/// note: Dont use with multithreaded applications!
inline void
kaspan_statistic_pop()
{
#if KASPAN_STATISTIC
  DEBUG_ASSERT(g_kaspan_statistic_stack.size() > 1);
  auto const old_parent = g_kaspan_statistic_stack.back();
  g_kaspan_statistic_stack.pop_back();
  auto const new_parent = g_kaspan_statistic_stack.back();
  g_kaspan_statistic_nodes[old_parent].finish();
  if (new_parent != SIZE_MAX) {
    g_kaspan_statistic_nodes[new_parent].end = old_parent + 1;
  }
#endif
}

// clang-format off
/// Compines `KASPAN_STATISTIC_PUSH` and `KASPAN_STATISTIC_POP`
/// where push is called directly and pop is called on scope exit.
/// note: Dont use with multithreaded applications!
#define KASPAN_STATISTIC_SCOPE(NAME) [[maybe_unused]] auto const CAT(guard,__COUNTER__) = IF(KASPAN_STATISTIC, \
  ScopeGuard([]{kaspan_statistic_push(NAME);}, []{kaspan_statistic_pop();}), 0)
// clang-format on

inline void
kaspan_statistic_write_json(std::ostream& os)
{
#if KASPAN_STATISTIC
  auto const& nodes   = g_kaspan_statistic_nodes;
  auto const& entries = g_kaspan_statistic_entries;
  auto        dfs     = [&](auto&& self, size_t& nit, size_t nend, size_t& eit) -> void {
    os << '"' << nodes[nit].name << '"' << ':' << '{';
    os << '"' << 'd' << 'u' << 'r' << 'a' << 't' << 'i' << 'o' << 'n' << '"' << ':' << nodes[nit].duration;
    auto const self_nit = nit;
    ++nit;
    while (nit < nend) {
      while (eit < entries.size() and entries[eit].parent == self_nit) {
        os << ',' << '"' << entries[eit].name << '"' << ':' << entries[eit].value;
        ++eit;
      }
      DEBUG_ASSERT_EQ(nodes[nit].parent, self_nit);
      os << ',';
      self(self, nit, nodes[nit].end, eit);
    }
    while (eit < entries.size() and entries[eit].parent == self_nit) {
      os << ',' << '"' << entries[eit].name << '"' << ':' << entries[eit].value;
      ++eit;
    }
    os << '}';
  };

  os << '{';
  size_t nit        = 0; // node iterator
  size_t eit        = 0; // entry iterator
  bool   first_root = true;
  while (nit < nodes.size()) {
    DEBUG_ASSERT(nodes[nit].parent == SIZE_MAX);
    if (not first_root)
      os << ',';
    first_root = false;

    dfs(dfs, nit, nodes[nit].end, eit);
  }
  os << '}';

  DEBUG_ASSERT_EQ(nit, nodes.size());
  DEBUG_ASSERT_EQ(eit, entries.size());
#else
  os << "{}";
#endif
}

inline void
kaspan_statistic_mpi_write_json(char const* file_path)
{
#if KASPAN_STATISTIC

  int rank = 0;
  int size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank == 0) {

    std::ofstream os{ file_path };
    os << '{' << '"' << '0' << '"' << ':';
    kaspan_statistic_write_json(os);
    constexpr int local_size = 0;

    std::vector recvcounts(size, 0);
    std::vector displs(size, 0);
    MPI_Gather(&local_size, 1, MPI_INT, recvcounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    size_t total_bytes = 0;
    for (int r = 0; r < size; ++r) {
      displs[r] = static_cast<int>(total_bytes);
      total_bytes += recvcounts[r];
    }

    std::vector<char> recvbuf(total_bytes);
    MPI_Gatherv(nullptr, 0, MPI_CHAR, recvbuf.data(), recvcounts.data(), displs.data(), MPI_CHAR, 0, MPI_COMM_WORLD);
    for (int r = 1; r < size; ++r) {
      auto const offset = static_cast<size_t>(displs[r]);
      auto const count  = static_cast<size_t>(recvcounts[r]);
      if (count != 0) {
        auto const json_obj = std::string_view{ recvbuf.data() + offset, count };
        os << ',' << '"' << r << '"' << ':' << json_obj;
      }
    }
    os << '}';

  } else {
    std::ostringstream ss;
    kaspan_statistic_write_json(ss);
    auto const json        = ss.str();
    auto const buffer_size = static_cast<int>(json.size());
    MPI_Gather(&buffer_size, 1, MPI_INT, nullptr, 0, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gatherv(json.data(), buffer_size, MPI_CHAR, nullptr, nullptr, nullptr, MPI_CHAR, 0, MPI_COMM_WORLD);
  }
#endif
}
