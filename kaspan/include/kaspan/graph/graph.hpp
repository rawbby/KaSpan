#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/memory/line.hpp>

#include <concepts>
#include <span>

namespace kaspan {

/**
 * @brief A non-mutable view of a graph in Compressed Sparse Row (CSR) format.
 *
 * This struct provides read-only access to graph data. It does not own the
 * underlying memory. The CSR is assumed to be sorted by vertex ID (u, then v).
 *
 * The CSR format consists of:
 * - head: An array of offsets of size n + 1 (if n > 0). The neighbors of vertex u
 *         are stored in csr[head[u]] to csr[head[u+1]-1].
 * - csr:  An array of neighbor vertex IDs of size m.
 */
struct graph_view
{
  vertex_t  n    = 0;       ///< Number of vertices
  index_t   m    = 0;       ///< Number of edges
  index_t*  head = nullptr; ///< Offsets into the CSR array (size n + 1)
  vertex_t* csr  = nullptr; ///< Neighbor array (size m)

  constexpr graph_view() noexcept  = default;
  constexpr ~graph_view() noexcept = default;

  constexpr graph_view(graph_view&&) noexcept      = default;
  constexpr graph_view(graph_view const&) noexcept = default;

  constexpr graph_view(
    integral_c auto n,
    integral_c auto m,
    index_t*                head,
    vertex_t*               csr) noexcept
    : n(integral_cast<vertex_t>(n))
    , m(integral_cast<index_t>(m))
    , head(head)
    , csr(csr)
  {
    debug_check();
  }

  constexpr auto operator=(graph_view&&) noexcept -> graph_view&      = default;
  constexpr auto operator=(graph_view const&) noexcept -> graph_view& = default;

  /**
   * @brief Get the neighbors of vertex u.
   * @param u Source vertex (global index).
   * @return A span of neighbors.
   */
  [[nodiscard]] constexpr auto csr_range(
    integral_c auto u) const noexcept -> std::span<vertex_t>
  {
    auto const j = integral_cast<vertex_t>(u);
    return { csr + head[j], csr + head[j + 1] };
  }

  /**
   * @brief Iterate over each vertex u in the graph.
   * @param consumer A function taking a vertex_t u.
   */
  template<typename Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t u = 0; u < n; ++u)
      consumer(u);
  }

  /**
   * @brief Iterate over each local vertex k.
   * @param consumer A function taking a vertex_t k.
   */
  template<typename Consumer>
  constexpr void each_k(
    Consumer&& consumer) const noexcept
  {
    for (vertex_t k = 0; k < n; ++k)
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
    for (vertex_t k = 0; k < n; ++k)
      consumer(k, k);
  }

  /**
   * @brief Iterate over each neighbor v of vertex u.
   * @param u Source vertex.
   * @param consumer A function taking a vertex_t v.
   */
  template<typename Consumer>
  constexpr void each_v(
    integral_c auto u,
    Consumer&&              consumer) const noexcept
  {
    for (vertex_t v : csr_range(u))
      consumer(v);
  }

  /**
   * @brief Iterate over each edge (u, v) starting from vertex u.
   * @param u Source vertex.
   * @param consumer A function taking (vertex_t u, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_uv(
    integral_c auto u,
    Consumer&&              consumer) const noexcept
  {
    auto const i = integral_cast<vertex_t>(u);
    each_v(i, [&](vertex_t v) { consumer(i, v); });
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
   * @brief Get the outdegree of vertex u.
   * @param u Source vertex.
   * @return The number of outgoing edges from u.
   */
  [[nodiscard]] constexpr auto outdegree(
    integral_c auto u) const noexcept -> vertex_t
  {
    auto const j = integral_cast<vertex_t>(u);
    return integral_cast<vertex_t>(head[j + 1] - head[j]);
  }

  /**
   * @brief Iterate over each edge (u, v) in the graph.
   * @param consumer A function taking (vertex_t u, vertex_t v).
   */
  template<typename Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    each_u([&](vertex_t u) { each_v(u, [&](vertex_t v) { consumer(u, v); }); });
  }

  /**
   * @brief Perform basic checks on memory addressability and pointer/size consistency.
   *
   * This class follows the convention that a size is zero if and only if the
   * corresponding pointer is nullptr.
   */
  constexpr void debug_check() const noexcept
  {
    if (KASPAN_MEMCHECK && !std::is_constant_evaluated()) {
      if (n > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(head, (n + 1) * sizeof(index_t));
      if (m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(csr, m * sizeof(vertex_t));
    }
    if (KASPAN_DEBUG) {
      ASSERT_GE(n, 0);
      ASSERT_GE(m, 0);
      ASSERT_SIZE_POINTER(n, head);
      ASSERT_SIZE_POINTER(m, csr);
    }
  }

  /**
   * @brief Perform deep validation of the graph structure.
   *
   * Verifies that offsets are consistent and edges are sorted.
   * Fails if the graph representation is invalid.
   */
  constexpr void debug_validate() const noexcept
  {
    if (KASPAN_MEMCHECK) {
      if (n > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(head, (n + 1) * sizeof(index_t));
      if (m > 0) KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(csr, m * sizeof(vertex_t));
    }
    if (KASPAN_DEBUG) {
      // validate memory consistency
      debug_check();

      // validate logical consistency

      if (n > 0) {
        DEBUG_ASSERT_EQ(head[0], 0);
        DEBUG_ASSERT_EQ(head[n], m);
      }

      index_t last_offset = 0;
      each_u([&](auto u) {
        ASSERT_LE(last_offset, head[u + 1]);
        last_offset = head[u + 1];
      });

      vertex_t prev_u = 0;
      // vertex_t prev_v = 0;
      each_uv([&](auto u, auto v) {
        ASSERT_LT(u, n);
        ASSERT_LT(v, n);
        ASSERT_LE(prev_u, u);
        // if (prev_u == u) ASSERT_LE(prev_v, v);
        prev_u = u;
        // prev_v = v;
      });
    }
  }
};

/**
 * @brief An owning graph structure in CSR format.
 *
 * This struct owns the memory for the head and csr arrays and provides
 * a view onto the data.
 */
struct graph
{
  vertex_t  n    = 0;       ///< Number of vertices
  index_t   m    = 0;       ///< Number of edges
  index_t*  head = nullptr; ///< Offsets (size n + 1)
  vertex_t* csr  = nullptr; ///< Neighbors (size m)

  constexpr graph() noexcept = default;

  graph(
    integral_c auto n,
    integral_c auto m)
    : n(integral_cast<vertex_t>(n))
    , m(integral_cast<index_t>(m))
    , head(line_alloc<index_t>(n ? n + 1 : 0))
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

  /**
   * @brief Create a non-mutable view of the graph.
   */
  [[nodiscard]] auto view() const noexcept -> graph_view
  {
    return { n, m, head, csr };
  }

  /**
   * @brief Get the neighbors of vertex u.
   */
  [[nodiscard]] constexpr auto csr_range(
    integral_c auto u) const noexcept -> std::span<vertex_t>
  {
    return view().csr_range(u);
  }

  /**
   * @brief Iterate over each vertex u.
   */
  template<typename Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    view().each_u(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each local vertex k.
   */
  template<typename Consumer>
  constexpr void each_k(
    Consumer&& consumer) const noexcept
  {
    view().each_k(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each local vertex k and its corresponding global vertex u.
   */
  template<typename Consumer>
  constexpr void each_ku(
    Consumer&& consumer) const noexcept
  {
    view().each_ku(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each neighbor v of vertex u.
   */
  template<typename Consumer>
  constexpr void each_v(
    integral_c auto u,
    Consumer&&              consumer) const noexcept
  {
    view().each_v(u, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each edge (u, v) starting from vertex u.
   */
  template<typename Consumer>
  constexpr void each_uv(
    integral_c auto u,
    Consumer&&              consumer) const noexcept
  {
    view().each_uv(u, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each local-global edge (k, v).
   */
  template<typename Consumer>
  constexpr void each_kv(
    Consumer&& consumer) const noexcept
  {
    view().each_kv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each local-global-global edge (k, u, v).
   */
  template<typename Consumer>
  constexpr void each_kuv(
    Consumer&& consumer) const noexcept
  {
    view().each_kuv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Get the outdegree of vertex u.
   */
  [[nodiscard]] constexpr auto outdegree(
    integral_c auto u) const noexcept -> vertex_t
  {
    return view().outdegree(u);
  }

  /**
   * @brief Iterate over each edge (u, v).
   */
  template<typename Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Perform basic checks via the view.
   */
  constexpr void debug_check() const noexcept
  {
    view().debug_check();
  }

  /**
   * @brief Perform deep validation via the view.
   */
  constexpr void debug_validate() const noexcept
  {
    view().debug_validate();
  }
};

} // namespace kaspan
