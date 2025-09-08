#pragma once

#include <buffer/Accessor.hpp>
#include <buffer/Buffer.hpp>
#include <graph/Partition.hpp>
#include <util/ErrorCode.hpp>

#include <kamping/collectives/alltoall.hpp>
#include <kamping/communicator.hpp>

template<WorldPartitionConcept Partition>
class SyncFrontierComm
{
public:
  SyncFrontierComm()  = default;
  ~SyncFrontierComm() = default;

  SyncFrontierComm(SyncFrontierComm const&) = delete;
  SyncFrontierComm(SyncFrontierComm&& rhs) noexcept
  {
    std::swap(recv_buf_, rhs.recv_buf_);
    recv_counts_ = std::move(rhs.recv_counts_);
    recv_displs_ = std::move(rhs.recv_displs_);
    std::swap(send_buf_, rhs.send_buf_);
    send_counts_ = std::move(rhs.send_counts_);
    send_displs_ = std::move(rhs.send_displs_);
  }

  auto operator=(SyncFrontierComm const&) -> SyncFrontierComm& = delete;
  auto operator=(SyncFrontierComm&& rhs) noexcept -> SyncFrontierComm&
  {
    if (this != &rhs) [[likely]] {
      std::swap(recv_buf_, rhs.recv_buf_);
      recv_counts_ = std::move(rhs.recv_counts_);
      recv_displs_ = std::move(rhs.recv_displs_);
      std::swap(send_buf_, rhs.send_buf_);
      send_counts_ = std::move(rhs.send_counts_);
      send_displs_ = std::move(rhs.send_displs_);
    }
    return *this;
  }

  static auto create(kamping::Communicator<> const& comm) -> Result<SyncFrontierComm>
  {
    SyncFrontierComm result{};
    result.access_ = I32Accessor{ comm.size() };
    RESULT_TRY(result.recv_counts_, Buffer::zeroes(result.access_));
    RESULT_TRY(result.recv_displs_, Buffer::create(result.access_));
    RESULT_TRY(result.send_counts_, Buffer::zeroes(result.access_));
    RESULT_TRY(result.send_displs_, Buffer::create(result.access_));

    // these two are ALWAYS zero and will never change
    auto recvd = result.access_.range(result.recv_displs_);
    auto sendd = result.access_.range(result.send_displs_);
    recvd[0]   = 0;
    sendd[0]   = 0;

    return result;
  }

  void clear()
  {
    auto sendc = access_.range(send_counts_);
    // now prepare send buffer and send counts for next iteration
    send_buf_.clear();
    std::memset(sendc.data(), 0, sendc.size_bytes());
  }

  void push(Partition const& part, u64 v)
  {
    auto sendc = access_.range(send_counts_);
    send_buf_.emplace_back(v);
    ++sendc[part.world_rank_of(v)];
  }

  void push_relaxed(kamping::Communicator<> const& comm, Partition const& part, u64 payload)
  {
    auto const rank = part.world_rank_of(payload);
    if (rank == comm.rank()) {
      recv_buf_.emplace_back(payload);
    } else {
      auto sendc = access_.range(send_counts_);
      send_buf_.emplace_back(payload);
      ++sendc[rank];
    }
  }

  [[nodiscard]] auto has_next() const -> bool
  {
    return not recv_buf_.empty();
  }

  [[nodiscard]] auto next() -> u64
  {
    auto const result = recv_buf_.back();
    recv_buf_.pop_back();
    return result;
  }

  template<bool AutoClear = true>
  auto communicate(kamping::Communicator<> const& comm, Partition const& part) -> bool
  {
    auto recvd = access_.range(recv_displs_);
    auto recvc = access_.range(recv_counts_);

    auto       sendd = access_.range(send_displs_);
    auto const sendc = access_.range(send_counts_);

    // communicate the message count across all ranks
    u64 world_message_count = send_buf_.size();
    MPI_Allreduce(MPI_IN_PLACE, &world_message_count, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);

    // check if there is communication needed
    if (world_message_count == 0)
      return false; // finished (no further communication)

    // collect recv counts from other ranks
    MPI_Alltoall(sendc.data(), 1, MPI_INT32_T, recvc.data(), 1, MPI_INT32_T, MPI_COMM_WORLD);

    // create prefix sums for send and recv (and compute total recv count)
    auto const world      = comm.size();
    u64        recv_count = recvc[0];
    for (size_t rank = 1; rank < world; ++rank) {
      sendd[rank] = sendd[rank - 1] + sendc[rank - 1];
      recvd[rank] = recvd[rank - 1] + recvc[rank - 1];
      recv_count += recvc[rank];
    }

    // reallocate recv buffer
    auto const recv_buf_offest = recv_buf_.size();
    recv_buf_.resize(recv_buf_offest + recv_count);

    // inplace sort the send buffer
    size_t index = 0;
    for (size_t bucket = 0; bucket < comm.size(); ++bucket) {
      // skip already placed elements
      size_t bucket_index = sendd[bucket] - index;
      index               = sendd[bucket];

      while (bucket_index < sendc[bucket]) {

        auto const u      = send_buf_[index];
        auto const rank_u = part.world_rank_of(u);

        if (bucket == rank_u) {
          ++sendd[bucket];
          ++bucket_index;
          ++index;
        }

        else {
          std::swap(send_buf_[index], send_buf_[sendd[rank_u]++]);
        }
      }
    }

    // restore prefix sums after they where used by the sorting
    for (int r = 0; r < comm.size(); ++r)
      sendd[r] -= sendc[r];

    // now we are ready to send
    MPI_Alltoallv(send_buf_.data(), sendc.data(), sendd.data(), MPI_UINT64_T, recv_buf_.data() + recv_buf_offest, recvc.data(), recvd.data(), MPI_UINT64_T, MPI_COMM_WORLD);

    // now prepare send buffer and send counts for next iteration
    if (AutoClear)
      clear();

    return true; // there where exchanged messages
  }

  // clang-format off
  [[nodiscard]] auto recv_buf()    const -> std::vector<u64> const& { return recv_buf_; }
  [[nodiscard]] auto recv_counts() const -> std::span<i32 const>    { return access_.range(recv_counts_); }
  [[nodiscard]] auto recv_displs() const -> std::span<i32 const>    { return access_.range(recv_displs_); }
  [[nodiscard]] auto send_buf()    const -> std::vector<u64> const& { return send_buf_; }
  [[nodiscard]] auto send_counts() const -> std::span<i32 const>    { return access_.range(send_counts_); }
  [[nodiscard]] auto send_displs() const -> std::span<i32 const>    { return access_.range(send_displs_); }
  // clang-format on

private:
  I32Accessor access_;

  std::vector<u64> recv_buf_;    // local frontier
  Buffer           recv_counts_; // number of messages to recv per rank
  Buffer           recv_displs_; // prefix sums of recv_counts

  std::vector<u64> send_buf_;    // messages to send
  Buffer           send_counts_; // number of messages to send per rank
  Buffer           send_displs_; // prefix sums of send_counts
};
