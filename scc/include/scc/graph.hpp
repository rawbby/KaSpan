#pragma once

#include <memory/buffer.hpp>
#include <scc/base.hpp>
#include <scc/part.hpp>

#include <type_traits>

struct local_graph
{
  buffer    buffer;
  vertex_t  n       = 0;
  index_t   m       = 0;
  index_t*  fw_head = nullptr;
  vertex_t* fw_csr  = nullptr;
  index_t*  bw_head = nullptr;
  vertex_t* bw_csr  = nullptr;
};

template<WorldPartConcept Part>
struct local_graph_part
{
  buffer    buffer;
  Part      part;
  index_t   m          = 0;
  index_t   local_fw_m = 0;
  index_t   local_bw_m = 0;
  index_t*  fw_head    = nullptr;
  index_t*  bw_head    = nullptr;
  vertex_t* fw_csr     = nullptr;
  vertex_t* bw_csr     = nullptr;
};

struct graph
{
  vertex_t  n       = 0;
  index_t   m       = 0;
  index_t*  fw_head = nullptr;
  vertex_t* fw_csr  = nullptr;
  index_t*  bw_head = nullptr;
  vertex_t* bw_csr  = nullptr;

  graph()  = default;
  ~graph() = default;

  explicit graph(local_graph const& rhs)
    : n(rhs.n)
    , m(rhs.m)
    , fw_head(rhs.fw_head)
    , fw_csr(rhs.fw_csr)
    , bw_head(rhs.bw_head)
    , bw_csr(rhs.bw_csr)
  {
  }
};

template<WorldPartConcept Part>
struct graph_part
{
  Part      part;
  index_t   m          = 0;
  index_t   local_fw_m = 0;
  index_t   local_bw_m = 0;
  index_t*  fw_head    = nullptr;
  index_t*  bw_head    = nullptr;
  vertex_t* fw_csr     = nullptr;
  vertex_t* bw_csr     = nullptr;

  graph_part()  = default;
  ~graph_part() = default;

  explicit graph_part(local_graph_part<Part> const& rhs)
    : part(rhs.part)
    , m(rhs.m)
    , local_fw_m(rhs.local_fw_m)
    , local_bw_m(rhs.local_bw_m)
    , fw_head(rhs.fw_head)
    , bw_head(rhs.bw_head)
    , fw_csr(rhs.fw_csr)
    , bw_csr(rhs.bw_csr)
  {
  }
};

inline std::span<vertex_t const>
csr_range(index_t const* head, vertex_t const* csr, vertex_t k)
{
  return { csr + head[k], csr + head[k + 1] };
}

inline std::span<vertex_t>
csr_range(index_t const* head, const vertex_t* csr, vertex_t k)
{
  return { csr + head[k], csr + head[k + 1] };
}

#define DEBUG_ASSERT_VALID_GRAPH_LIGHT(N, M, HEAD, CSR)                                                                                                                            \
  DEBUG_ASSERT_NE(HEAD, nullptr);                                                                                                                                                  \
  DEBUG_ASSERT_NE(CSR, nullptr);                                                                                                                                                   \
  DEBUG_ASSERT_GE(N, 0);                                                                                                                                                           \
  DEBUG_ASSERT_GE(M, 0);                                                                                                                                                           \
  DEBUG_ASSERT_EQ((HEAD)[0], 0);                                                                                                                                                     \
  DEBUG_ASSERT_EQ((HEAD)[N], M);

#define DEBUG_ASSERT_VALID_GRAPH(N, M, HEAD, CSR)                                                                                                                                  \
  DEBUG_ASSERT_VALID_GRAPH_LIGHT(N, M, HEAD, CSR);                                                                                                                                 \
  IF(KASPAN_DEBUG, {                                                                                                                                                               \
    std::remove_cvref_t<decltype(N)> const _n = N;                                                                                                                                 \
    for (std::remove_cvref_t<decltype(N)> _i = 1; _i < _n; ++_i) { DEBUG_ASSERT_GE((HEAD)[_i], (HEAD)[_i - 1]); }                                                                      \
    std::remove_cvref_t<decltype(M)> const _m = M;                                                                                                                                 \
    for (std::remove_cvref_t<decltype(M)> _i = 1; _i < _m; ++_i) { DEBUG_ASSERT_IN_RANGE((CSR)[_i], 0, N); }                                                                         \
  });

#define DEBUG_ASSERT_VALID_GRAPH_PART_LIGHT(PART, HEAD, CSR)                                                                                                                       \
  DEBUG_ASSERT_NE(HEAD, nullptr);                                                                                                                                                  \
  DEBUG_ASSERT_NE(CSR, nullptr);                                                                                                                                                   \
  DEBUG_ASSERT_GE((PART).n, 0);                                                                                                                                                      \
  DEBUG_ASSERT_GE((PART).local_n(), 0);                                                                                                                                              \
  DEBUG_ASSERT_EQ((HEAD)[0], 0);

#define DEBUG_ASSERT_VALID_GRAPH_PART(PART, HEAD, CSR)                                                                                                                             \
  DEBUG_ASSERT_VALID_GRAPH_PART_LIGHT(PART, HEAD, CSR);                                                                                                                            \
  IF(KASPAN_DEBUG, {                                                                                                                                                               \
    std::remove_cvref_t<decltype((PART).local_n())> const _n = (PART).local_n();                                                                                                       \
    for (std::remove_cvref_t<decltype(_n)> _i = 1; _i < _n; ++_i) { DEBUG_ASSERT_GE((HEAD)[_i], (HEAD)[_i - 1]); }                                                                     \
    std::remove_cvref_t<decltype((HEAD)[_n])> const _m = (HEAD)[_n];                                                                                                                   \
    for (std::remove_cvref_t<decltype(_m)> _i = 1; _i < _m; ++_i) {                                                                                                                \
      DEBUG_ASSERT_GE((CSR)[_i], 0);                                                                                                                                                 \
      DEBUG_ASSERT_LT((CSR)[_i], (PART).n);                                                                                                                                            \
    }                                                                                                                                                                              \
  });

inline auto
make_graph_buffer(vertex_t n, index_t m) -> buffer
{
  return make_buffer<index_t, vertex_t, index_t, vertex_t>(n + 1, m, n + 1, m);
}

inline auto
make_fw_graph_buffer(vertex_t n, index_t m) -> buffer
{
  return make_buffer<index_t, vertex_t>(n + 1, m);
}

inline auto
make_graph_buffer(vertex_t n, index_t fw_m, index_t bw_m) -> buffer
{
  return make_buffer<index_t, vertex_t, index_t, vertex_t>(n + 1, fw_m, n + 1, bw_m);
}
