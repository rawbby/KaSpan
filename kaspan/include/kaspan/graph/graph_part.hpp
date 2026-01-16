#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/line.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>

#include <concepts>
#include <span>

namespace kaspan {

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

} // namespace kaspan
