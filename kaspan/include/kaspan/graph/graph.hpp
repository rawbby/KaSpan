#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>

#include <concepts>
#include <span>

namespace kaspan {

struct graph_view
{
  vertex_t  n    = 0;
  vertex_t  m    = 0;
  index_t*  head = nullptr;
  vertex_t* csr  = nullptr;

  constexpr graph_view() noexcept  = default;
  constexpr ~graph_view() noexcept = default;

  constexpr graph_view(graph_view&&) noexcept      = default;
  constexpr graph_view(graph_view const&) noexcept = default;

  constexpr graph_view(
    vertex_t  n,
    vertex_t  m,
    index_t*  head,
    vertex_t* csr) noexcept
    : n(n)
    , m(m)
    , head(head)
    , csr(csr)
  {
    DEBUG_ASSERT_GE(n, 0);
    DEBUG_ASSERT_GE(m, 0);
    DEBUG_ASSERT_SIZE_POINTER(n, head);
    DEBUG_ASSERT_SIZE_POINTER(m, csr);
  }

  constexpr auto operator=(graph_view&&) noexcept -> graph_view&      = default;
  constexpr auto operator=(graph_view const&) noexcept -> graph_view& = default;

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t u = 0; u < n; ++u)
      consumer(u);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    for (vertex_t v : std::span{ csr + head[u], csr + head[u + 1] })
      consumer(u, v);
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    each_u([&](vertex_t u) {
      each_v(u, [&](vertex_t v) {
        consumer(u, v);
      });
    });
  }

  constexpr void debug_validate() const noexcept
  {
    // validate memory consistency
    {
      DEBUG_ASSERT_GE(n, 0);
      DEBUG_ASSERT_GE(m, 0);
      DEBUG_ASSERT_SIZE_POINTER(n, head);
      DEBUG_ASSERT_SIZE_POINTER(m, csr);
      DEBUG_ASSERT_EQ(head[n], m);
      if (!std::is_constant_evaluated()) {
        KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(head, (n + 1) * sizeof(index_t));
        KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(head, m * sizeof(vertex_t));
      }
    }

    if (n > 0) {
      // validate sorted head
      {
        IF(KASPAN_DEBUG, vertex_t last_offset = head[0]);
        each_u([&](vertex_t u) {
          DEBUG_ASSERT_LE(last_offset, head[u + 1]);
          IF(KASPAN_DEBUG, last_offset = head[u + 1]);
        });
      }

      // validate sorted csr
      {
        IF(KASPAN_DEBUG, vertex_t last_u = 0);
        IF(KASPAN_DEBUG, vertex_t last_v = 0);
        each_uv([&](vertex_t u, vertex_t v) {
          if (u == last_u) DEBUG_ASSERT_LE(last_u, u);
          else DEBUG_ASSERT_LE(last_v, v);
          IF(KASPAN_DEBUG, last_u = u);
          IF(KASPAN_DEBUG, last_v = v);
        });
      }
    }
  }
};

struct graph
{
  vertex_t  n    = 0;
  vertex_t  m    = 0;
  index_t*  head = nullptr;
  vertex_t* csr  = nullptr;

  constexpr graph() noexcept = default;

  graph(
    vertex_t n,
    vertex_t m)
    : n(n)
    , m(m)
    , head(static_cast<index_t*>(line_alloc((n + 1) * sizeof(index_t))))
    , csr(static_cast<vertex_t*>(line_alloc(m * sizeof(vertex_t))))
  {
    DEBUG_ASSERT_GE(n, 0);
    DEBUG_ASSERT_GE(m, 0);
    DEBUG_ASSERT_SIZE_POINTER(n, head);
    DEBUG_ASSERT_SIZE_POINTER(m, csr);
  }
  ~graph() noexcept
  {
    line_free(head);
    line_free(csr);
  }

  constexpr graph(graph&&) noexcept      = default;
  constexpr graph(graph const&) noexcept = delete;

  constexpr auto operator=(graph&&) noexcept -> graph&      = default;
  constexpr auto operator=(graph const&) noexcept -> graph& = delete;

  [[nodiscard]] constexpr auto view() const noexcept -> graph_view
  {
    return graph_view{ n, m, head, csr };
  }
};

template<world_part_concept Part>
struct graph_part_view
{
  using part_t = Part;
  part_t*   part{};
  vertex_t  local_m = 0;
  index_t*  head    = nullptr;
  vertex_t* csr     = nullptr;
};

template<world_part_concept Part>
struct graph_part
{
  using part_t = Part;
  part_t          part{};
  vertex_t        local_m = 0;
  array<index_t>  head{};
  array<vertex_t> csr{};
};

struct bidi_graph_view
{
  vertex_t n = 0;
  vertex_t m = 0;
  struct
  {
    index_t*  head = nullptr;
    vertex_t* csr  = nullptr;
  } fw{};
  struct
  {
    index_t*  head = nullptr;
    vertex_t* csr  = nullptr;
  } bw{};
};

struct bidi_graph
{
  vertex_t n = 0;
  vertex_t m = 0;
  struct
  {
    array<index_t>  head{};
    array<vertex_t> csr{};
  } fw{};
  struct
  {
    array<index_t>  head{};
    array<vertex_t> csr{};
  } bw{};
};

template<world_part_concept Part>
struct bidi_graph_part_view
{
  using part_t = Part;
  part_t*  part{};
  vertex_t local_m = 0;
  struct
  {
    index_t*  head = nullptr;
    vertex_t* csr  = nullptr;
  } fw{};
  struct
  {
    index_t*  head = nullptr;
    vertex_t* csr  = nullptr;
  } bw{};
};

template<world_part_concept Part>
struct bidi_graph_part
{
  using part_t = Part;
  part_t*  part{};
  vertex_t local_m = 0;
  struct
  {
    array<index_t>  head{};
    array<vertex_t> csr{};
  } fw{};
  struct
  {
    array<index_t>  head{};
    array<vertex_t> csr{};
  } bw{};
};

}
