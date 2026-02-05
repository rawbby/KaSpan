#pragma once

#include <kaspan/graph/graph_part.hpp>

namespace kaspan {

/**
 * @brief A non-mutable view of a partitioned bidirectional graph.
 *
 * Reuses graph_part_view for each direction.
 */
template<part_view_c Part>
struct bidi_graph_part_view
{
  Part    part       = {}; ///< The partition information
  index_t local_fw_m = 0;  ///< Number of local forward edges
  index_t local_bw_m = 0;  ///< Number of local backward edges
  struct
  {
    index_t*  head = nullptr; ///< Local forward offsets
    vertex_t* csr  = nullptr; ///< Forward neighbors (global IDs)
  } fw{};
  struct
  {
    index_t*  head = nullptr; ///< Local backward offsets
    vertex_t* csr  = nullptr; ///< Backward neighbors (global IDs)
  } bw{};

  constexpr bidi_graph_part_view() noexcept  = default;
  constexpr ~bidi_graph_part_view() noexcept = default;

  constexpr bidi_graph_part_view(bidi_graph_part_view&&) noexcept      = default;
  constexpr bidi_graph_part_view(bidi_graph_part_view const&) noexcept = default;

  constexpr bidi_graph_part_view(
    Part                    part,
    integral_c auto local_fw_m,
    integral_c auto local_bw_m,
    index_t*                fw_head,
    vertex_t*               fw_csr,
    index_t*                bw_head,
    vertex_t*               bw_csr) noexcept
    : part(part)
    , local_fw_m(integral_cast<index_t>(local_fw_m))
    , local_bw_m(integral_cast<index_t>(local_bw_m))
    , fw{ fw_head,
          fw_csr }
    , bw{ bw_head,
          bw_csr }
  {
    debug_check();
  }

  constexpr auto operator=(bidi_graph_part_view&&) noexcept -> bidi_graph_part_view&      = default;
  constexpr auto operator=(bidi_graph_part_view const&) noexcept -> bidi_graph_part_view& = default;

  /**
   * @brief Create a view of the forward partitioned graph.
   */
  [[nodiscard]] constexpr auto fw_view() const noexcept -> graph_part_view<Part>
  {
    return { part, local_fw_m, fw.head, fw.csr };
  }

  /**
   * @brief Create a view of the backward partitioned graph.
   */
  [[nodiscard]] constexpr auto bw_view() const noexcept -> graph_part_view<Part>
  {
    return { part, local_bw_m, bw.head, bw.csr };
  }

  /**
   * @brief Create a view with forward and backward directions swapped.
   */
  [[nodiscard]] constexpr auto inverse_view() const noexcept -> bidi_graph_part_view
  {
    return { part, local_bw_m, local_fw_m, bw.head, bw.csr, fw.head, fw.csr };
  }

  /**
   * @brief Get the forward neighbors of local vertex k.
   */
  [[nodiscard]] constexpr auto csr_range(
    integral_c auto k) const noexcept -> std::span<vertex_t>
  {
    return fw_view().csr_range(k);
  }

  /**
   * @brief Get the backward neighbors of local vertex k.
   */
  [[nodiscard]] constexpr auto bw_csr_range(
    integral_c auto k) const noexcept -> std::span<vertex_t>
  {
    return bw_view().csr_range(k);
  }

  /**
   * @brief Iterate over each local vertex k.
   */
  template<typename Consumer>
  constexpr void each_k(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_k(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each local vertex k and its corresponding global vertex u.
   */
  template<typename Consumer>
  constexpr void each_ku(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_ku(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward global neighbor v of local vertex k.
   */
  template<typename Consumer>
  constexpr void each_v(
    integral_c auto k,
    Consumer&&              consumer) const noexcept
  {
    fw_view().each_v(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward global neighbor v of all local source vertices.
   */
  template<typename Consumer>
  constexpr void each_v(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_v(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward global neighbor v of local vertex k.
   */
  template<typename Consumer>
  constexpr void each_bw_v(
    integral_c auto k,
    Consumer&&              consumer) const noexcept
  {
    bw_view().each_v(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward global neighbor v of all local source vertices.
   */
  template<typename Consumer>
  constexpr void each_bw_v(
    Consumer&& consumer) const noexcept
  {
    bw_view().each_v(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Get the outdegree of local vertex k.
   */
  [[nodiscard]] constexpr auto outdegree(
    integral_c auto k) const noexcept -> vertex_t
  {
    return fw_view().outdegree(k);
  }

  /**
   * @brief Get the indegree of local vertex k.
   */
  [[nodiscard]] constexpr auto indegree(
    integral_c auto k) const noexcept -> vertex_t
  {
    return bw_view().outdegree(k);
  }

  /**
   * @brief Iterate over each forward local-global edge (k, v).
   */
  template<typename Consumer>
  constexpr void each_kv(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_kv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward local-global edge (k, v).
   */
  template<typename Consumer>
  constexpr void each_bw_kv(
    Consumer&& consumer) const noexcept
  {
    bw_view().each_kv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward global vertex u and its corresponding global neighbor v.
   */
  template<typename Consumer>
  constexpr void each_uv(
    integral_c auto k,
    Consumer&&              consumer) const noexcept
  {
    fw_view().each_uv(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward global vertex u and its corresponding global neighbor v for all local source vertices.
   */
  template<typename Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward global vertex u and its corresponding global neighbor v.
   */
  template<typename Consumer>
  constexpr void each_bw_uv(
    integral_c auto k,
    Consumer&&              consumer) const noexcept
  {
    bw_view().each_uv(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward global vertex u and its corresponding global neighbor v for all local source vertices.
   */
  template<typename Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    bw_view().each_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each global vertex u.
   */
  template<typename Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_u(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward local-global-global edge (k, u, v).
   */
  template<typename Consumer>
  constexpr void each_kuv(
    Consumer&& consumer) const noexcept
  {
    fw_view().each_kuv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward local-global-global edge (k, u, v).
   */
  template<typename Consumer>
  constexpr void each_bw_kuv(
    Consumer&& consumer) const noexcept
  {
    bw_view().each_kuv(std::forward<Consumer>(consumer));
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
 * @brief An owning partitioned bidirectional graph structure.
 */
template<part_c Part>
struct bidi_graph_part
{
  Part    part{};         ///< The partition info
  index_t local_fw_m = 0; ///< Forward edge count
  index_t local_bw_m = 0; ///< Backward edge count
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

  constexpr bidi_graph_part() noexcept = default;

  bidi_graph_part(
    Part                    part,
    integral_c auto local_fw_m,
    integral_c auto local_bw_m)
    : part(std::move(part))
    , local_fw_m(integral_cast<index_t>(local_fw_m))
    , local_bw_m(integral_cast<index_t>(local_bw_m))
    , fw{ line_alloc<index_t>(part.local_n() ? part.local_n() + 1 : 0),
          line_alloc<vertex_t>(local_fw_m) }
    , bw{ line_alloc<index_t>(part.local_n() ? part.local_n() + 1 : 0),
          line_alloc<vertex_t>(local_bw_m) }
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
    : part(std::move(rhs.part))
    , local_fw_m(rhs.local_fw_m)
    , local_bw_m(rhs.local_bw_m)
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
      part       = std::move(rhs.part);
      local_fw_m = rhs.local_fw_m;
      local_bw_m = rhs.local_bw_m;
      fw         = rhs.fw;
      bw         = rhs.bw;
      rhs.fw     = {};
      rhs.bw     = {};
    }
    return *this;
  }
  constexpr auto operator=(bidi_graph_part const&) noexcept -> bidi_graph_part& = delete;

  /**
   * @brief Create a non-mutable view of the partitioned bidirectional graph.
   */
  [[nodiscard]] auto view() const noexcept -> bidi_graph_part_view<decltype(part.view())>
  {
    return { part.view(), local_fw_m, local_bw_m, fw.head, fw.csr, bw.head, bw.csr };
  }

  /**
   * @brief Create a view of the forward partitioned graph.
   */
  [[nodiscard]] constexpr auto fw_view() const noexcept -> graph_part_view<decltype(part.view())>
  {
    return view().fw_view();
  }

  /**
   * @brief Create a view of the backward partitioned graph.
   */
  [[nodiscard]] constexpr auto bw_view() const noexcept -> graph_part_view<decltype(part.view())>
  {
    return view().bw_view();
  }

  /**
   * @brief Get the forward neighbors of local vertex k.
   */
  [[nodiscard]] constexpr auto csr_range(
    integral_c auto k) const noexcept -> std::span<vertex_t>
  {
    return view().csr_range(k);
  }

  /**
   * @brief Get the backward neighbors of local vertex k.
   */
  [[nodiscard]] constexpr auto bw_csr_range(
    integral_c auto k) const noexcept -> std::span<vertex_t>
  {
    return view().bw_csr_range(k);
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
   * @brief Iterate over each forward neighbor v of local vertex k.
   */
  template<typename Consumer>
  constexpr void each_v(
    integral_c auto k,
    Consumer&&              consumer) const noexcept
  {
    view().each_v(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward neighbor v of all local source vertices.
   */
  template<typename Consumer>
  constexpr void each_v(
    Consumer&& consumer) const noexcept
  {
    view().each_v(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward neighbor v of local vertex k.
   */
  template<typename Consumer>
  constexpr void each_bw_v(
    integral_c auto k,
    Consumer&&              consumer) const noexcept
  {
    view().each_bw_v(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward neighbor v of all local source vertices.
   */
  template<typename Consumer>
  constexpr void each_bw_v(
    Consumer&& consumer) const noexcept
  {
    view().each_bw_v(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Get the outdegree of local vertex k.
   */
  [[nodiscard]] constexpr auto outdegree(
    integral_c auto k) const noexcept -> vertex_t
  {
    return view().outdegree(k);
  }

  /**
   * @brief Get the indegree of local vertex k.
   */
  [[nodiscard]] constexpr auto indegree(
    integral_c auto k) const noexcept -> vertex_t
  {
    return view().indegree(k);
  }

  /**
   * @brief Iterate over each forward local-global edge (k, v).
   */
  template<typename Consumer>
  constexpr void each_kv(
    Consumer&& consumer) const noexcept
  {
    view().each_kv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward global vertex u and its corresponding global neighbor v.
   */
  template<typename Consumer>
  constexpr void each_uv(
    integral_c auto k,
    Consumer&&              consumer) const noexcept
  {
    view().each_uv(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward global vertex u and its corresponding global neighbor v for all local source vertices.
   */
  template<typename Consumer>
  constexpr void each_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward global vertex u and its corresponding global neighbor v.
   */
  template<typename Consumer>
  constexpr void each_bw_uv(
    integral_c auto k,
    Consumer&&              consumer) const noexcept
  {
    view().each_bw_uv(k, std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward global vertex u and its corresponding global neighbor v for all local source vertices.
   */
  template<typename Consumer>
  constexpr void each_bw_uv(
    Consumer&& consumer) const noexcept
  {
    view().each_bw_uv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each global vertex u.
   */
  template<typename Consumer>
  constexpr void each_u(
    Consumer&& consumer) const noexcept
  {
    view().each_u(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward local-global edge (k, v).
   */
  template<typename Consumer>
  constexpr void each_bw_kv(
    Consumer&& consumer) const noexcept
  {
    view().each_bw_kv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each forward local-global-global edge (k, u, v).
   */
  template<typename Consumer>
  constexpr void each_kuv(
    Consumer&& consumer) const noexcept
  {
    view().each_kuv(std::forward<Consumer>(consumer));
  }

  /**
   * @brief Iterate over each backward local-global-global edge (k, u, v).
   */
  template<typename Consumer>
  constexpr void each_bw_kuv(
    Consumer&& consumer) const noexcept
  {
    view().each_bw_kuv(std::forward<Consumer>(consumer));
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
