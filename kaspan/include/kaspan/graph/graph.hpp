#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/borrow.hpp>
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
    if (!std::is_constant_evaluated()) {
      if (n > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, (n + 1) * sizeof(index_t));
      if (m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, m * sizeof(vertex_t));
    }
  }

  constexpr auto operator=(graph_view&&) noexcept -> graph_view&      = default;
  constexpr auto operator=(graph_view const&) noexcept -> graph_view& = default;

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return { csr + head[u], csr + head[u + 1] };
  }

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
    for (vertex_t v : csr_range(u))
      consumer(v);
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(head[u + 1] - head[u]);
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
        if (n > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(head, (n + 1) * sizeof(index_t));
        if (m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(head, m * sizeof(vertex_t));
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
    , head(line_alloc<index_t>(n + 1))
    , csr(line_alloc<vertex_t>(m))
  {
    DEBUG_ASSERT_GE(n, 0);
    DEBUG_ASSERT_GE(m, 0);
    DEBUG_ASSERT_SIZE_POINTER(n, head);
    DEBUG_ASSERT_SIZE_POINTER(m, csr);
    if (!std::is_constant_evaluated()) {
      if (n > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, (n + 1) * sizeof(index_t));
      if (m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, m * sizeof(vertex_t));
    }
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

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return { csr + head[u], csr + head[u + 1] };
  }

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
    for (vertex_t v : csr_range(u))
      consumer(v);
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(head[u + 1] - head[u]);
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
    view().debug_validate();
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

  constexpr graph_part_view() noexcept  = default;
  constexpr ~graph_part_view() noexcept = default;

  constexpr graph_part_view(graph_part_view&&) noexcept      = default;
  constexpr graph_part_view(graph_part_view const&) noexcept = default;

  constexpr graph_part_view(
    part_t*   part,
    vertex_t  local_m,
    index_t*  head,
    vertex_t* csr) noexcept
    : part(part)
    , local_m(local_m)
    , head(head)
    , csr(csr)
  {
    DEBUG_ASSERT_POINTER(part);
    DEBUG_ASSERT_GE(part->n, 0);
    DEBUG_ASSERT_GE(part->local_n(), 0);
    DEBUG_ASSERT_SIZE_POINTER(part->local_n(), head);
    DEBUG_ASSERT_SIZE_POINTER(local_m, csr);
    if (!std::is_constant_evaluated()) {
      if (part->local_n() > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, (part->local_n() + 1) * sizeof(index_t));
      if (local_m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, local_m * sizeof(vertex_t));
    }
  }

  constexpr auto operator=(graph_part_view&&) noexcept -> graph_part_view&      = default;
  constexpr auto operator=(graph_part_view const&) noexcept -> graph_part_view& = default;

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return { csr + head[u], csr + head[u + 1] };
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t u = 0; u < part->local_n(); ++u)
      consumer(u);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    for (vertex_t v : csr_range(u))
      consumer(v);
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(head[u + 1] - head[u]);
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
      DEBUG_ASSERT_GE(part->local_n(), 0);
      DEBUG_ASSERT_GE(local_m, 0);
      DEBUG_ASSERT_SIZE_POINTER(part->local_n(), head);
      DEBUG_ASSERT_SIZE_POINTER(local_m, csr);
      DEBUG_ASSERT_EQ(head[part->local_n()], local_m);
      if (!std::is_constant_evaluated()) {
        if (part->local_n() > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(head, (part->local_n() + 1) * sizeof(index_t));
        if (local_m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(head, local_m * sizeof(vertex_t));
      }
    }

    if (part->local_n() > 0) {
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

template<world_part_concept Part>
struct graph_part
{
  using part_t = Part;
  part_t          part{};
  vertex_t        local_m = 0;
  array<index_t>  head{};
  array<vertex_t> csr{};

  constexpr graph_part() noexcept = default;

  graph_part(
    part_t   part,
    vertex_t local_m)
    : part(part)
    , local_m(local_m)
    , head(part.local_n() + 1)
    , csr(local_m)
  {
    DEBUG_ASSERT_POINTER(part);
    DEBUG_ASSERT_GE(part->n, 0);
    DEBUG_ASSERT_GE(part->local_n(), 0);
    DEBUG_ASSERT_SIZE_POINTER(part->local_n(), head);
    DEBUG_ASSERT_SIZE_POINTER(local_m, csr);
    if (!std::is_constant_evaluated()) {
      if (part->local_n() > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, (part->local_n() + 1) * sizeof(index_t));
      if (local_m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, local_m * sizeof(vertex_t));
    }
  }

  ~graph_part() noexcept = default;

  constexpr graph_part(graph_part&&) noexcept      = default;
  constexpr graph_part(graph_part const&) noexcept = delete;

  constexpr auto operator=(graph_part&&) noexcept -> graph_part&      = default;
  constexpr auto operator=(graph_part const&) noexcept -> graph_part& = delete;

  [[nodiscard]] constexpr auto view() const noexcept -> graph_part_view<part_t>
  {
    return { const_cast<part_t*>(&part), local_m, const_cast<index_t*>(head.data()), const_cast<vertex_t*>(csr.data()) };
  }

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return view().csr_range(u);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    view().each_u(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    view().each_v(u, std::forward<Consumer>(consumer));
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return view().outdegree(u);
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_uv(std::forward<Consumer>(consumer));
  }

  constexpr void debug_validate() const noexcept
  {
    view().debug_validate();
  }
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

  constexpr bidi_graph_view() noexcept  = default;
  constexpr ~bidi_graph_view() noexcept = default;

  constexpr bidi_graph_view(bidi_graph_view&&) noexcept      = default;
  constexpr bidi_graph_view(bidi_graph_view const&) noexcept = default;

  constexpr bidi_graph_view(
    vertex_t  n,
    vertex_t  m,
    index_t*  fw_head,
    vertex_t* fw_csr,
    index_t*  bw_head,
    vertex_t* bw_csr) noexcept
    : n(n)
    , m(m)
    , fw{ fw_head,
          fw_csr }
    , bw{ bw_head,
          bw_csr }
  {
    DEBUG_ASSERT_GE(n, 0);
    DEBUG_ASSERT_GE(m, 0);
    DEBUG_ASSERT_SIZE_POINTER(n, fw.head);
    DEBUG_ASSERT_SIZE_POINTER(m, fw.csr);
    DEBUG_ASSERT_SIZE_POINTER(n, bw.head);
    DEBUG_ASSERT_SIZE_POINTER(m, bw.csr);
  }

  constexpr auto operator=(bidi_graph_view&&) noexcept -> bidi_graph_view&      = default;
  constexpr auto operator=(bidi_graph_view const&) noexcept -> bidi_graph_view& = default;

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return { fw.csr + fw.head[u], fw.csr + fw.head[u + 1] };
  }

  [[nodiscard]] constexpr auto bw_csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return { bw.csr + bw.head[u], bw.csr + bw.head[u + 1] };
  }

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
    for (vertex_t v : csr_range(u))
      consumer(v);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_bw_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    for (vertex_t v : bw_csr_range(u))
      consumer(v);
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(fw.head[u + 1] - fw.head[u]);
  }

  [[nodiscard]] constexpr auto indegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(bw.head[u + 1] - bw.head[u]);
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

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    each_u([&](vertex_t u) {
      each_bw_v(u, [&](vertex_t v) {
        consumer(u, v);
      });
    });
  }

  constexpr void debug_validate() const noexcept
  {
    DEBUG_ASSERT_GE(n, 0);
    DEBUG_ASSERT_GE(m, 0);
    DEBUG_ASSERT_SIZE_POINTER(n, fw.head);
    DEBUG_ASSERT_SIZE_POINTER(m, fw.csr);
    DEBUG_ASSERT_SIZE_POINTER(n, bw.head);
    DEBUG_ASSERT_SIZE_POINTER(m, bw.csr);
    DEBUG_ASSERT_EQ(fw.head[n], m);
    DEBUG_ASSERT_EQ(bw.head[n], m);
  }
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

  constexpr bidi_graph() noexcept = default;

  bidi_graph(
    vertex_t n,
    vertex_t m)
    : n(n)
    , m(m)
    , fw{ array<index_t>{ n + 1 },
          array<vertex_t>{ m } }
    , bw{ array<index_t>{ n + 1 },
          array<vertex_t>{ m } }
  {
    DEBUG_ASSERT_GE(n, 0);
    DEBUG_ASSERT_GE(m, 0);
  }

  ~bidi_graph() noexcept = default;

  constexpr bidi_graph(bidi_graph&&) noexcept      = default;
  constexpr bidi_graph(bidi_graph const&) noexcept = delete;

  constexpr auto operator=(bidi_graph&&) noexcept -> bidi_graph&      = default;
  constexpr auto operator=(bidi_graph const&) noexcept -> bidi_graph& = delete;

  [[nodiscard]] constexpr auto view() const noexcept -> bidi_graph_view
  {
    return { n, m, const_cast<index_t*>(fw.head.data()), const_cast<vertex_t*>(fw.csr.data()), const_cast<index_t*>(bw.head.data()), const_cast<vertex_t*>(bw.csr.data()) };
  }

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return view().csr_range(u);
  }

  [[nodiscard]] constexpr auto bw_csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return view().bw_csr_range(u);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    view().each_u(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    view().each_v(u, std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_bw_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    view().each_bw_v(u, std::forward<Consumer>(consumer));
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return view().outdegree(u);
  }

  [[nodiscard]] constexpr auto indegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return view().indegree(u);
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_uv(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_bw_uv(std::forward<Consumer>(consumer));
  }

  constexpr void debug_validate() const noexcept
  {
    view().debug_validate();
  }
};

template<world_part_concept Part>
struct bidi_graph_part_view
{
  using part_t = Part;
  part_t*  part{};
  vertex_t local_m_fw = 0;
  vertex_t local_m_bw = 0;
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

  constexpr bidi_graph_part_view() noexcept  = default;
  constexpr ~bidi_graph_part_view() noexcept = default;

  constexpr bidi_graph_part_view(bidi_graph_part_view&&) noexcept      = default;
  constexpr bidi_graph_part_view(bidi_graph_part_view const&) noexcept = default;

  constexpr bidi_graph_part_view(
    part_t*   part,
    vertex_t  local_m_fw,
    vertex_t  local_m_bw,
    index_t*  fw_head,
    vertex_t* fw_csr,
    index_t*  bw_head,
    vertex_t* bw_csr) noexcept
    : part(part)
    , local_m_fw(local_m_fw)
    , local_m_bw(local_m_bw)
    , fw{ fw_head,
          fw_csr }
    , bw{ bw_head,
          bw_csr }
  {
    DEBUG_ASSERT_NE(part, nullptr);
    DEBUG_ASSERT_GE(local_m_fw, 0);
    DEBUG_ASSERT_GE(local_m_bw, 0);
    DEBUG_ASSERT_SIZE_POINTER(part->local_n(), fw.head);
    DEBUG_ASSERT_SIZE_POINTER(local_m_fw, fw.csr);
    DEBUG_ASSERT_SIZE_POINTER(part->local_n(), bw.head);
    DEBUG_ASSERT_SIZE_POINTER(local_m_bw, bw.csr);
  }

  constexpr auto operator=(bidi_graph_part_view&&) noexcept -> bidi_graph_part_view&      = default;
  constexpr auto operator=(bidi_graph_part_view const&) noexcept -> bidi_graph_part_view& = default;

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return { fw.csr + fw.head[u], fw.csr + fw.head[u + 1] };
  }

  [[nodiscard]] constexpr auto bw_csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return { bw.csr + bw.head[u], bw.csr + bw.head[u + 1] };
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t u = 0; u < part->local_n(); ++u)
      consumer(u);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    for (vertex_t v : csr_range(u))
      consumer(v);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_bw_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    for (vertex_t v : bw_csr_range(u))
      consumer(v);
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(fw.head[u + 1] - fw.head[u]);
  }

  [[nodiscard]] constexpr auto indegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return integral_cast<vertex_t>(bw.head[u + 1] - bw.head[u]);
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

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    each_u([&](vertex_t u) {
      each_bw_v(u, [&](vertex_t v) {
        consumer(u, v);
      });
    });
  }

  constexpr void debug_validate() const noexcept
  {
    DEBUG_ASSERT_NE(part, nullptr);
    vertex_t const local_n = part->local_n();
    DEBUG_ASSERT_GE(local_n, 0);
    DEBUG_ASSERT_GE(local_m_fw, 0);
    DEBUG_ASSERT_GE(local_m_bw, 0);
    DEBUG_ASSERT_SIZE_POINTER(local_n, fw.head);
    DEBUG_ASSERT_SIZE_POINTER(local_m_fw, fw.csr);
    DEBUG_ASSERT_SIZE_POINTER(local_n, bw.head);
    DEBUG_ASSERT_SIZE_POINTER(local_m_bw, bw.csr);
    DEBUG_ASSERT_EQ(fw.head[local_n], local_m_fw);
    DEBUG_ASSERT_EQ(bw.head[local_n], local_m_bw);
  }
};

template<world_part_concept Part>
struct bidi_graph_part
{
  using part_t = Part;
  part_t   part{};
  vertex_t local_m_fw = 0;
  vertex_t local_m_bw = 0;
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

  constexpr bidi_graph_part() noexcept = default;

  bidi_graph_part(
    part_t   part,
    vertex_t local_m_fw,
    vertex_t local_m_bw)
    : part(part)
    , local_m_fw(local_m_fw)
    , local_m_bw(local_m_bw)
    , fw{ array<index_t>{ part.local_n() + 1 },
          array<vertex_t>{ local_m_fw } }
    , bw{ array<index_t>{ part.local_n() + 1 },
          array<vertex_t>{ local_m_bw } }
  {
    DEBUG_ASSERT_GE(local_m_fw, 0);
    DEBUG_ASSERT_GE(local_m_bw, 0);
  }

  ~bidi_graph_part() noexcept = default;

  constexpr bidi_graph_part(bidi_graph_part&&) noexcept      = default;
  constexpr bidi_graph_part(bidi_graph_part const&) noexcept = delete;

  constexpr auto operator=(bidi_graph_part&&) noexcept -> bidi_graph_part&      = default;
  constexpr auto operator=(bidi_graph_part const&) noexcept -> bidi_graph_part& = delete;

  [[nodiscard]] constexpr auto view() const noexcept -> bidi_graph_part_view<part_t>
  {
    return {
      const_cast<part_t*>(&part),          local_m_fw, local_m_bw, const_cast<index_t*>(fw.head.data()), const_cast<vertex_t*>(fw.csr.data()), const_cast<index_t*>(bw.head.data()),
      const_cast<vertex_t*>(bw.csr.data())
    };
  }

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return view().csr_range(u);
  }

  [[nodiscard]] constexpr auto bw_csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return view().bw_csr_range(u);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    view().each_u(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    view().each_v(u, std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_bw_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    view().each_bw_v(u, std::forward<Consumer>(consumer));
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return view().outdegree(u);
  }

  [[nodiscard]] constexpr auto indegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return view().indegree(u);
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_uv(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_bw_uv(std::forward<Consumer>(consumer));
  }

  constexpr void debug_validate() const noexcept
  {
    view().debug_validate();
  }
};

}
