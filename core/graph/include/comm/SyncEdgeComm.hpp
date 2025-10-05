#pragma once

#include <buffer/Buffer.hpp>
#include <graph/Edge.hpp>
#include <graph/Part.hpp>
#include <util/Result.hpp>

#include <kamping/collectives/allreduce.hpp>
#include <kamping/collectives/alltoall.hpp>
#include <kamping/collectives/barrier.hpp>
#include <kamping/communicator.hpp>

template<class Impl>
class SyncAlltoallvBase
{
public:
  using Payload = Impl::Payload;

  SyncAlltoallvBase()  = default;
  ~SyncAlltoallvBase() = default;

  explicit SyncAlltoallvBase(Impl impl)
    : impl_(std::move(impl))
  {
  }

  SyncAlltoallvBase(SyncAlltoallvBase const&) = delete;
  SyncAlltoallvBase(SyncAlltoallvBase&& rhs) noexcept
    : impl_(std::move(rhs.impl_))
    , recv_buf_(std::move(rhs.recv_buf_))
    , recv_counts_(std::move(rhs.recv_counts_))
    , recv_displs_(std::move(rhs.recv_displs_))
    , send_buf_(std::move(rhs.send_buf_))
    , send_counts_(std::move(rhs.send_counts_))
    , send_displs_(std::move(rhs.send_displs_))
  {
  }

  auto operator=(SyncAlltoallvBase const&) -> SyncAlltoallvBase& = delete;
  auto operator=(SyncAlltoallvBase&& rhs) noexcept -> SyncAlltoallvBase&
  {
    if (this != &rhs) [[likely]] {
      impl_        = std::move(rhs.impl_);
      recv_buf_    = std::move(rhs.recv_buf_);
      recv_counts_ = std::move(rhs.recv_counts_);
      recv_displs_ = std::move(rhs.recv_displs_);
      send_buf_    = std::move(rhs.send_buf_);
      send_counts_ = std::move(rhs.send_counts_);
      send_displs_ = std::move(rhs.send_displs_);
    }
    return *this;
  }

  static auto create(kamping::Communicator<> const& comm, Impl impl) -> Result<SyncAlltoallvBase>
  {
    SyncAlltoallvBase result{ std::move(impl) };
    RESULT_TRY(result.recv_counts_, I32Buffer::zeroes(comm.size()));
    RESULT_TRY(result.recv_displs_, I32Buffer::create(comm.size()));
    RESULT_TRY(result.send_counts_, I32Buffer::zeroes(comm.size()));
    RESULT_TRY(result.send_displs_, I32Buffer::create(comm.size()));
    result.recv_displs_[0] = 0;
    result.send_displs_[0] = 0;
    return result;
  }

  void clear()
  {
    send_buf_.clear();
    std::memset(send_counts_.data(), 0, send_counts_.bytes());
  }

  void push(Payload payload)
  {
    send_buf_.emplace_back(payload);
    ++send_counts_[impl_.world_rank_of(payload)];
  }

  void push_relaxed(Payload payload)
  {
    auto const payload_rank = impl_.world_rank_of(payload);

    if (payload_rank == impl_.world_rank()) {
      recv_buf_.emplace_back(payload);
    }

    else {
      send_buf_.emplace_back(payload);
      ++send_counts_[payload_rank];
    }
  }

  [[nodiscard]] auto has_next() const -> bool
  {
    return not recv_buf_.empty();
  }

  [[nodiscard]] auto next() -> Payload
  {
    auto const result = recv_buf_.back();
    recv_buf_.pop_back();
    return result;
  }

  template<bool AutoClear = true>
  auto communicate(kamping::Communicator<> const& comm) -> bool
  {
    namespace kmp = kamping;

    u64 rmc = send_buf_.size(); // rank message count
    if (comm.allreduce_single(kmp::send_buf(rmc), kmp::op(kmp::ops::plus{})) == 0) {
      return false; // world message count == 0
    }

    comm.alltoall(
      kmp::send_buf(send_counts_.range()),
      kmp::send_count(1),
      kmp::recv_buf(recv_counts_.range()));

    // create prefix sums for send and recv (and compute total recv count)
    u64 recv_count = recv_counts_[0];
    for (size_t rank = 1; rank < comm.size(); ++rank) {
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
      size_t bucket_index = send_displs_[bucket] - index;
      index               = send_displs_[bucket];
      while (bucket_index < send_counts_[bucket]) {
        auto const rank_u = impl_.world_rank_of(send_buf_[index]);
        if (bucket == rank_u) {
          ++send_displs_[bucket];
          ++bucket_index;
          ++index;
        } else {
          auto const lhs = index;
          auto const rhs = send_displs_[rank_u]++;
          impl_.swap(lhs, rhs);
          std::swap(send_buf_[lhs], send_buf_[rhs]);
        }
      }
    }

    // restore prefix sums after they where used by the sorting
    for (int r = 0; r < comm.size(); ++r)
      send_displs_[r] -= send_counts_[r];

    // now we are ready to send
    comm.alltoallv(
      kmp::send_buf(send_buf_),
      kmp::send_counts(send_counts_.range()),
      kmp::send_displs(send_displs_.range()),
      kmp::recv_buf(std::span{ recv_buf_ }.last(recv_count)),
      kmp::recv_counts(recv_counts_.range()),
      kmp::recv_displs(recv_displs_.range()));

    // now prepare send buffer and send counts for next iteration
    if (AutoClear)
      clear();

    return true; // there where exchanged messages
  }

  // clang-format off
  [[nodiscard]] auto recv_buf() -> std::span<Payload> { return { recv_buf_ }; }
  [[nodiscard]] auto recv_buf() const -> std::span<Payload const> { return { recv_buf_ }; }
  [[nodiscard]] auto recv_counts() const -> std::span<i32 const> { return recv_counts_.range(); }
  [[nodiscard]] auto recv_displs() const -> std::span<i32 const> { return recv_displs_.range(); }
  [[nodiscard]] auto send_buf() -> std::span<Payload> { return { send_buf_ }; }
  [[nodiscard]] auto send_buf() const -> std::span<Payload const> { return { send_buf_ }; }
  [[nodiscard]] auto send_counts() const -> std::span<i32 const> { return send_counts_.range(); }
  [[nodiscard]] auto send_displs() const -> std::span<i32 const> { return send_displs_.range(); }
  // clang-format on

private:
  Impl impl_;

  std::vector<Payload> recv_buf_;    // local frontier
  I32Buffer            recv_counts_; // number of messages to recv per rank
  I32Buffer            recv_displs_; // prefix sums of recv_counts

  std::vector<Payload> send_buf_;    // messages to send
  I32Buffer            send_counts_; // number of messages to send per rank
  I32Buffer            send_displs_; // prefix sums of send_counts
};

template<WorldPartConcept Part>
class SyncEdgeComm
{
public:
  using Payload = Edge;

  SyncEdgeComm()  = default;
  ~SyncEdgeComm() = default;

  explicit SyncEdgeComm(Part part)
    : part_(part)
  {
  }

  SyncEdgeComm(SyncEdgeComm const&) = default;
  SyncEdgeComm(SyncEdgeComm&&)      = default;

  auto operator=(SyncEdgeComm const&) -> SyncEdgeComm& = default;
  auto operator=(SyncEdgeComm&&) -> SyncEdgeComm&      = default;

  auto world_rank() const -> size_t
  {
    return part_.world_rank;
  }

  auto world_rank_of(Edge payload) const -> size_t
  {
    return part_.world_rank_of(payload.u);
  }

  void swap(size_t /* i */, size_t /* j */) const {}

private:
  Part part_;
};

template<WorldPartConcept Part>
class SyncFrontier
{
public:
  using Payload = u64;

  SyncFrontier()  = default;
  ~SyncFrontier() = default;

  explicit SyncFrontier(Part part)
    : part_(part)
  {
  }

  SyncFrontier(SyncFrontier const&) = default;
  SyncFrontier(SyncFrontier&&)      = default;

  auto operator=(SyncFrontier const&) -> SyncFrontier& = default;
  auto operator=(SyncFrontier&&) -> SyncFrontier&      = default;

  auto world_rank() const -> size_t
  {
    return part_.world_rank;
  }

  auto world_rank_of(Payload payload) const -> size_t
  {
    return part_.world_rank_of(payload);
  }

  void swap(size_t /* i */, size_t /* j */) const {}

private:
  Part part_;
};
