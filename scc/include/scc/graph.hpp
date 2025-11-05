#pragma once

#include <scc/base.hpp>
#include <scc/part.hpp>
#include <memory/buffer.hpp>

struct LocalGraph
{
  Buffer    buffer;
  vertex_t  n;
  index_t   m;
  index_t*  fw_head;
  vertex_t* fw_csr;
  index_t*  bw_head;
  vertex_t* bw_csr;
};

template<WorldPartConcept Part>
struct LocalGraphPart
{
  Buffer    buffer;
  Part      part;
  index_t   m;
  index_t   local_fw_m;
  index_t   local_bw_m;
  index_t*  fw_head;
  index_t*  bw_head;
  vertex_t* fw_csr;
  vertex_t* bw_csr;
};

struct Graph
{
  vertex_t  n;
  index_t   m;
  index_t*  fw_head;
  vertex_t* fw_csr;
  index_t*  bw_head;
  vertex_t* bw_csr;

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
  index_t   m;
  index_t   local_fw_m;
  index_t   local_bw_m;
  index_t*  fw_head;
  index_t*  bw_head;
  vertex_t* fw_csr;
  vertex_t* bw_csr;

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
