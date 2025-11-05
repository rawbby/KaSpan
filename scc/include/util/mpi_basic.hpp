#pragma once

#include <memory/borrow.hpp>
#include <util/arithmetic.hpp>

#include <mpi.h>

// clang-format off
template<typename T>
const inline MPI_Datatype mpi_basic_type =
  ByteConcept<T> ? MPI_BYTE     :
  I8Concept<T>   ? MPI_INT8_T   :
  I16Concept<T>  ? MPI_INT16_T  :
  I32Concept<T>  ? MPI_INT32_T  :
  I64Concept<T>  ? MPI_INT64_T  :
  U8Concept<T>   ? MPI_UINT8_T  :
  U16Concept<T>  ? MPI_UINT16_T :
  U32Concept<T>  ? MPI_UINT32_T :
  U64Concept<T>  ? MPI_UINT64_T :
  F32Concept<T>  ? MPI_FLOAT    :
  F64Concept<T>  ? MPI_DOUBLE   :
  MPI_DATATYPE_NULL;
// clang-format on

template<typename T>
concept MpiBasicConcept = mpi_basic_type<T> != MPI_DATATYPE_NULL;

constexpr inline auto mpi_byte_t   = MPI_BYTE;
constexpr inline auto mpi_i8_t     = MPI_INT8_T;
constexpr inline auto mpi_i16_t    = MPI_INT16_T;
constexpr inline auto mpi_i32_t    = MPI_INT32_T;
constexpr inline auto mpi_i64_t    = MPI_INT64_T;
constexpr inline auto mpi_u8_t     = MPI_UINT8_T;
constexpr inline auto mpi_u16_t    = MPI_UINT16_T;
constexpr inline auto mpi_u32_t    = MPI_UINT32_T;
constexpr inline auto mpi_u64_t    = MPI_UINT64_T;
constexpr inline auto mpi_float_t  = MPI_FLOAT;
constexpr inline auto mpi_double_t = MPI_DOUBLE;

inline bool mpi_world_root = true;
inline i32  mpi_world_rank = 0;
inline i32  mpi_world_size = 1;

#define MPI_DEFAULT_INIT()                        \
  MPI_Init(nullptr, nullptr);                     \
  SCOPE_GUARD(MPI_Finalize());                    \
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_world_rank); \
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_world_size); \
  mpi_world_root = mpi_world_rank == 0;

/**
 * @brief Allocate temporary receive-counts and byte-displacements arrays for MPI-4 “_c” collectives.
 *
 * @details
 * Returns pointers to counts (MPI_Count[mpi_world_size]) and displacements (MPI_Aint[mpi_world_size])
 * backed by a single Buffer. Keep the returned buffer alive while using the pointers.
 */
inline auto
mpi_basic_counts_and_displs()
{
  struct Result
  {
    Buffer     buffer;
    MPI_Count* counts;
    MPI_Aint*  displs;
  };

  auto buffer = Buffer::create(
    page_ceil<MPI_Count>(mpi_world_size),
    page_ceil<MPI_Aint>(mpi_world_size));

  void* memory = buffer.data();
  auto* counts = borrow<MPI_Count>(memory, mpi_world_size);
  auto* displs = borrow<MPI_Aint>(memory, mpi_world_size);

  return Result{ std::move(buffer), counts, displs };
}

/**
 * @brief Allgather a single element count from each rank.
 *
 * @param send_count Local element count.
 * @param counts Out array of length mpi_world_size receiving one MPI_Count per rank.
 *
 * @pre counts points to at least mpi_world_size entries.
 */
inline void
mpi_basic_allgatherv_counts(MPI_Count send_count, MPI_Count* counts)
{
  MPI_Allgather_c(&send_count, 1, MPI_COUNT, counts, 1, MPI_COUNT, MPI_COMM_WORLD);
}

/**
 * @brief All-to-all exchange of per-destination element counts (MPI-4 “_c” variant).
 *
 * @param send_counts Array of length mpi_world_size: send_counts[j] is the number of elements
 *                    this rank will send to rank j.
 * @param recv_counts Out array of length mpi_world_size: recv_counts[j] becomes the number of
 *                    elements this rank will receive from rank j.
 *
 * @pre send_counts and recv_counts point to at least mpi_world_size entries.
 *
 * @note Convenience wrapper around MPI_Alltoall_c with MPI_COUNT on MPI_COMM_WORLD, typically
 *       used to derive receive counts for Alltoallv-style exchanges.
 */
inline void
mpi_basic_alltoallv_counts(MPI_Count const* send_counts, MPI_Count* recv_counts)
{
  MPI_Alltoall_c(send_counts, 1, MPI_COUNT, recv_counts, 1, MPI_COUNT, MPI_COMM_WORLD);
}

/**
 * @brief Compute byte displacements from element counts for type T (MPI-4 “_c” variants).
 *
 * @param counts Per-rank element counts (MPI_Count[mpi_world_size]).
 * @param displs Out byte displacements (MPI_Aint[mpi_world_size]).
 * @return Total element count (sum of counts).
 */
template<typename T>
auto
mpi_basic_displs(MPI_Count const* counts, MPI_Aint* displs) -> MPI_Count
{
  MPI_Count count = 0;
  for (i32 i = 0; i < mpi_world_size; ++i) {
    displs[i] = static_cast<MPI_Aint>(count) * static_cast<MPI_Aint>(sizeof(T));
    count += counts[i];
  }
  return count;
}

template<MpiBasicConcept T>
void
mpi_basic_allreduce_inplace(T* send_buffer, MPI_Count send_count, MPI_Op op)
{
  MPI_Allreduce_c(MPI_IN_PLACE, send_buffer, send_count, mpi_basic_type<T>, op, MPI_COMM_WORLD);
}

template<MpiBasicConcept T>
auto
mpi_basic_allreduce_single(T const& send_value, MPI_Op op) -> T
{
  T recv_value;
  MPI_Allreduce_c(&send_value, &recv_value, 1, mpi_basic_type<T>, op, MPI_COMM_WORLD);
  return recv_value;
}

template<typename T>
void
mpi_basic_allreduce_inplace(T* send_buffer, MPI_Count send_count, MPI_Datatype datatype, MPI_Op op)
{
  MPI_Allreduce_c(MPI_IN_PLACE, send_buffer, send_count, datatype, op, MPI_COMM_WORLD);
}

template<typename T>
auto
mpi_basic_allreduce_single(T const& send_value, MPI_Datatype datatype, MPI_Op op) -> T
{
  T recv_value;
  MPI_Allreduce_c(&send_value, &recv_value, 1, datatype, op, MPI_COMM_WORLD);
  return recv_value;
}

template<MpiBasicConcept T>
void
mpi_basic_alltoallv(
  T const*         send_buffer,
  MPI_Count const* send_counts,
  MPI_Aint const*  send_displs,
  T*               recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  recv_displs)
{
  MPI_Alltoallv_c(send_buffer, send_counts, send_displs, mpi_basic_type<T>, recv_buffer, recv_counts, recv_displs, mpi_basic_type<T>, MPI_COMM_WORLD);
}

inline auto
mpi_basic_alltoallv(
  void const*      send_buffer,
  MPI_Count const* send_counts,
  MPI_Aint const*  send_displs,
  void*            recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  recv_displs,
  MPI_Datatype     datatype)

{
  MPI_Alltoallv_c(send_buffer, send_counts, send_displs, datatype, recv_buffer, recv_counts, recv_displs, datatype, MPI_COMM_WORLD);
}

/**
 * @brief Typed wrapper for MPI_Allgatherv_c with element counts and byte displacements.
 *
 * @param send_buffer Pointer to local elements.
 * @param send_count Number of local elements.
 * @param recv_buffer Pointer to receive buffer holding sum(counts) elements.
 * @param recv_counts Per-rank element counts (MPI_Count[mpi_world_size]).
 * @param recv_displs Per-rank byte displacements (MPI_Aint[mpi_world_size]).
 */
template<MpiBasicConcept T>
void
mpi_basic_allgatherv(
  T const*         send_buffer,
  MPI_Count        send_count,
  T*               recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  recv_displs)
{
  MPI_Allgatherv_c(send_buffer, send_count, mpi_basic_type<T>, recv_buffer, recv_counts, recv_displs, mpi_basic_type<T>, MPI_COMM_WORLD);
}

inline void
mpi_basic_allgatherv(
  void const*      send_buffer,
  MPI_Count        send_count,
  void*            recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  recv_displs,
  MPI_Datatype     datatype)
{
  MPI_Allgatherv_c(send_buffer, send_count, datatype, recv_buffer, recv_counts, recv_displs, datatype, MPI_COMM_WORLD);
}

/**
 * @brief Allocate-and-call convenience wrapper for MPI_Allgatherv_c.
 *
 * @param send_buffer Pointer to local elements.
 * @param send_count Number of local elements.
 * @return { buffer, recv, count } where:
 *         - buffer owns the receive storage (count elements),
 *         - recv points into buffer,
 *         - count is the total number of received elements.
 *
 * @note Keeps internal counts/displacements storage alive until after the MPI call.
 */
template<MpiBasicConcept T>
auto
mpi_basic_allgatherv(T const* send_buffer, MPI_Count send_count)
{
  struct Result
  {
    Buffer    buffer;
    T*        recv;
    MPI_Count count;
  };

  Result result;
  auto [buffer, counts, displs] = mpi_basic_counts_and_displs();
  mpi_basic_allgatherv_counts(send_count, counts);
  result.count  = mpi_basic_displs<T>(counts, displs);
  result.buffer = Buffer::create<T>(result.count);
  result.recv   = static_cast<T*>(result.buffer.data());

  mpi_basic_allgatherv<T>(send_buffer, send_count, result.recv, counts, displs);
  return result;
}

/**
 * @brief In-place, zero-extra-memory partition of a send buffer into per-rank buckets.
 *
 * Reorders send_buffer so that items destined for rank r form a contiguous segment starting at
 * byte displacement send_displs[r] and spanning send_counts[r] elements. The function uses
 * send_displs as temporary per-rank “end-of-ordered” element indices during processing and
 * overwrites entries with finalized byte displacements as soon as a rank is completed.
 *
 * @tparam T  Item type stored in send_buffer.
 * @tparam Fn Callable with signature i32(T const&), returning the destination rank in [0, mpi_world_size).
 *
 * @param send_buffer [in,out] Array of length sum(send_counts) holding the items to be partitioned.
 * @param send_counts [in]     Per-rank item counts (length mpi_world_size).
 * @param send_displs [in,out] Scratch/output array (length mpi_world_size). Initialized internally
 *                             to element-index prefix sums; on return contains byte displacements
 *                             suitable for MPI “_c” collectives (multiples of sizeof(T)).
 * @param rank_of     [in]     Function determining the target rank for an item.
 *
 * @pre mpi_world_size is initialized and > 0.
 * @pre send_buffer != nullptr, send_counts != nullptr, send_displs != nullptr.
 * @pre For all r in [0, mpi_world_size): send_counts[r] equals the number of items x with rank_of(x) == r.
 * @pre Total item count and per-rank counts fit in MPI_Aint for index/byte arithmetic.
 *
 * @post send_buffer is partitioned by destination rank (not stable).
 * @post For all r: send_displs[r] is the byte offset of rank r’s segment; segment size is send_counts[r].
 *
 * @complexity O(N + R), where N = sum(send_counts) and R = mpi_world_size. No additional dynamic memory.
 */
template<typename T, typename Fn>
  requires(std::convertible_to<std::invoke_result_t<Fn, T const&>, i32>)
void
mpi_basic_inplace_partition_by_rank(T* send_buffer, MPI_Count const* send_counts, MPI_Aint* send_displs, Fn&& rank_of)
{
  DEBUG_ASSERT_NE(send_buffer, nullptr);
  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(send_displs, nullptr);

  // Initialize send_displs as element-index prefix sums (scratch).
  // Entries will be overwritten with byte displacements once a rank is finalized.
  send_displs[0] = 0;
  for (i32 r = 1; r < mpi_world_size; ++r) {
    send_displs[r] = send_displs[r - 1] + static_cast<MPI_Aint>(send_counts[r - 1]);
  }

  // In-place partition:
  // For each rank r, process its segment [rank_beg, rank_end):
  //   - ordered_items starts at the end of the already-ordered prefix for r,
  //   - if the current item targets r, advance,
  //   - otherwise swap it into the (growing) end of the correct target bucket.
  // Progress is guaranteed: each step places one item into the correct bucket.
  MPI_Aint ordered_items = 0;
  for (i32 rank = 0; rank < mpi_world_size; ++rank) {
    auto const rank_beg = ordered_items;                                       // start of r’s bucket (element index)
    auto const rank_end = rank_beg + static_cast<MPI_Aint>(send_counts[rank]); // end of r’s bucket (element index)

    ordered_items = send_displs[rank]; // end of already-ordered prefix within r’s bucket (element index)

    // Finalize r’s displacement now: convert to bytes. For ranks > r, send_displs still holds element indices.
    send_displs[rank] = rank_beg * static_cast<MPI_Aint>(sizeof(T));

    while (ordered_items < rank_end) {
      auto const target_rank = static_cast<i32>(rank_of(send_buffer[ordered_items]));
      DEBUG_ASSERT_GE(target_rank, rank, "all messages of target rank < current rank should be ordered already");
      DEBUG_ASSERT_LT(target_rank, mpi_world_size, "target rank out of bounds");

      if (target_rank == rank) {
        ++ordered_items; // item is already in the correct bucket segment
      } else {
        // For target ranks > r, send_displs[target_rank] is still an element-index “end-of-ordered” pointer.
        auto const target_rank_ordered_end = send_displs[target_rank]++;
        std::swap(send_buffer[ordered_items], send_buffer[target_rank_ordered_end]);
      }
    }
  }
}
