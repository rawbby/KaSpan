#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/memory/line.hpp>

#include <concepts>
#include <span>

namespace kaspan {

/**
 * @brief A non-mutable view of a partitioned graph in CSR format.
 *
 * This struct provides read-only access to local graph data. It does not own the
 * underlying memory. The CSR is assumed to be sorted by vertex ID (u, then v).
 *
 * The CSR format consists of:
 * - head: An array of local offsets of size local_n + 1 (if local_n > 0).
 *         The neighbors of local vertex k are stored in csr[head[k]] to csr[head[k+1]-1].
 * - csr:  An array of neighbor vertex global IDs of size local_m.
 */
template<part_view_concept Part>
struct graph_part_view
{
  Part      part    = nullptr;
  index_t   local_m = 0;
  index_t*  head    = nullptr;
  vertex_t* csr     = nullptr;

  constexpr graph_part_view() noexcept  = default;
  constexpr ~graph_part_view() noexcept = default;

  constexpr graph_part_view(graph_part_view&&) noexcept      = default;
  constexpr graph_part_view(graph_part_view const&) noexcept = default;

  constexpr graph_part_view(
    Part                    part,
    arithmetic_concept auto local_m,
    index_t*                head,
    vertex_t*               csr) noexcept
    : part(part)
    , local_m(integral_cast<index_t>(local_m))
    , head(head)
    , csr(csr)
  {
    debug_check();
  }

  constexpr auto operator=(graph_part_view&&) noexcept -> graph_part_view&      = default;
  constexpr auto operator=(graph_part_view const&) noexcept -> graph_part_view& = default;

  [[nodiscard]] constexpr auto csr_range(
    arithmetic_concept auto k) const noexcept -> std::span<vertex_t>
  {
    auto const l = integral_cast<vertex_t>(k);
    return { csr + head[l], csr + head[l + 1] };
  }

  /**
   * @brief Iterate over each local vertex k.
   * @param consumer A function taking a vertex_t k.
   */
  template<typename Consumer>
  constexpr void each_k(
    Consumer&& consumer) const noexcept
  {
    auto const local_n = part.local_n();
    for (vertex_t k = 0; k < local_n; ++k)
      consumer(k);
  }

  /**
   * @brief Iterate over each local vertex k and its corresponding global vertex u.
   * @param consumer A function taking (vertex_t k, vertex_t u).
   */
  template<typename Consumer>
  constexpr void each_ku(
    Consumer&& consumer) const noexcept
  {
    each_k([&](vertex_t k) { consumer(k, part.to_global(k)); });
  }

  /**
   * @brief Iterate over each global neighbor v of local vertex k.
   * @param k Local source vertex.
   * @param consumer A function taking a vertex_t v.
   */
  template<typename Consumer>
  constexpr void each_v(
    arithmetic_concept auto k,
    Consumer&&              consumer) const noexcept
  {
    for (vertex_t v : csr_range(k))
      consumer(v);
  }

  /**
   * @brief Iterate over each global neighbor v of all local source vertices.
   * @param consumer A function taking a vertex_t v.
   */
  template<typename Consumer>
  constexpr void each_v(
    Consumer&& consumer) const noexcept
  {
    each_kv([&](vertex_t /* k */, vertex_t v) { consumer(v); });
  }

  /**
   * @brief Iterate over each global vertex u and its corresponding global neighbor v.
   * @param k Local source vertex index.
   * @param consumer A function taking (vertex_t u, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_uv(
    arithmetic_concept auto k,
    Consumer&&              consumer) const noexcept
  {
    auto const u = part.to_global(k);
    each_v(k, [&](vertex_t v) { consumer(u, v); });
  }

  /**
   * @brief Iterate over each global vertex u and its corresponding global neighbor v.
   * @param consumer A function taking (vertex_t u, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    each_kuv([&](vertex_t /* k */, vertex_t u, vertex_t v) { consumer(u, v); });
  }

  /**
   * @brief Iterate over each global vertex u.
   * @param consumer A function taking a vertex_t u.
   */
  template<typename Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    each_k([&](vertex_t k) { consumer(part.to_global(k)); });
  }

  /**
   * @brief Get the outdegree of local vertex k.
   * @param k Local source vertex.
   * @return The number of outgoing edges from k.
   */
  [[nodiscard]] constexpr auto outdegree(
    arithmetic_concept auto k) const noexcept -> vertex_t
  {
    auto const l = integral_cast<vertex_t>(k);
    return integral_cast<vertex_t>(head[l + 1] - head[l]);
  }

  /**
   * @brief Iterate over each local-global edge (k, v).
   * @param consumer A function taking (vertex_t k, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_kv(
    Consumer&& consumer) const noexcept
  {
    each_k([&](vertex_t k) { each_v(k, [&](vertex_t v) { consumer(k, v); }); });
  }

  /**
   * @brief Iterate over each local-global-global edge (k, u, v).
   * @param consumer A function taking (vertex_t k, vertex_t u, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_kuv(
    Consumer&& consumer) const noexcept
  {
    each_ku([&](vertex_t k, vertex_t u) { each_v(k, [&](vertex_t v) { consumer(k, u, v); }); });
  }

  /**
   * @brief Perform basic checks on memory addressability and pointer/size consistency.
   *
   * This class follows the convention that a size is zero if and only if the
   * corresponding pointer is nullptr.
   */
  constexpr void debug_check() const noexcept
  {
    auto const local_n = part.local_n();
    if (KASPAN_MEMCHECK && !std::is_constant_evaluated()) {
      if (local_n > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, (local_n + 1) * sizeof(index_t));
      if (local_m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(csr, local_m * sizeof(vertex_t));
    }
    if (KASPAN_DEBUG) {
      ASSERT_GE(part.n(), 0);
      ASSERT_GE(local_n, 0);
      ASSERT_GE(local_m, 0);
      ASSERT_SIZE_POINTER(local_n, head);
      ASSERT_SIZE_POINTER(local_m, csr);
    }
  }

  constexpr void debug_validate() const noexcept
  {
    auto const local_n = part.local_n();
    if (KASPAN_MEMCHECK) {
      if (local_n > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(head, (local_n + 1) * sizeof(index_t));
      if (local_m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(csr, local_m * sizeof(vertex_t));
    }
    if (KASPAN_DEBUG) {
      // validate memory consistency
      debug_check();

      // validate logical consistency

      if (local_n > 0) {
        DEBUG_ASSERT_EQ(head[0], 0);
        DEBUG_ASSERT_EQ(head[local_n], local_m);
      }

      index_t last_offset = 0;
      each_k([&](auto k) {
        ASSERT_LE(last_offset, head[k + 1]);
        last_offset = head[k + 1];
      });

      vertex_t prev_k = 0;
      // vertex_t prev_v = 0;
      each_kuv([&](auto k, auto u, auto v) {
        ASSERT_LT(k, local_n);
        ASSERT_LT(u, part.n());
        ASSERT_LT(v, part.n());
        ASSERT_LE(prev_k, k);
        // if (prev_k == k) ASSERT_LE(prev_v, v);
        prev_k = k;
        // prev_v = v;
      });
    }
  }
};

template<part_concept Part>
struct graph_part
{
  Part      part{};
  index_t   local_m = 0;
  index_t*  head    = nullptr;
  vertex_t* csr     = nullptr;

  constexpr graph_part() noexcept = default;

  graph_part(
    Part                    part,
    arithmetic_concept auto local_m)
    : part(std::move(part))
    , local_m(integral_cast<index_t>(local_m))
    , head(line_alloc<index_t>(part.local_n() ? part.local_n() + 1 : 0))
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
    : part(std::move(rhs.part))
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
      part     = std::move(rhs.part);
      local_m  = rhs.local_m;
      head     = rhs.head;
      csr      = rhs.csr;
      rhs.head = nullptr;
      rhs.csr  = nullptr;
    }
    return *this;
  }
  constexpr auto operator=(graph_part const&) noexcept -> graph_part& = delete;

  [[nodiscard]] auto view() const noexcept -> graph_part_view<decltype(part.view())>
  {
    return { part.view(), local_m, head, csr };
  }

  [[nodiscard]] constexpr auto csr_range(
    arithmetic_concept auto k) const noexcept -> std::span<vertex_t>
  {
    return view().csr_range(k);
  }

  /**
   * @brief Iterate over each local vertex k.
   * @param consumer A function taking a vertex_t k.
   */
  template<typename Consumer>
  constexpr void each_k(
    Consumer&& consumer) const noexcept
  {
    view().each_k(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each local vertex k and its corresponding global vertex u.
   * @param consumer A function taking (vertex_t k, vertex_t u).
   */
  template<typename Consumer>
  constexpr void each_ku(
    Consumer&& consumer) const noexcept
  {
    view().each_ku(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each global neighbor v of local vertex k.
   * @param k Local source vertex.
   * @param consumer A function taking a vertex_t v.
   */
  template<typename Consumer>
  constexpr void each_v(
    arithmetic_concept auto k,
    Consumer&&              consumer) const noexcept
  {
    view().each_v(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each global neighbor v of all local source vertices.
   * @param consumer A function taking a vertex_t v.
   */
  template<typename Consumer>
  constexpr void each_v(
    Consumer&& consumer) const noexcept
  {
    view().each_v(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each global vertex u and its corresponding global neighbor v.
   * @param k Local source vertex.
   * @param consumer A function taking (vertex_t u, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_uv(
    arithmetic_concept auto k,
    Consumer&&              consumer) const noexcept
  {
    view().each_uv(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each global vertex u and its corresponding global neighbor v.
   * @param consumer A function taking (vertex_t u, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each global vertex u.
   * @param consumer A function taking a vertex_t u.
   */
  template<typename Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    view().each_u(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Get the outdegree of local vertex k.
   * @param k Local source vertex.
   * @return The number of outgoing edges from k.
   */
  [[nodiscard]] constexpr auto outdegree(
    arithmetic_concept auto k) const noexcept -> vertex_t
  {
    return view().outdegree(k);
  }

  /**
   * @brief Iterate over each local-global edge (k, v).
   * @param consumer A function taking (vertex_t k, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_kv(
    Consumer&& consumer) const noexcept
  {
    view().each_kv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each local-global-global edge (k, u, v).
   * @param consumer A function taking (vertex_t k, vertex_t u, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_kuv(
    Consumer&& consumer) const noexcept
  {
    view().each_kuv(std::forward<Consumer>(consumer));
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
