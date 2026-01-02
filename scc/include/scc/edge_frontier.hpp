#pragma once

#include <scc/base.hpp>
#include <scc/part.hpp>

#include <mpi.h>

#include <vector>

struct edge_frontier
{
  std::vector<Edge> send_buffer;
  std::vector<Edge> recv_buffer;

  Buffer     buffer;
  MPI_Count* send_counts = nullptr;
  MPI_Aint*  send_displs = nullptr;
  MPI_Count* recv_counts = nullptr;
  MPI_Aint*  recv_displs = nullptr;

  static auto create(u64 capacity = 0) -> edge_frontier
  {
    auto buffer = make_buffer<MPI_Count, MPI_Count, MPI_Aint, MPI_Aint>(
      mpi_world_size, mpi_world_size, mpi_world_size, mpi_world_size);
    auto* memory = buffer.data();

    edge_frontier frontier;
    if (capacity) {
      frontier.send_buffer.reserve(capacity);
      frontier.recv_buffer.reserve(capacity);
    }
    frontier.buffer      = std::move(buffer);
    frontier.send_counts = ::borrow_array_clean<MPI_Count>(&memory, mpi_world_size);
    frontier.send_displs = ::borrow_array<MPI_Aint>(&memory, mpi_world_size);
    frontier.recv_counts = ::borrow_array<MPI_Count>(&memory, mpi_world_size);
    frontier.recv_displs = ::borrow_array<MPI_Aint>(&memory, mpi_world_size);
    return frontier;
  }

  void local_push(Edge const& edge)
  {
    recv_buffer.emplace_back(edge);
  }

  void push(i32 rank, Edge const& edge)
  {
    send_buffer.emplace_back(edge);
    ++send_counts[rank];
  }

  void relaxed_push(i32 rank, Edge const& edge)
  {
    if (rank == mpi_world_rank) {
      local_push(edge);
    } else {
      push(rank, edge);
    }
  }

  auto has_next() -> bool
  {
    return not recv_buffer.empty();
  }

  auto next() -> Edge
  {
    auto const edge = recv_buffer.back();
    recv_buffer.pop_back();
    return edge;
  }

  template<WorldPartConcept Part>
  auto comm(Part const& part) -> bool
  {
    DEBUG_ASSERT_NE(mpi_edge_t, MPI_DATATYPE_NULL);

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

    mpi_basic_inplace_partition_by_rank(send_buffer.data(), send_counts, send_displs, [&part](Edge const& edge) {
      return part.world_rank_of(edge.u);
    });

    mpi_basic_alltoallv(
      send_buffer.data(),
      send_counts,
      send_displs,
      recv_memory,
      recv_counts,
      recv_displs,
      mpi_edge_t);

    send_buffer.clear();
    std::memset(send_counts, 0, mpi_world_size * sizeof(MPI_Count));

    return true;
  }

  template<WorldPartConcept Part>
  [[nodiscard]] auto icomm(Part const& part) -> MPI_Request
  {
    DEBUG_ASSERT_NE(mpi_edge_t, MPI_DATATYPE_NULL);

    auto const send_count = mpi_basic_displs(send_counts, send_displs);
    DEBUG_ASSERT_EQ(send_count, send_buffer.size());

    auto const total_messages = mpi_basic_allreduce_single(send_count, MPI_SUM);
    if (total_messages == 0) {
      return MPI_REQUEST_NULL;
    }

    mpi_basic_alltoallv_counts(send_counts, recv_counts);
    auto const recv_count = mpi_basic_displs(recv_counts, recv_displs);

    auto const recv_offset = recv_buffer.size();
    recv_buffer.resize(recv_offset + recv_count);
    auto* recv_memory = recv_buffer.data() + recv_offset;

    mpi_basic_inplace_partition_by_rank(send_buffer.data(), send_counts, send_displs, [&part](Edge const& edge) {
      return part.world_rank_of(edge.u);
    });

    return mpi_basic_ialltoallv(
      send_buffer.data(),
      send_counts,
      send_displs,
      recv_memory,
      recv_counts,
      recv_displs,
      mpi_edge_t);
  }

  bool isync(MPI_Request request)
  {
    if (request == MPI_REQUEST_NULL) {
      return false;
    }

    mpi_basic_wait(request);
    send_buffer.clear();
    std::memset(send_counts, 0, mpi_world_size * sizeof(MPI_Count));
    return true;
  }
};
