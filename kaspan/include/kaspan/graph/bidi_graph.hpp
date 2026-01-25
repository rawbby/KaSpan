#pragma once

#include <kaspan/graph/graph.hpp>

namespace kaspan {

/**
 * @brief A non-mutable view of a bidirectional graph in CSR format.
 *
 * Provides read-only access to both forward (fw) and backward (bw) edges.
 * Reuses graph_view for each direction.
 *
 * Each direction follows the CSR format:
 * - head: An array of offsets of size n + 1 (if n > 0).
 * - csr:  An array of neighbor vertex IDs of size m.
 */
struct bidi_graph_view
{
  vertex_t n = 0; ///< Number of vertices
  index_t  m = 0; ///< Number of edges (in each direction)
  struct
  {
    index_t*  head = nullptr; ///< Forward offsets
    vertex_t* csr  = nullptr; ///< Forward neighbors
  } fw{};
  struct
  {
    index_t*  head = nullptr; ///< Backward offsets
    vertex_t* csr  = nullptr; ///< Backward neighbors
  } bw{};

  constexpr bidi_graph_view() noexcept  = default;
  constexpr ~bidi_graph_view() noexcept = default;

  constexpr bidi_graph_view(bidi_graph_view&&) noexcept      = default;
  constexpr bidi_graph_view(bidi_graph_view const&) noexcept = default;

  constexpr bidi_graph_view(
    vertex_t  n,
    index_t   m,
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
    debug_check();
  }

  constexpr auto operator=(bidi_graph_view&&) noexcept -> bidi_graph_view&      = default;
  constexpr auto operator=(bidi_graph_view const&) noexcept -> bidi_graph_view& = default;

  /**
   * @brief Create a view of the forward graph.
   */
  [[nodiscard]] constexpr auto fw_view() const noexcept -> graph_view
  {
    return { n, m, fw.head, fw.csr };
  }

  /**
   * @brief Create a view of the backward graph.
   */
  [[nodiscard]] constexpr auto bw_view() const noexcept -> graph_view
  {
    return { n, m, bw.head, bw.csr };
  }

  /**
   * @brief Create a view with forward and backward directions swapped.
   */
  [[nodiscard]] constexpr auto inverse_view() const noexcept -> bidi_graph_view
  {
    return { n, m, bw.head, bw.csr, fw.head, fw.csr };
  }

  /**
   * @brief Get the forward neighbors of vertex u.
   */
  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t>
  {
    return fw_view().csr_range(u);
  }

  /**
   * @brief Get the backward neighbors of vertex u.
   */
  [[nodiscard]] constexpr auto bw_csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t>
  {
    return bw_view().csr_range(u);
  }

  /**
   * @brief Iterate over each vertex u.
   */
  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_u(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward neighbor v of vertex u.
   */
  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    fw_view().each_v(u, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward neighbor v of vertex u.
   */
  template<std::invocable<vertex_t> Consumer>
  constexpr void each_bw_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    bw_view().each_v(u, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Get the outdegree of vertex u.
   */
  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return fw_view().outdegree(u);
  }

  /**
   * @brief Get the indegree of vertex u.
   */
  [[nodiscard]] constexpr auto indegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return bw_view().outdegree(u);
  }

  /**
   * @brief Iterate over each forward edge (u, v).
   */
  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward edge (u, v).
   */
  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    bw_view().each_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Perform basic checks via directional views.
   */
  constexpr void debug_check() const noexcept
  {
    fw_view().debug_check();
    bw_view().debug_check();
  }

  /**
   * @brief Perform deep validation via directional views.
   */
  constexpr void debug_validate() const noexcept
  {
    fw_view().debug_validate();
    bw_view().debug_validate();
  }
};

/**
 * @brief An owning bidirectional graph structure.
 */
struct bidi_graph
{
  vertex_t n = 0; ///< Number of vertices
  index_t  m = 0; ///< Number of edges (in each direction)
  struct
  {
    index_t*  head = nullptr; ///< Forward offsets
    vertex_t* csr  = nullptr; ///< Forward neighbors
  } fw{};
  struct
  {
    index_t*  head = nullptr; ///< Backward offsets
    vertex_t* csr  = nullptr; ///< Backward neighbors
  } bw{};

  constexpr bidi_graph() noexcept = default;

  bidi_graph(
    vertex_t n,
    index_t  m)
    : n(n)
    , m(m)
    , fw{ line_alloc<index_t>(n == 0 ? 0 : n + 1),
          line_alloc<vertex_t>(m) }
    , bw{ line_alloc<index_t>(n == 0 ? 0 : n + 1),
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

  /**
   * @brief Create a non-mutable view of the bidirectional graph.
   */
  [[nodiscard]] auto view() const noexcept -> bidi_graph_view
  {
    return { n, m, fw.head, fw.csr, bw.head, bw.csr };
  }

  /**
   * @brief Create a view of the forward graph.
   */
  [[nodiscard]] constexpr auto fw_view() const noexcept -> graph_view
  {
    return view().fw_view();
  }

  /**
   * @brief Create a view of the backward graph.
   */
  [[nodiscard]] constexpr auto bw_view() const noexcept -> graph_view
  {
    return view().bw_view();
  }

  /**
   * @brief Create a view with forward and backward directions swapped.
   */
  [[nodiscard]] constexpr auto inverse_view() const noexcept -> bidi_graph_view
  {
    return view().inverse_view();
  }

  /**
   * @brief Get the forward neighbors of vertex u.
   */
  [[nodiscard]] constexpr auto csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t>
  {
    return view().csr_range(u);
  }

  /**
   * @brief Get the backward neighbors of vertex u.
   */
  [[nodiscard]] constexpr auto bw_csr_range(
    vertex_t u) const noexcept -> std::span<vertex_t>
  {
    return view().bw_csr_range(u);
  }

  /**
   * @brief Iterate over each vertex u.
   */
  template<std::invocable<vertex_t> Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    view().each_u(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward neighbor v of vertex u.
   */
  template<std::invocable<vertex_t> Consumer>
  constexpr void each_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    view().each_v(u, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward neighbor v of vertex u.
   */
  template<std::invocable<vertex_t> Consumer>
  constexpr void each_bw_v(
    vertex_t   u,
    Consumer&& consumer) const noexcept
  {
    view().each_bw_v(u, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Get the outdegree of vertex u.
   */
  [[nodiscard]] constexpr auto outdegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return view().outdegree(u);
  }

  /**
   * @brief Get the indegree of vertex u.
   */
  [[nodiscard]] constexpr auto indegree(
    vertex_t u) const noexcept -> vertex_t
  {
    return view().indegree(u);
  }

  /**
   * @brief Iterate over each forward edge (u, v).
   */
  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward edge (u, v).
   */
  template<std::invocable<vertex_t,
                          vertex_t> Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_bw_uv(std::forward<Consumer>(consumer));
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
