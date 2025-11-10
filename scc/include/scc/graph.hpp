#pragma once

#include <memory/buffer.hpp>
#include <scc/base.hpp>
#include <scc/part.hpp>

struct LocalGraph
{
  Buffer    buffer;
  vertex_t  n       = 0;
  index_t   m       = 0;
  index_t*  fw_head = nullptr;
  vertex_t* fw_csr  = nullptr;
  index_t*  bw_head = nullptr;
  vertex_t* bw_csr  = nullptr;
};

template<WorldPartConcept Part>
struct LocalGraphPart
{
  Buffer    buffer;
  Part      part;
  index_t   m          = 0;
  index_t   local_fw_m = 0;
  index_t   local_bw_m = 0;
  index_t*  fw_head    = nullptr;
  index_t*  bw_head    = nullptr;
  vertex_t* fw_csr     = nullptr;
  vertex_t* bw_csr     = nullptr;
};

struct Graph
{
  vertex_t  n       = 0;
  index_t   m       = 0;
  index_t*  fw_head = nullptr;
  vertex_t* fw_csr  = nullptr;
  index_t*  bw_head = nullptr;
  vertex_t* bw_csr  = nullptr;

  Graph() = default;
  ~Graph() = default;

  explicit Graph(LocalGraph const& rhs)
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
struct GraphPart
{
  Part      part;
  index_t   m          = 0;
  index_t   local_fw_m = 0;
  index_t   local_bw_m = 0;
  index_t*  fw_head    = nullptr;
  index_t*  bw_head    = nullptr;
  vertex_t* fw_csr     = nullptr;
  vertex_t* bw_csr     = nullptr;

  GraphPart() = default;
  ~GraphPart() = default;

  explicit GraphPart(LocalGraphPart<Part> const& rhs)
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
