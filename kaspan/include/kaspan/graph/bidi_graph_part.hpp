#pragma once

#include <kaspan/graph/graph_part.hpp>

namespace kaspan {

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

} // namespace kaspan
