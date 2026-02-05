#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/memory/accessor/vector.hpp>
#include <kaspan/util/mpi_basic.hpp>

#include <mpi.h>

#include <cmath>
#include <cstring>
#include <type_traits>

namespace kaspan {

template<typename T>
struct frontier_view
{
  vector<vertex_t>* send_buffer = nullptr;
  vector<vertex_t>* recv_buffer = nullptr;
  MPI_Count*        send_counts = nullptr;
  MPI_Aint*         send_displs = nullptr;
  MPI_Count*        recv_counts = nullptr;
  MPI_Aint*         recv_displs = nullptr;

  constexpr frontier_view() noexcept  = default;
  constexpr ~frontier_view() noexcept = default;

  constexpr frontier_view(
    vector<vertex_t>* send_buffer,
    vector<vertex_t>* recv_buffer,
    MPI_Count*        send_counts,
    MPI_Aint*         send_displs,
    MPI_Count*        recv_counts,
    MPI_Aint*         recv_displs) noexcept
    : send_buffer(send_buffer)
    , recv_buffer(recv_buffer)
    , send_counts(send_counts)
    , send_displs(send_displs)
    , recv_counts(recv_counts)
    , recv_displs(recv_displs)
  {
  }

  constexpr frontier_view(frontier_view const&) noexcept = default;
  constexpr frontier_view(frontier_view&&) noexcept      = default;

  constexpr auto operator=(frontier_view const&) noexcept -> frontier_view& = default;
  constexpr auto operator=(frontier_view&&) noexcept -> frontier_view&      = default;

  template<part_view_c Part>
  constexpr auto world_rank_of(
    Part part,
    T    value) noexcept -> i32
  {
    if constexpr (std::same_as<T, vertex_t>) {
      return part.world_rank_of(value);
    } else if constexpr (std::same_as<T, edge_t>) {
      return part.world_rank_of(value.u);
    } else {
      static_assert(std::same_as<T, labeled_edge_t>);
      return part.world_rank_of(value.u);
    }
  }

  void local_push(
    T value) const
  {
    if constexpr (std::same_as<T, vertex_t>) {
      recv_buffer->push_back(value);
    } else if constexpr (std::same_as<T, edge_t>) {
      recv_buffer->push_back(value.u);
      recv_buffer->push_back(value.v);
    } else {
      static_assert(std::same_as<T, labeled_edge_t>);
      recv_buffer->push_back(value.u);
      recv_buffer->push_back(value.v);
      recv_buffer->push_back(value.l);
    }
  }

  void local_unsafe_push(
    T value) const
  {
    if constexpr (std::same_as<T, vertex_t>) {
      recv_buffer->unsafe_push_back(value);
    } else if constexpr (std::same_as<T, edge_t>) {
      recv_buffer->unsafe_push_back(value.u);
      recv_buffer->unsafe_push_back(value.v);
    } else {
      static_assert(std::same_as<T, labeled_edge_t>);
      recv_buffer->unsafe_push_back(value.u);
      recv_buffer->unsafe_push_back(value.v);
      recv_buffer->unsafe_push_back(value.l);
    }
  }

  template<part_view_c Part>
  void push(
    Part                    part,
    integral_c auto rank,
    T                       value)
  {
    if constexpr (std::same_as<T, vertex_t>) {
      send_buffer->push_back(value);
    } else if constexpr (std::same_as<T, edge_t>) {
      send_buffer->push_back(value.u);
      send_buffer->push_back(value.v);
    } else {
      static_assert(std::same_as<T, labeled_edge_t>);
      send_buffer->push_back(value.u);
      send_buffer->push_back(value.v);
      send_buffer->push_back(value.l);
    }
    ++send_counts[integral_cast<i32>(rank)];
  }

  template<part_view_c Part>
  void push(
    Part                    part,
    integral_c auto rank,
    T                       value,
    auto&&                  consumer)
  {
    if constexpr (std::same_as<T, vertex_t>) {
      send_buffer->push_back(value);
    } else if constexpr (std::same_as<T, edge_t>) {
      send_buffer->push_back(value.u);
      send_buffer->push_back(value.v);
    } else {
      static_assert(std::same_as<T, labeled_edge_t>);
      send_buffer->push_back(value.u);
      send_buffer->push_back(value.v);
      send_buffer->push_back(value.l);
    }
    ++send_counts[integral_cast<i32>(rank)];
  }

  template<part_view_c Part>
  void relaxed_push(
    Part                    part,
    integral_c auto rank,
    T                       value)
  {
    auto const r = integral_cast<i32>(rank);
    if (r == mpi_basic::world_rank) local_push(value);
    else push(part, r, value);
  }

  template<part_view_c Part>
  void relaxed_push(
    Part                    part,
    integral_c auto rank,
    T                       value,
    auto&&                  consumer)
  {
    auto const r = integral_cast<i32>(rank);
    if (r == mpi_basic::world_rank) local_push(value);
    else push(part, r, value, consumer);
  }

  template<part_view_c Part>
  void push(
    Part   part,
    T      value,
    auto&& consumer)
  {
    push(part, world_rank_of(part, value), value, consumer);
  }

  template<part_view_c Part>
  void push(
    Part part,
    T    value)
  {
    push(part, world_rank_of(part, value), value);
  }

  template<part_view_c Part>
  void relaxed_push(
    Part   part,
    T      value,
    auto&& consumer)
  {
    relaxed_push(part, world_rank_of(part, value), value, consumer);
  }

  template<part_view_c Part>
  void relaxed_push(
    Part part,
    T    value)
  {
    relaxed_push(part, world_rank_of(part, value), value);
  }

  [[nodiscard]] auto size() const -> u64
  {
    constexpr auto element_count = sizeof(T) / sizeof(vertex_t);
    return recv_buffer->size() / element_count;
  }

  [[nodiscard]] auto empty() const -> bool
  {
    return recv_buffer->empty();
  }

  [[nodiscard]] auto has_next() const -> bool
  {
    return !recv_buffer->empty();
  }

  auto next() -> T
  {
    if constexpr (std::same_as<T, vertex_t>) {
      auto const u = recv_buffer->back();
      recv_buffer->pop_back();
      return u;
    } else if constexpr (std::same_as<T, edge_t>) {
      auto const v = recv_buffer->back();
      recv_buffer->pop_back();
      auto const u = recv_buffer->back();
      recv_buffer->pop_back();
      return edge_t{ u, v };
    } else {
      static_assert(std::same_as<T, labeled_edge_t>);
      auto const l = recv_buffer->back();
      recv_buffer->pop_back();
      auto const v = recv_buffer->back();
      recv_buffer->pop_back();
      auto const u = recv_buffer->back();
      recv_buffer->pop_back();
      return labeled_edge_t{ u, v, l };
    }
  }

  template<part_view_c Part>
  auto comm(
    Part part) -> bool
  {
    constexpr auto element_count = sizeof(T) / sizeof(vertex_t);

    auto const mpi_type = std::is_same_v<T, vertex_t> ? mpi_vertex_t : (std::is_same_v<T, edge_t> ? mpi_edge_t : mpi_labeled_edge_t);

    auto const send_count = mpi_basic::displs(send_counts, send_displs);
    DEBUG_ASSERT_EQ(send_count * element_count, send_buffer->size());

    if (0 == mpi_basic::allreduce_single(send_count, mpi_basic::sum)) {
      return false;
    }

    mpi_basic::alltoallv_counts(send_counts, recv_counts);
    auto const recv_count = mpi_basic::displs(recv_counts, recv_displs);

    auto const recv_offset = recv_buffer->size();
    recv_buffer->resize(recv_offset + recv_count * element_count);
    auto* recv_memory = static_cast<T*>(static_cast<void*>(recv_buffer->data() + recv_offset));
    auto* send_memory = static_cast<T*>(static_cast<void*>(send_buffer->data()));

    auto const value_to_rank = [&](T value) -> i32 { return world_rank_of(part, value); };
    mpi_basic::inplace_partition_by_rank(send_memory, send_counts, send_displs, value_to_rank);

    mpi_basic::alltoallv(send_buffer->data(), send_counts, send_displs, recv_memory, recv_counts, recv_displs, mpi_type);

    send_buffer->clear();
    std::memset(send_counts, 0x00, mpi_basic::world_size * sizeof(MPI_Count));

    return true;
  }

  template<part_view_c Part>
  auto comm(
    Part   part,
    auto&& consumer) -> bool
  {
    if (!comm(part)) return false;

    while (has_next()) {
      consumer(next());
    }
    return true;
  }
};

struct frontier
{
  vector<vertex_t> send_buffer{};
  vector<vertex_t> recv_buffer{};
  MPI_Count*       send_counts = nullptr;
  MPI_Aint*        send_displs = nullptr;
  MPI_Count*       recv_counts = nullptr;
  MPI_Aint*        recv_displs = nullptr;

  frontier() noexcept
    : send_counts(line_alloc<MPI_Count>(mpi_basic::world_size))
    , send_displs(line_alloc<MPI_Aint>(mpi_basic::world_size))
    , recv_counts(line_alloc<MPI_Count>(mpi_basic::world_size))
    , recv_displs(line_alloc<MPI_Aint>(mpi_basic::world_size))
  {
    std::memset(send_counts, 0x00, mpi_basic::world_size * sizeof(MPI_Count));
  }

  ~frontier() noexcept
  {
    line_free(send_counts);
    line_free(send_displs);
    line_free(recv_counts);
    line_free(recv_displs);
    send_counts = nullptr;
    send_displs = nullptr;
    recv_counts = nullptr;
    recv_displs = nullptr;
  }

  explicit frontier(
    integral_c auto initial_capacity) noexcept
    : send_buffer(initial_capacity)
    , recv_buffer(initial_capacity)
    , send_counts(line_alloc<MPI_Count>(mpi_basic::world_size))
    , send_displs(line_alloc<MPI_Aint>(mpi_basic::world_size))
    , recv_counts(line_alloc<MPI_Count>(mpi_basic::world_size))
    , recv_displs(line_alloc<MPI_Aint>(mpi_basic::world_size))
  {
    std::memset(send_counts, 0x00, mpi_basic::world_size * sizeof(MPI_Count));
  }

  frontier(frontier const&) noexcept = delete;
  frontier(
    frontier&& rhs) noexcept
    : send_buffer(std::move(rhs.send_buffer))
    , recv_buffer(std::move(rhs.recv_buffer))
    , send_counts(rhs.send_counts)
    , send_displs(rhs.send_displs)
    , recv_counts(rhs.recv_counts)
    , recv_displs(rhs.recv_displs)
  {
    rhs.send_counts = nullptr;
    rhs.send_displs = nullptr;
    rhs.recv_counts = nullptr;
    rhs.recv_displs = nullptr;
  }

  auto operator=(frontier const&) noexcept -> frontier& = delete;
  auto operator=(
    frontier&& rhs) noexcept -> frontier&
  {
    if (this != &rhs) {
      send_buffer = std::move(rhs.send_buffer);
      recv_buffer = std::move(rhs.recv_buffer);
      line_free(send_counts);
      line_free(send_displs);
      line_free(recv_counts);
      line_free(recv_displs);
      send_counts     = rhs.send_counts;
      send_displs     = rhs.send_displs;
      recv_counts     = rhs.recv_counts;
      recv_displs     = rhs.recv_displs;
      rhs.send_counts = nullptr;
      rhs.send_displs = nullptr;
      rhs.recv_counts = nullptr;
      rhs.recv_displs = nullptr;
    }
    return *this;
  }

  template<typename T>
    requires(std::same_as<T,
                          vertex_t> ||
             std::same_as<T,
                          edge_t> ||
             std::same_as<T,
                          labeled_edge_t>)
  auto view() -> frontier_view<T>
  {
    return { &send_buffer, &recv_buffer, send_counts, send_displs, recv_counts, recv_displs };
  }
};

} // namespace kaspan
