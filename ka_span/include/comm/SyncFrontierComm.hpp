#pragma once

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
    RESULT_TRY(result.recv_counts_, I32Buffer::zeroes(comm.size()));
    RESULT_TRY(result.recv_displs_, I32Buffer::create(comm.size()));
    RESULT_TRY(result.send_counts_, I32Buffer::zeroes(comm.size()));
    RESULT_TRY(result.send_displs_, I32Buffer::create(comm.size()));

    // these two are ALWAYS zero and will never change
    result.recv_displs_[0] = 0;
    result.send_displs_[0] = 0;

    return result;
  }

  void clear()
  {
    // now prepare send buffer and send counts for next iteration
    send_buf_.clear();
    std::memset(send_counts_.data(), 0, send_counts_.bytes());
  }

  void push(Partition const& part, u64 v)
  {
    send_buf_.emplace_back(v);
    ++send_counts_[part.world_rank_of(v)];
  }

  void push_relaxed(kamping::Communicator<> const& comm, Partition const& part, u64 payload)
  {
    auto const rank = part.world_rank_of(payload);
    if (rank == comm.rank()) {
      recv_buf_.emplace_back(payload);
    } else {
      send_buf_.emplace_back(payload);
      ++send_counts_[rank];
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
    // communicate the message count across all ranks
    u64 world_message_count = send_buf_.size();
    MPI_Allreduce(MPI_IN_PLACE, &world_message_count, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);

    // check if there is communication needed
    if (world_message_count == 0)
      return false; // finished (no further communication)

    // collect recv counts from other ranks
    MPI_Alltoall(send_counts_.data(), 1, MPI_INT32_T, recv_counts_.data(), 1, MPI_INT32_T, MPI_COMM_WORLD);

    // create prefix sums for send and recv (and compute total recv count)
    auto const world      = comm.size();
    u64        recv_count = recv_counts_[0];
    for (size_t rank = 1; rank < world; ++rank) {
      send_displs_[rank] = send_displs_[rank - 1] + send_counts_[rank - 1];
      recv_displs_[rank] = recv_displs_[rank - 1] + recv_counts_[rank - 1];
      recv_count += recv_counts_[rank];
    }

    // reallocate recv buffer
    auto const recv_buf_offest = recv_buf_.size();
    recv_buf_.resize(recv_buf_offest + recv_count);

    // inplace sort the send buffer
    size_t index = 0;
    for (size_t bucket = 0; bucket < comm.size(); ++bucket) {
      // skip already placed elements
      size_t bucket_index = send_displs_[bucket] - index;
      index               = send_displs_[bucket];

      while (bucket_index < send_counts_[bucket]) {

        auto const u      = send_buf_[index];
        auto const rank_u = part.world_rank_of(u);

        if (bucket == rank_u) {
          ++send_displs_[bucket];
          ++bucket_index;
          ++index;
        }

        else {
          std::swap(send_buf_[index], send_buf_[send_displs_[rank_u]++]);
        }
      }
    }

    // restore prefix sums after they where used by the sorting
    for (int r = 0; r < comm.size(); ++r)
      send_displs_[r] -= send_counts_[r];

    // now we are ready to send
    auto const* send_counts = send_counts_.range().data();
    auto const* send_displs = send_displs_.range().data();
    auto const* recv_counts = recv_counts_.range().data();
    auto const* recv_displs = recv_displs_.range().data();
    MPI_Alltoallv(send_buf_.data(), send_counts, send_displs, MPI_UINT64_T, recv_buf_.data() + recv_buf_offest, recv_counts, recv_displs, MPI_UINT64_T, MPI_COMM_WORLD);

    // now prepare send buffer and send counts for next iteration
    if (AutoClear)
      clear();

    return true; // there where exchanged messages
  }

  // clang-format off
  [[nodiscard]] auto recv_buf() const -> std::vector<u64> const & { return recv_buf_; }
  [[nodiscard]] auto recv_counts() const -> std::span<i32 const> { return recv_counts_.range(); }
  [[nodiscard]] auto recv_displs() const -> std::span<i32 const> { return recv_displs_.range(); }
  [[nodiscard]] auto send_buf() const -> std::vector<u64> const & { return send_buf_; }
  [[nodiscard]] auto send_counts() const -> std::span<i32 const> { return send_counts_.range(); }
  [[nodiscard]] auto send_displs() const -> std::span<i32 const> { return send_displs_.range(); }
  // clang-format on

private:
  std::vector<u64> recv_buf_;    // local frontier
  I32Buffer        recv_counts_; // number of messages to recv per rank
  I32Buffer        recv_displs_; // prefix sums of recv_counts

  std::vector<u64> send_buf_;    // messages to send
  I32Buffer        send_counts_; // number of messages to send per rank
  I32Buffer        send_displs_; // prefix sums of send_counts
};
