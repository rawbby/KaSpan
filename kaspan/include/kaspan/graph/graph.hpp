#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>

#include <span>

namespace kaspan {

struct graph
{
  vertex_t        n = 0;
  vertex_t        m = 0;
  array<index_t>  head{};
  array<vertex_t> csr{};
};

struct graph_view
{
  vertex_t  n    = 0;
  vertex_t  m    = 0;
  index_t*  head = nullptr;
  vertex_t* csr  = nullptr;
};

template<world_part_concept Part>
struct graph_part
{
  using part_t = Part;
  part_t          part{};
  vertex_t        local_m = 0;
  array<index_t>  head{};
  array<vertex_t> csr{};
};

template<world_part_concept Part>
struct graph_part_view
{
  using part_t = Part;
  part_t*   part{};
  vertex_t  local_m = 0;
  index_t*  head    = nullptr;
  vertex_t* csr     = nullptr;
};

struct bidi_graph
{
  vertex_t n = 0;
  vertex_t m = 0;
  struct
  {
    array<index_t>  head{};
    array<vertex_t> csr{};
  } fw{};
  struct
  {
    array<index_t>  head{};
    array<vertex_t> csr{};
  } bw{};
};

struct bidi_graph_view
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
};

template<world_part_concept Part>
struct bidi_graph_part
{
  using part_t = Part;
  part_t*  part{};
  vertex_t local_m = 0;
  struct
  {
    array<index_t>  head{};
    array<vertex_t> csr{};
  } fw{};
  struct
  {
    array<index_t>  head{};
    array<vertex_t> csr{};
  } bw{};
};

template<world_part_concept Part>
struct bidi_graph_part_view
{
  using part_t = Part;
  part_t*  part{};
  vertex_t local_m = 0;
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
};

}
