#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/pp.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <chrono>
#include <fstream>
#include <kaspan/mpi_basic/mpi_basic.hpp>
#include <ostream>
#include <vector>

namespace kaspan {

#ifndef KASPAN_STATISTIC
#define KASPAN_STATISTIC KASPAN_DEBUG
#endif

#if KASPAN_STATISTIC

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

  static timestamp now() { return std::chrono::duration_cast<unit>(clock::now().time_since_epoch()).count(); }

  void finish() { duration = now() - duration; }
};

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

inline void
kaspan_statistic_add(char const* name, u64 value)
{
  DEBUG_ASSERT(not g_kaspan_statistic_stack.empty());
  auto const parent = g_kaspan_statistic_stack.back();
  g_kaspan_statistic_entries.emplace_back(parent, name, value);
}

inline void
kaspan_statistic_push(char const* name)
{
  auto const new_parent = g_kaspan_statistic_nodes.size();
  auto const old_parent = g_kaspan_statistic_stack.back();
  g_kaspan_statistic_nodes.emplace_back(old_parent, new_parent, name);
  g_kaspan_statistic_stack.emplace_back(new_parent);
}

inline void
kaspan_statistic_pop()
{
  DEBUG_ASSERT(g_kaspan_statistic_stack.size() > 1);
  auto const old_parent = g_kaspan_statistic_stack.back();
  g_kaspan_statistic_stack.pop_back();
  auto const new_parent = g_kaspan_statistic_stack.back();
  g_kaspan_statistic_nodes[old_parent].finish();
  if (new_parent != SIZE_MAX) { g_kaspan_statistic_nodes[new_parent].end = old_parent + 1; }
}

inline void
kaspan_statistic_mpi_write_json(char const* file_path)
{
  constexpr auto kaspan_statistic_write_json = [](std::ostream& os) {
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
      if (not first_root) { os << ','; }
      first_root = false;

      dfs(dfs, nit, nodes[nit].end, eit);
    }
    os << '}';

    DEBUG_ASSERT_EQ(nit, nodes.size());
    DEBUG_ASSERT_EQ(eit, entries.size());
  };

  int const rank = mpi_basic::world_rank;
  int const size = mpi_basic::world_size;

  if (rank == 0) {

    std::ofstream os{ file_path };
    os << '{' << '"' << '0' << '"' << ':';
    kaspan_statistic_write_json(os);
    constexpr int local_size = 0;

    std::vector recvcounts(size, 0);
    std::vector displs(size, 0);
    mpi_basic::gather(local_size, recvcounts.data(), 0);

    size_t total_bytes = 0;
    for (int r = 0; r < size; ++r) {
      displs[r] = static_cast<int>(total_bytes);
      total_bytes += recvcounts[r];
    }

    std::vector<char> recvbuf(total_bytes);
    mpi_basic::gatherv<char>(nullptr, 0, recvbuf.data(), recvcounts.data(), displs.data(), 0);
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
    mpi_basic::gather<int>(buffer_size, nullptr, 0);
    mpi_basic::gatherv<char>(json.data(), buffer_size, nullptr, nullptr, nullptr, 0);
  }
}

#else

inline void
kaspan_statistic_add(char const* name, u64 value)
{
}

inline void
kaspan_statistic_push(char const* name)
{
}

inline void
kaspan_statistic_pop()
{
}

inline void
kaspan_statistic_mpi_write_json(char const* file_path)
{
}

#endif

#define IF_KASPAN_STATISTIC(...) IF(KASPAN_STATISTIC, __VA_ARGS__)

/// Adds a key (string literal), value (unsigned 64-bit integer) statistic to the current scope.
/// Ensure that `KASPAN_STATISTIC_PUSH` or `KASPAN_STATISTIC_SCOPE` was called before,
/// else this macro may lead to undefined behaviour.
/// note: Dont use with multithreaded applications!
#define KASPAN_STATISTIC_ADD(NAME, VALUE) kaspan::kaspan_statistic_add((NAME), (VALUE))

/// Start a new scope. Ensure every scope is finalized (via `KASPAN_STATISTIC_POP`) when the program is finished.
/// note: Dont use with multithreaded applications!
#define KASPAN_STATISTIC_PUSH(NAME) kaspan::kaspan_statistic_push(NAME)

/// Finalizes the current scope. Ensure every scope is finalized when the program is finished.
/// note: Dont use with multithreaded applications!
#define KASPAN_STATISTIC_POP() kaspan::kaspan_statistic_pop()

#define KASPAN_STATISTIC_MPI_WRITE_JSON(FILENAME) kaspan::kaspan_statistic_mpi_write_json(FILENAME)

// clang-format off
/// Compines `KASPAN_STATISTIC_PUSH` and `KASPAN_STATISTIC_POP`
/// where push is called directly and pop is called on scope exit.
/// note: Dont use with multithreaded applications!
#define KASPAN_STATISTIC_SCOPE(NAME) [[maybe_unused]] auto const CAT(guard,__COUNTER__) = IF_KASPAN_STATISTIC( \
  kaspan::scope_guard([]{kaspan::kaspan_statistic_push(NAME);}, []{kaspan::kaspan_statistic_pop();}), 0)
// clang-format on

} // namespace kaspan
