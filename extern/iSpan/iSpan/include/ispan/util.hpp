#pragma once

#include <comm/MpiBasic.hpp>
#include <graph/Base.hpp>
#include <sys/stat.h>

#include <limits>

using depth_t = i16;

constexpr inline auto mpi_depth_t = mpi_basic_type<depth_t>;

constexpr inline depth_t depth_unset = -1;

constexpr inline vertex_t scc_id_undecided = std::numeric_limits<vertex_t>::max();
constexpr inline vertex_t scc_id_singular  = scc_id_undecided - 1;
constexpr inline vertex_t scc_id_largest   = scc_id_undecided - 2;

inline auto
create_scc_id_buffer(vertex_t n) -> Result<VertexBuffer>
{
#ifdef KASPAN_COMPRESS_BUFFER
  return VertexBuffer::create(n + n, sizeof(vertex_t));
#else
  return VertexBuffer::create(n + n);
#endif
}

inline off_t
fsize(char const* filename)
{
  struct stat st{};
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}
