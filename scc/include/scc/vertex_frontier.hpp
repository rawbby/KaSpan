#pragma once

#include <scc/base.hpp>
#include <scc/part.hpp>

#include <mpi.h>

#include <vector>

struct vertex_frontier
{
  std::vector<vertex_t> send_buffer;
  std::vector<vertex_t> recv_buffer;

  Buffer     buffer;
  MPI_Count* send_counts = nullptr;
  MPI_Aint*  send_displs = nullptr;
  MPI_Count* recv_counts = nullptr;
  MPI_Aint*  recv_displs = nullptr;

  static auto create() -> vertex_frontier
  {
    auto buffer = make_buffer<MPI_Count, MPI_Count, MPI_Aint, MPI_Aint>(
      mpi_world_size, mpi_world_size, mpi_world_size, mpi_world_size);
    auto* memory = buffer.data();

    vertex_frontier frontier;
    frontier.buffer      = std::move(buffer);
    frontier.send_counts = ::borrow_clean<MPI_Count>(memory, mpi_world_size);
    frontier.send_displs = ::borrow<MPI_Aint>(memory, mpi_world_size);
    frontier.recv_counts = ::borrow<MPI_Count>(memory, mpi_world_size);
    frontier.recv_displs = ::borrow<MPI_Aint>(memory, mpi_world_size);
    return frontier;
  }

  void local_push(vertex_t u)
  {
    recv_buffer.emplace_back(u);
  }

  void push(i32 rank, vertex_t u)
  {
    send_buffer.emplace_back(u);
    ++send_counts[rank];
  }

  [[nodiscard]] auto has_next() const -> bool
  {
    return not recv_buffer.empty();
  }

  auto next() -> vertex_t
  {
    auto const edge = recv_buffer.back();
    recv_buffer.pop_back();
    return edge;
  }

  template<WorldPartConcept Part>
  auto comm(Part const& part) -> bool
  {
    auto const send_count = mpi_basic_displs(send_counts, send_displs);
    DEBUG_ASSERT_EQ(send_count, send_buffer.size());

    auto const total_messages = mpi_basic_allreduce_single(send_count, MPI_SUM);
    if (total_messages == 0) {
      return false;
    }

    mpi_basic_alltoallv_counts(send_counts, recv_counts);
    auto const recv_count = mpi_basic_displs(recv_counts, recv_displs);

    auto const recv_offset = recv_buffer.size();
    recv_buffer.resize(recv_offset + recv_count);
    auto* recv_memory = recv_buffer.data() + recv_offset;

    mpi_basic_inplace_partition_by_rank(send_buffer.data(), send_counts, send_displs, [&part](vertex_t u) {
      return part.world_rank_of(u);
    });

    mpi_basic_alltoallv(
      send_buffer.data(),
      send_counts,
      send_displs,
      recv_memory,
      recv_counts,
      recv_displs);

    send_buffer.clear();
    std::memset(send_counts, 0, mpi_world_size * sizeof(MPI_Count));

    return true;
  }
};
