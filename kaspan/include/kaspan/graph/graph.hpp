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
  vertex_t       n    = 0;
  vertex_t       m    = 0;
  index_t const* head = nullptr;
  vertex_t const* csr  = nullptr;

  constexpr graph_view() noexcept  = default;
  constexpr ~graph_view() noexcept = default;

  constexpr graph_view(graph_view&&) noexcept      = default;
  constexpr graph_view(graph_view const&) noexcept = default;

  constexpr graph_view(
    vertex_t       n,
    vertex_t       m,
    index_t const* head,
    vertex_t const* csr) noexcept
    : n(n)
    , m(m)
    , head(head)
    , csr(csr)
  {
    debug_check();
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

  constexpr void debug_check() const noexcept
  {
    DEBUG_ASSERT_GE(n, 0);
    DEBUG_ASSERT_GE(m, 0);
    DEBUG_ASSERT_SIZE_POINTER(n, head);
    DEBUG_ASSERT_SIZE_POINTER(m, csr);
    if (!std::is_constant_evaluated()) {
      if (n > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, (n + 1) * sizeof(index_t));
      if (m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(csr, m * sizeof(vertex_t));
    }
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
        if (m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(csr, m * sizeof(vertex_t));
      }
    }

    if (n > 0) {
      // validate sorted head
      {
        IF(KASPAN_DEBUG, index_t last_offset = head[0]);
        for (vertex_t u = 0; u < n; ++u) {
          DEBUG_ASSERT_LE(last_offset, head[u + 1]);
          IF(KASPAN_DEBUG, last_offset = head[u + 1]);
        }
      }

      // validate sorted csr
      {
        each_u([&](vertex_t u) {
          auto const range = csr_range(u);
          for (size_t i = 1; i < range.size(); ++i) {
            DEBUG_ASSERT_LE(range[i - 1], range[i]);
          }
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
    debug_check();
  }
  ~graph() noexcept
  {
    line_free(head);
    line_free(csr);
  }

  constexpr graph(
    graph&& rhs) noexcept
    : n(rhs.n)
    , m(rhs.m)
    , head(rhs.head)
    , csr(rhs.csr)
  {
    rhs.head = nullptr;
    rhs.csr  = nullptr;
  }
  constexpr graph(graph const&) noexcept = delete;

  constexpr auto operator=(
    graph&& rhs) noexcept -> graph&
  {
    if (this != &rhs) {
      line_free(head);
      line_free(csr);
      n        = rhs.n;
      m        = rhs.m;
      head     = rhs.head;
      csr      = rhs.csr;
      rhs.head = nullptr;
      rhs.csr  = nullptr;
    }
    return *this;
  }
  constexpr auto operator=(graph const&) noexcept -> graph& = delete;

  [[nodiscard]] constexpr auto view() const noexcept -> graph_view
  {
    return { n, m, head, csr };
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

  constexpr void debug_check() const noexcept
  {
    view().debug_check();
  }

  constexpr void debug_validate() const noexcept
  {
    view().debug_validate();
  }
};

template<world_part_concept Part>
struct graph_part_view
{
  using part_t         = Part;
  part_t const*  part    = nullptr;
  vertex_t       local_m = 0;
  index_t const* head    = nullptr;
  vertex_t const* csr     = nullptr;

  constexpr graph_part_view() noexcept  = default;
  constexpr ~graph_part_view() noexcept = default;

  constexpr graph_part_view(graph_part_view&&) noexcept      = default;
  constexpr graph_part_view(graph_part_view const&) noexcept = default;

  constexpr graph_part_view(
    part_t const*  part,
    vertex_t       local_m,
    index_t const* head,
    vertex_t const* csr) noexcept
    : part(part)
    , local_m(local_m)
    , head(head)
    , csr(csr)
  {
    debug_check();
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

  constexpr void debug_check() const noexcept
  {
    DEBUG_ASSERT_POINTER(part);
    DEBUG_ASSERT_GE(part->n, 0);
    DEBUG_ASSERT_GE(part->local_n(), 0);
    DEBUG_ASSERT_SIZE_POINTER(part->local_n(), head);
    DEBUG_ASSERT_SIZE_POINTER(local_m, csr);
    if (!std::is_constant_evaluated()) {
      if (part->local_n() > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, (part->local_n() + 1) * sizeof(index_t));
      if (local_m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(csr, local_m * sizeof(vertex_t));
    }
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
        if (local_m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(csr, local_m * sizeof(vertex_t));
      }
    }

    if (part->local_n() > 0) {
      // validate sorted head
      {
        IF(KASPAN_DEBUG, index_t last_offset = head[0]);
        for (vertex_t u = 0; u < part->local_n(); ++u) {
          DEBUG_ASSERT_LE(last_offset, head[u + 1]);
          IF(KASPAN_DEBUG, last_offset = head[u + 1]);
        }
      }

      // validate sorted csr
      {
        each_u([&](vertex_t u) {
          auto const range = csr_range(u);
          for (size_t i = 1; i < range.size(); ++i) {
            DEBUG_ASSERT_LE(range[i - 1], range[i]);
          }
        });
      }
    }
  }
};

template<world_part_concept Part>
struct graph_part
{
  using part_t = Part;
  part_t    part{};
  vertex_t  local_m = 0;
  index_t*  head    = nullptr;
  vertex_t* csr     = nullptr;

  constexpr graph_part() noexcept = default;

  graph_part(
    part_t   part,
    vertex_t local_m)
    : part(part)
    , local_m(local_m)
    , head(line_alloc<index_t>(part.local_n() + 1))
    , csr(line_alloc<vertex_t>(local_m))
  {
    debug_check();
  }

  ~graph_part() noexcept
  {
    line_free(head);
    line_free(csr);
  }

  constexpr graph_part(
    graph_part&& rhs) noexcept
    : part(rhs.part)
    , local_m(rhs.local_m)
    , head(rhs.head)
    , csr(rhs.csr)
  {
    rhs.head = nullptr;
    rhs.csr  = nullptr;
  }
  constexpr graph_part(graph_part const&) noexcept = delete;

  constexpr auto operator=(
    graph_part&& rhs) noexcept -> graph_part&
  {
    if (this != &rhs) {
      line_free(head);
      line_free(csr);
      part     = rhs.part;
      local_m  = rhs.local_m;
      head     = rhs.head;
      csr      = rhs.csr;
      rhs.head = nullptr;
      rhs.csr  = nullptr;
    }
    return *this;
  }
  constexpr auto operator=(graph_part const&) noexcept -> graph_part& = delete;

  [[nodiscard]] constexpr auto view() const noexcept -> graph_part_view<part_t>
  {
    return { &part, local_m, head, csr };
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

  constexpr void debug_check() const noexcept
  {
    view().debug_check();
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
    index_t const*  head = nullptr;
    vertex_t const* csr  = nullptr;
  } fw{};
  struct
  {
    index_t const*  head = nullptr;
    vertex_t const* csr  = nullptr;
  } bw{};

  constexpr bidi_graph_view() noexcept  = default;
  constexpr ~bidi_graph_view() noexcept = default;

  constexpr bidi_graph_view(bidi_graph_view&&) noexcept      = default;
  constexpr bidi_graph_view(bidi_graph_view const&) noexcept = default;

  constexpr bidi_graph_view(
    vertex_t       n,
    vertex_t       m,
    index_t const* fw_head,
    vertex_t const* fw_csr,
    index_t const* bw_head,
    vertex_t const* bw_csr) noexcept
    : n(n)
    , m(m)
    , fw{ fw_head,
          fw_csr }
    , bw{ bw_head,
          bw_csr }
  {
    debug_check();
  }

  constexpr auto operator=(bidi_graph_view&&) noexcept -> bidi_graph_view&      = default;
  constexpr auto operator=(bidi_graph_view const&) noexcept -> bidi_graph_view& = default;

  [[nodiscard]] constexpr auto fw_view() const noexcept -> graph_view
  {
    return { n, m, fw.head, fw.csr };
  }

  [[nodiscard]] constexpr auto bw_view() const noexcept -> graph_view
  {
    return { n, m, bw.head, bw.csr };
  }

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return fw_view().csr_range(u);
  }

  [[nodiscard]] constexpr auto bw_csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return bw_view().csr_range(u);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_u(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    fw_view().each_v(u, std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_bw_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    bw_view().each_v(u, std::forward<Consumer>(consumer));
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return fw_view().outdegree(u);
  }

  [[nodiscard]] constexpr auto indegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return bw_view().outdegree(u);
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_uv(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    bw_view().each_uv(std::forward<Consumer>(consumer));
  }

  constexpr void debug_check() const noexcept
  {
    fw_view().debug_check();
    bw_view().debug_check();
  }

  constexpr void debug_validate() const noexcept
  {
    fw_view().debug_validate();
    bw_view().debug_validate();
  }
};

struct bidi_graph
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

  constexpr bidi_graph() noexcept = default;

  bidi_graph(
    vertex_t n,
    vertex_t m)
    : n(n)
    , m(m)
    , fw{ line_alloc<index_t>(n + 1),
          line_alloc<vertex_t>(m) }
    , bw{ line_alloc<index_t>(n + 1),
          line_alloc<vertex_t>(m) }
  {
    debug_check();
  }

  ~bidi_graph() noexcept
  {
    line_free(fw.head);
    line_free(fw.csr);
    line_free(bw.head);
    line_free(bw.csr);
  }

  constexpr bidi_graph(
    bidi_graph&& rhs) noexcept
    : n(rhs.n)
    , m(rhs.m)
    , fw(rhs.fw)
    , bw(rhs.bw)
  {
    rhs.fw = {};
    rhs.bw = {};
  }
  constexpr bidi_graph(bidi_graph const&) noexcept = delete;

  constexpr auto operator=(
    bidi_graph&& rhs) noexcept -> bidi_graph&
  {
    if (this != &rhs) {
      line_free(fw.head);
      line_free(fw.csr);
      line_free(bw.head);
      line_free(bw.csr);
      n      = rhs.n;
      m      = rhs.m;
      fw     = rhs.fw;
      bw     = rhs.bw;
      rhs.fw = {};
      rhs.bw = {};
    }
    return *this;
  }
  constexpr auto operator=(bidi_graph const&) noexcept -> bidi_graph& = delete;

  [[nodiscard]] constexpr auto view() const noexcept -> bidi_graph_view
  {
    return { n, m, fw.head, fw.csr, bw.head, bw.csr };
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

  constexpr void debug_check() const noexcept
  {
    view().debug_check();
  }

  constexpr void debug_validate() const noexcept
  {
    view().debug_validate();
  }
};

template<world_part_concept Part>
struct bidi_graph_part_view
{
  using part_t          = Part;
  part_t const*  part       = nullptr;
  vertex_t       local_m_fw = 0;
  vertex_t       local_m_bw = 0;
  struct
  {
    index_t const*  head = nullptr;
    vertex_t const* csr  = nullptr;
  } fw{};
  struct
  {
    index_t const*  head = nullptr;
    vertex_t const* csr  = nullptr;
  } bw{};

  constexpr bidi_graph_part_view() noexcept  = default;
  constexpr ~bidi_graph_part_view() noexcept = default;

  constexpr bidi_graph_part_view(bidi_graph_part_view&&) noexcept      = default;
  constexpr bidi_graph_part_view(bidi_graph_part_view const&) noexcept = default;

  constexpr bidi_graph_part_view(
    part_t const*  part,
    vertex_t       local_m_fw,
    vertex_t       local_m_bw,
    index_t const* fw_head,
    vertex_t const* fw_csr,
    index_t const* bw_head,
    vertex_t const* bw_csr) noexcept
    : part(part)
    , local_m_fw(local_m_fw)
    , local_m_bw(local_m_bw)
    , fw{ fw_head,
          fw_csr }
    , bw{ bw_head,
          bw_csr }
  {
    debug_check();
  }

  constexpr auto operator=(bidi_graph_part_view&&) noexcept -> bidi_graph_part_view&      = default;
  constexpr auto operator=(bidi_graph_part_view const&) noexcept -> bidi_graph_part_view& = default;

  [[nodiscard]] constexpr auto fw_view() const noexcept -> graph_part_view<part_t>
  {
    return { part, local_m_fw, fw.head, fw.csr };
  }

  [[nodiscard]] constexpr auto bw_view() const noexcept -> graph_part_view<part_t>
  {
    return { part, local_m_bw, bw.head, bw.csr };
  }

  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return fw_view().csr_range(u);
  }

  [[nodiscard]] constexpr auto bw_csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t const>
  {
    return bw_view().csr_range(u);
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_u(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    fw_view().each_v(u, std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t> Consumer>
  constexpr void each_bw_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    bw_view().each_v(u, std::forward<Consumer>(consumer));
  }

  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return fw_view().outdegree(u);
  }

  [[nodiscard]] constexpr auto indegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return bw_view().outdegree(u);
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_uv(std::forward<Consumer>(consumer));
  }

  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    bw_view().each_uv(std::forward<Consumer>(consumer));
  }

  constexpr void debug_check() const noexcept
  {
    fw_view().debug_check();
    bw_view().debug_check();
  }

  constexpr void debug_validate() const noexcept
  {
    fw_view().debug_validate();
    bw_view().debug_validate();
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
    index_t*  head = nullptr;
    vertex_t* csr  = nullptr;
  } fw{};
  struct
  {
    index_t*  head = nullptr;
    vertex_t* csr  = nullptr;
  } bw{};

  constexpr bidi_graph_part() noexcept = default;

  bidi_graph_part(
    part_t   part,
    vertex_t local_m_fw,
    vertex_t local_m_bw)
    : part(part)
    , local_m_fw(local_m_fw)
    , local_m_bw(local_m_bw)
    , fw{ line_alloc<index_t>(part.local_n() + 1),
          line_alloc<vertex_t>(local_m_fw) }
    , bw{ line_alloc<index_t>(part.local_n() + 1),
          line_alloc<vertex_t>(local_m_bw) }
  {
    debug_check();
  }

  ~bidi_graph_part() noexcept
  {
    line_free(fw.head);
    line_free(fw.csr);
    line_free(bw.head);
    line_free(bw.csr);
  }

  constexpr bidi_graph_part(
    bidi_graph_part&& rhs) noexcept
    : part(rhs.part)
    , local_m_fw(rhs.local_m_fw)
    , local_m_bw(rhs.local_m_bw)
    , fw(rhs.fw)
    , bw(rhs.bw)
  {
    rhs.fw = {};
    rhs.bw = {};
  }
  constexpr bidi_graph_part(bidi_graph_part const&) noexcept = delete;

  constexpr auto operator=(
    bidi_graph_part&& rhs) noexcept -> bidi_graph_part&
  {
    if (this != &rhs) {
      line_free(fw.head);
      line_free(fw.csr);
      line_free(bw.head);
      line_free(bw.csr);
      part       = rhs.part;
      local_m_fw = rhs.local_m_fw;
      local_m_bw = rhs.local_m_bw;
      fw         = rhs.fw;
      bw         = rhs.bw;
      rhs.fw     = {};
      rhs.bw     = {};
    }
    return *this;
  }
  constexpr auto operator=(bidi_graph_part const&) noexcept -> bidi_graph_part& = delete;

  [[nodiscard]] constexpr auto view() const noexcept -> bidi_graph_part_view<part_t>
  {
    return { &part, local_m_fw, local_m_bw, fw.head, fw.csr, bw.head, bw.csr };
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

  constexpr void debug_check() const noexcept
  {
    view().debug_check();
  }

  constexpr void debug_validate() const noexcept
  {
    view().debug_validate();
  }
};

}
