#pragma once

#include <kaspan/scc/base.hpp>
#include <kaspan/graph/part.hpp>

#include <mpi.h>

#include <vector>

namespace kaspan {

struct edge_frontier
{
  std::vector<edge_t> send_buffer;
  std::vector<edge_t> recv_buffer;

  buffer     storage;
  MPI_Count* send_counts = nullptr;
  MPI_Aint*  send_displs = nullptr;
  MPI_Count* recv_counts = nullptr;
  MPI_Aint*  recv_displs = nullptr;

  static auto create(
    u64 capacity = 0) -> edge_frontier
  {
    auto  b      = make_buffer<MPI_Count, MPI_Count, MPI_Aint, MPI_Aint>(mpi_basic::world_size, mpi_basic::world_size, mpi_basic::world_size, mpi_basic::world_size);
    auto* memory = b.data();

    edge_frontier frontier;
    if (capacity != 0U) {
      frontier.send_buffer.reserve(capacity);
      frontier.recv_buffer.reserve(capacity);
    }
    frontier.storage     = std::move(b);
    frontier.send_counts = borrow_array_clean<MPI_Count>(&memory, mpi_basic::world_size);
    frontier.send_displs = borrow_array<MPI_Aint>(&memory, mpi_basic::world_size);
    frontier.recv_counts = borrow_array<MPI_Count>(&memory, mpi_basic::world_size);
    frontier.recv_displs = borrow_array<MPI_Aint>(&memory, mpi_basic::world_size);
    return frontier;
  }

  void local_push(
    edge_t const& e)
  {
    recv_buffer.emplace_back(e);
  }

  void push(
    i32           rank,
    edge_t const& e)
  {
    send_buffer.emplace_back(e);
    ++send_counts[rank];
  }

  void relaxed_push(
    i32           rank,
    edge_t const& e)
  {
    if (rank == mpi_basic::world_rank) {
      local_push(e);
    } else {
      push(rank, e);
    }
  }

  [[nodiscard]] auto has_next() const -> bool
  {
    return not recv_buffer.empty();
  }

  auto next() -> edge_t
  {
    auto const e = recv_buffer.back();
    recv_buffer.pop_back();
    return e;
  }

  // clang-format off
  auto cbegin() const { return recv_buffer.cbegin(); }
  auto cend()   const { return recv_buffer.cend();   }
  auto begin()  const { return cbegin();             }
  auto end()    const { return cend();               }
  auto begin()        { return recv_buffer.begin();  }
  auto end()          { return recv_buffer.end();    }
  // clang-format on

  template<world_part_concept part_t>
  auto comm(
    part_t const& part) -> bool
  {
    DEBUG_ASSERT_NE(mpi_edge_t, mpi_basic::datatype_null);

    auto const send_count = mpi_basic::displs(send_counts, send_displs);
    DEBUG_ASSERT_EQ(send_count, send_buffer.size());

    auto const total_messages = mpi_basic::allreduce_single(send_count, mpi_basic::sum);
    if (total_messages == 0) {
      return false;
    }

    mpi_basic::alltoallv_counts(send_counts, recv_counts);
    auto const recv_count = mpi_basic::displs(recv_counts, recv_displs);

    auto const recv_offset = recv_buffer.size();
    recv_buffer.resize(recv_offset + recv_count);
    auto* recv_memory = recv_buffer.data() + recv_offset;

    mpi_basic::inplace_partition_by_rank(send_buffer.data(), send_counts, send_displs, [&part](edge_t const& e) {
      return part.world_rank_of(e.u);
    });

    mpi_basic::alltoallv(send_buffer.data(), send_counts, send_displs, recv_memory, recv_counts, recv_displs, mpi_edge_t);

    send_buffer.clear();
    std::memset(send_counts, 0, mpi_basic::world_size * sizeof(MPI_Count));

    return true;
  }
};

} // namespace kaspan
