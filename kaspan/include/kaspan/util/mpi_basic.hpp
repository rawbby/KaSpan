#pragma once

#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/debug.hpp>
#include <kaspan/debug/valgrind.hpp>
#include <kaspan/memory/borrow.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/integral_cast.hpp>
#include <kaspan/util/pp.hpp>
#include <kaspan/util/scope_guard.hpp>

#include <mpi.h>

#include <chrono>
#include <numeric>

namespace kaspan::mpi_basic {

/**
 * @brief The default communicator used by mpi_basic.
 */
inline MPI_Comm comm_world = MPI_COMM_WORLD;

/**
 * @brief Whether this rank is the root (rank 0).
 */
inline bool world_root = true;

/**
 * @brief The rank of the process in MPI_COMM_WORLD.
 */
inline i32 world_rank = 0;

/**
 * @brief The size of MPI_COMM_WORLD.
 */
inline i32 world_size = 1;

// clang-format off
/**
 * @brief Mapping from C++ types to MPI datatypes.
 */
template<typename T>
constexpr inline auto type =
  byte_c<T> ? MPI_BYTE     :
  i8_c<T>   ? MPI_INT8_T   :
  i16_c<T>  ? MPI_INT16_T  :
  i32_c<T>  ? MPI_INT32_T  :
  i64_c<T>  ? MPI_INT64_T  :
  u8_c<T>   ? MPI_UINT8_T  :
  u16_c<T>  ? MPI_UINT16_T :
  u32_c<T>  ? MPI_UINT32_T :
  u64_c<T>  ? MPI_UINT64_T :
  f32_c<T>  ? MPI_FLOAT    :
  f64_c<T>  ? MPI_DOUBLE   :
  MPI_DATATYPE_NULL;
// clang-format on

/**
 * @brief mpi_type_c for types that have a corresponding MPI datatype.
 */
template<typename T>
concept mpi_type_c = type<T> != MPI_DATATYPE_NULL;

constexpr inline auto byte_t = MPI_BYTE;
constexpr inline auto i8_t   = MPI_INT8_T;
constexpr inline auto i16_t  = MPI_INT16_T;
constexpr inline auto i32_t  = MPI_INT32_T;
constexpr inline auto i64_t  = MPI_INT64_T;
constexpr inline auto u8_t   = MPI_UINT8_T;
constexpr inline auto u16_t  = MPI_UINT16_T;
constexpr inline auto u32_t  = MPI_UINT32_T;
constexpr inline auto u64_t  = MPI_UINT64_T;

using Datatype = MPI_Datatype;
using Op       = MPI_Op;

constexpr inline Datatype datatype_null = MPI_DATATYPE_NULL;
constexpr inline Op       op_null       = MPI_OP_NULL;

constexpr inline Op sum  = MPI_SUM;
constexpr inline Op min  = MPI_MIN;
constexpr inline Op max  = MPI_MAX;
constexpr inline Op lor  = MPI_LOR;
constexpr inline Op land = MPI_LAND;

inline void
get_address(
  void const* location,
  MPI_Aint*   address)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Get_address(location, address);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
type_create_struct(
  int             count,
  int const*      blocklengths,
  MPI_Aint const* displs,
  Datatype const* types,
  Datatype*       newtype)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Type_create_struct(count, blocklengths, displs, types, newtype);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
type_commit(
  Datatype* type)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Type_commit(type);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
type_free(
  Datatype* type)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Type_free(type);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
type_get_extent(
  Datatype  type,
  MPI_Aint* lb,
  MPI_Aint* extent)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Type_get_extent(type, lb, extent);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
op_create(
  MPI_User_function* user_fn,
  int                commute,
  Op*                op)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Op_create(user_fn, commute, op);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline void
op_free(
  Op* op)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Op_free(op);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

inline auto
extent_of(
  MPI_Datatype datatype) -> MPI_Aint
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  MPI_Aint lb;
  MPI_Aint extent;

  KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(lb);
  KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(extent);

  [[maybe_unused]] auto const rc0 = MPI_Type_get_extent(datatype, &lb, &extent);
  DEBUG_ASSERT_EQ(rc0, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(extent);
  DEBUG_ASSERT_GE(lb, 0);
  DEBUG_ASSERT_GT(extent, 0);

  if constexpr (KASPAN_DEBUG) {
    int type_size;

    KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(type_size);

    auto const rc1 = MPI_Type_size(datatype, &type_size);
    ASSERT_EQ(rc1, MPI_SUCCESS);

    KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(type_size);
    ASSERT_EQ(type_size, extent);
  }

  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  return extent;
}

template<mpi_type_c T>
auto
extent_of() -> MPI_Aint
{
  return extent_of(type<T>);
}

/**
 * @brief Initialize MPI (MPI_Init) and arrange automatic MPI_Finalize via SCOPE_GUARD.
 * Populates world_rank, world_size, and world_root.
 *
 * Call once per process before using the wrappers. Not thread-safe.
 */
#define MPI_INIT()                                                                                                                                                                 \
  {                                                                                                                                                                                \
    [[maybe_unused]] auto const rc0 = MPI_Init(nullptr, nullptr);                                                                                                                  \
    DEBUG_ASSERT_EQ(rc0, MPI_SUCCESS);                                                                                                                                             \
  }                                                                                                                                                                                \
  SCOPE_GUARD(MPI_Finalize());                                                                                                                                                     \
  {                                                                                                                                                                                \
    [[maybe_unused]] auto const rc1 = MPI_Comm_rank(MPI_COMM_WORLD, &kaspan::mpi_basic::world_rank);                                                                               \
    DEBUG_ASSERT_EQ(rc1, MPI_SUCCESS);                                                                                                                                             \
    [[maybe_unused]] auto const rc2 = MPI_Comm_size(MPI_COMM_WORLD, &kaspan::mpi_basic::world_size);                                                                               \
    DEBUG_ASSERT_EQ(rc2, MPI_SUCCESS);                                                                                                                                             \
    kaspan::mpi_basic::world_root = kaspan::mpi_basic::world_rank == 0;                                                                                                            \
  }                                                                                                                                                                                \
  KASPAN_MEMCHECK_PRINTF("[INIT] rank %d initialized\n", kaspan::mpi_basic::world_rank); /* this print should help to map the rank to the pid in valgrind logs */                  \
  ((void)0)

/**
 * @brief Borrow counts and displacements arrays from memory.
 *
 * @param memory Pointer to memory to borrow from.
 * @return PACK(counts, displs)
 */
inline auto
counts_and_displs(
  void** memory)
{
  DEBUG_ASSERT_NE(memory, nullptr);
  DEBUG_ASSERT_NE(*memory, nullptr);
  auto* counts = borrow_array<MPI_Count>(memory, world_size);
  auto* displs = borrow_array<MPI_Aint>(memory, world_size);
  return PACK(counts, displs);
}

/**
 * @brief Allocate temporary counts and displacements arrays for MPI-4 "_c" collectives.
 *
 * @return PACK(buffer, counts, displs)
 */
inline auto
counts_and_displs()
{
  auto  buffer = make_buffer<MPI_Count, MPI_Aint>(world_size, world_size);
  auto* memory = buffer.data();
  auto* counts = borrow_array<MPI_Count>(&memory, world_size);
  auto* displs = borrow_array<MPI_Aint>(&memory, world_size);
  return PACK(buffer, counts, displs);
}

/**
 * @brief Calculate displacements from counts.
 *
 * @param counts Per-rank element counts.
 * @param displs Out array of displacements.
 * @return Total number of elements.
 */
inline auto
displs(
  MPI_Count const* counts,
  MPI_Aint*        displs) -> MPI_Count
{
  DEBUG_ASSERT_NE(counts, nullptr);
  DEBUG_ASSERT_NE(displs, nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(displs, world_size * sizeof(MPI_Aint));

  MPI_Count count = 0;
  for (i32 i = 0; i < world_size; ++i) {
    DEBUG_ASSERT_GE(counts[i], 0);
    displs[i] = integral_cast<MPI_Aint>(count);
    count += counts[i];
  }

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(displs, world_size * sizeof(MPI_Aint));

  return count;
}

/**
 * @brief MPI barrier on comm_world.
 */
inline void
barrier()
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  [[maybe_unused]] auto const rc = MPI_Barrier(comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief In-place, zero-extra-memory partition of a send buffer into per-rank buckets.
 *
 * @tparam T  Item type stored in send_buffer.
 * @tparam fn_t Callable with signature i32(T const&), returning the destination rank.
 */
template<typename T,
         typename fn_t>
  requires(std::convertible_to<std::invoke_result_t<fn_t,
                                                    T const&>,
                               i32>)
void
inplace_partition_by_rank(
  T*               send_buffer,
  MPI_Count const* send_counts,
  MPI_Aint*        send_displs,
  fn_t&&           rank_of)
{
  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(send_displs, nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(send_displs, world_size * sizeof(MPI_Aint));

  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const sned_count = std::accumulate(send_counts, send_counts + world_size, integral_cast<MPI_Count>(0)));

  DEBUG_ASSERT(sned_count == 0 || send_buffer != nullptr);
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, sned_count * sizeof(T));

  // Initialize send_displs as element-index prefix sums.
  send_displs[0] = 0;
  for (i32 r = 1; r < world_size; ++r) {
    send_displs[r] = send_displs[r - 1] + integral_cast<MPI_Aint>(send_counts[r - 1]);
  }

  MPI_Aint ordered_items = 0;
  for (i32 rank = 0; rank < world_size; ++rank) {
    auto const rank_beg = ordered_items;
    auto const rank_end = rank_beg + integral_cast<MPI_Aint>(send_counts[rank]);

    ordered_items = send_displs[rank];

    send_displs[rank] = rank_beg;

    while (ordered_items < rank_end) {
      auto const target_rank = integral_cast<i32>(rank_of(send_buffer[ordered_items]));
      DEBUG_ASSERT_GE(target_rank, rank);
      DEBUG_ASSERT_LT(target_rank, world_size);

      if (target_rank == rank) {
        ++ordered_items;
      } else {
        auto const target_rank_ordered_end = send_displs[target_rank]++;
        std::swap(send_buffer[ordered_items], send_buffer[target_rank_ordered_end]);
      }
    }
  }
}

/**
 * @brief All-reduce for a single typed value.
 */
template<typename T>
auto
allreduce_single(
  T const&     send_value,
  MPI_Datatype datatype,
  MPI_Op       op) -> T
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);

  KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(send_value);

  T recv_value;
  KASPAN_MEMCHECK_MAKE_VALUE_UNDEFINED(recv_value);

  [[maybe_unused]] auto const rc = MPI_Allreduce_c(&send_value, &recv_value, 1, datatype, op, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_VALUE_IS_DEFINED(recv_value);

  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  return recv_value;
}

/**
 * @brief All-reduce for a single typed value.
 */
template<mpi_type_c T>
auto
allreduce_single(
  T const& send_value,
  MPI_Op   op) -> T
{
  return allreduce_single(send_value, type<T>, op);
}

/**
 * @brief Get the maximum current time across all ranks.
 */
inline auto
allreduce_max_time() -> u64
{
  using namespace std::chrono;
  auto const now          = steady_clock::now();
  auto const timestamp    = now.time_since_epoch();
  auto const timestamp_ns = duration_cast<nanoseconds>(timestamp);
  return allreduce_single(integral_cast<u64>(timestamp_ns.count()), max);
}

/**
 * @brief Gathers data from all processes and distributes it to all processes (overload with specific MPI_Datatype).
 *
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param send_type The MPI datatype of the send buffer.
 * @param recv_buffer The buffer to receive the data.
 * @param recv_count The number of elements to receive from each process.
 * @param recv_type The MPI datatype of the receive buffer.
 */
inline void
allgather(
  void const*  send_buffer,
  MPI_Count    send_count,
  MPI_Datatype send_type,
  void*        recv_buffer,
  MPI_Count    recv_count,
  MPI_Datatype recv_type)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(send_type, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(recv_type, MPI_DATATYPE_NULL);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  DEBUG_ASSERT_GE(recv_count, 0);
  DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

  IF(KASPAN_MEMCHECK, auto const send_extent = extent_of(send_type));
  IF(KASPAN_MEMCHECK, auto const recv_extent = extent_of(recv_type));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * send_extent);
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * world_size * recv_extent);
  KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * world_size * recv_extent);

  [[maybe_unused]] auto const rc = MPI_Allgather_c(send_buffer, send_count, send_type, recv_buffer, recv_count, recv_type, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * world_size * recv_extent);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Gathers data from all processes and distributes it to all processes.
 *
 * @tparam T The type of the data to gather.
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param recv_buffer The buffer to receive the data.
 * @param recv_count The number of elements to receive from each process.
 */
template<mpi_type_c T>
void
allgather(
  T const*  send_buffer,
  MPI_Count send_count,
  T*        recv_buffer,
  MPI_Count recv_count)
{
  allgather(send_buffer, send_count, type<T>, recv_buffer, recv_count, type<T>);
}

/**
 * @brief Gathers data from all processes and distributes it to all processes (overload for single elements).
 *
 * @tparam T The type of the data to gather.
 * @param send_value The value to send.
 * @param recv_buffer The buffer to receive the data.
 */
template<mpi_type_c T>
void
allgather(
  T const& send_value,
  T*       recv_buffer)
{
  allgather(&send_value, 1, type<T>, recv_buffer, 1, type<T>);
}

/**
 * @brief Allgather a single count from each rank.
 *
 * @param send_count Local count.
 * @param counts Out array of length world_size receiving one MPI_Count per rank.
 */
inline void
allgather_counts(
  MPI_Count  send_count,
  MPI_Count* counts)
{
  allgather(&send_count, 1, MPI_COUNT, counts, 1, MPI_COUNT);
}

/**
 * @brief Untyped wrapper for MPI_Allgatherv_c.
 */
inline void
allgatherv(
  void const*      send_buffer,
  MPI_Count        send_count,
  void*            recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  recv_displs,
  MPI_Datatype     datatype)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_displs, world_size * sizeof(MPI_Aint));

  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const recv_count = std::accumulate(recv_counts, recv_counts + world_size, integral_cast<MPI_Count>(0)));
  IF(KASPAN_MEMCHECK, auto const extent = extent_of(datatype));

  DEBUG_ASSERT_GE(recv_count, 0);
  DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * extent);
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * extent);
  KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * extent);

  [[maybe_unused]] auto const rc = MPI_Allgatherv_c(send_buffer, send_count, datatype, recv_buffer, recv_counts, recv_displs, datatype, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * extent);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Typed wrapper for MPI_Allgatherv_c with element counts and displacements.
 *
 * @param send_buffer Pointer to local elements.
 * @param send_count Number of local elements.
 * @param recv_buffer Pointer to receive buffer holding sum(counts) elements.
 * @param recv_counts Per-rank element counts (MPI_Count[world_size]).
 * @param recv_displs Per-rank displacements (MPI_Aint[world_size]).
 */
template<mpi_type_c T>
void
allgatherv(
  T const*         send_buffer,
  MPI_Count        send_count,
  T*               recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  recv_displs)
{
  allgatherv(send_buffer, send_count, recv_buffer, recv_counts, recv_displs, type<T>);
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
 */
template<mpi_type_c T>
auto
allgatherv(
  T const*  send_buffer,
  MPI_Count send_count)
{
  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  auto [buffer, c, d] = counts_and_displs();
  allgather_counts(send_count, c);

  auto const count = displs(c, d);
  auto       recv  = make_array<T>(count);

  allgatherv<T>(send_buffer, send_count, recv.data(), c, d);
  return PACK(recv, count);
}
/**
 * @brief In-place all-reduce for a buffer of untyped elements.
 */
inline void
allreduce_inplace(
  void*        send_buffer,
  MPI_Count    send_count,
  MPI_Datatype datatype,
  MPI_Op       op)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(op, MPI_OP_NULL);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  IF(KASPAN_MEMCHECK, auto const extent = extent_of(datatype));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * extent);

  [[maybe_unused]] auto const rc = MPI_Allreduce_c(MPI_IN_PLACE, send_buffer, send_count, datatype, op, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * extent);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief In-place all-reduce for a buffer of typed elements.
 */
template<mpi_type_c T>
void
allreduce_inplace(
  T*        send_buffer,
  MPI_Count send_count,
  MPI_Op    op)
{
  allreduce_inplace(send_buffer, send_count, type<T>, op);
}

/**
 * @brief In-place all-reduce for a single typed value.
 */
template<mpi_type_c T>
void
allreduce_inplace(
  T&     send_value,
  MPI_Op op)
{
  allreduce_inplace(&send_value, 1, type<T>, op);
}

/**
 * @brief Untyped wrapper for MPI_Alltoallv_c.
 */
inline void
alltoallv(
  void const*      send_buffer,
  MPI_Count const* send_counts,
  MPI_Aint const*  send_displs,
  void*            recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  recv_displs,
  MPI_Datatype     datatype)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(datatype, MPI_DATATYPE_NULL);

  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(send_displs, nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);
  DEBUG_ASSERT_NE(recv_displs, nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_displs, world_size * sizeof(MPI_Aint));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_displs, world_size * sizeof(MPI_Aint));

  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const send_count = std::accumulate(send_counts, send_counts + world_size, integral_cast<MPI_Count>(0)));
  IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const recv_count = std::accumulate(recv_counts, recv_counts + world_size, integral_cast<MPI_Count>(0)));
  IF(KASPAN_MEMCHECK, auto const data_extent = extent_of(datatype));

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  DEBUG_ASSERT_GE(recv_count, 0);
  DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * data_extent);
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * data_extent);
  KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * data_extent);

  [[maybe_unused]] auto const rc = MPI_Alltoallv_c(send_buffer, send_counts, send_displs, datatype, recv_buffer, recv_counts, recv_displs, datatype, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * data_extent);
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Typed wrapper for MPI_Alltoallv_c.
 */
template<mpi_type_c T>
void
alltoallv(
  T const*         send_buffer,
  MPI_Count const* send_counts,
  MPI_Aint const*  send_displs,
  T*               recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  recv_displs)
{
  alltoallv(send_buffer, send_counts, send_displs, recv_buffer, recv_counts, recv_displs, type<T>);
}

/**
 * @brief All-to-all exchange of per-destination element counts (MPI-4 "_c" variant).
 *
 * @param send_counts Array of length world_size.
 * @param recv_counts Out array of length world_size.
 */
inline void
alltoallv_counts(
  MPI_Count const* send_counts,
  MPI_Count*       recv_counts)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(send_counts, nullptr);
  DEBUG_ASSERT_NE(recv_counts, nullptr);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_counts, world_size * sizeof(MPI_Count));
  KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_counts, world_size * sizeof(MPI_Count));

  [[maybe_unused]] auto const rc = MPI_Alltoall_c(send_counts, 1, MPI_COUNT, recv_counts, 1, MPI_COUNT, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_counts, world_size * sizeof(MPI_Count));
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Gathers data from all processes to the root process (untyped).
 */
inline void
gather(
  void const*  send_buffer,
  MPI_Count    send_count,
  MPI_Datatype send_type,
  void*        recv_buffer,
  MPI_Count    recv_count,
  MPI_Datatype recv_type,
  int          root = 0)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(send_type, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(recv_type, MPI_DATATYPE_NULL);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  IF(KASPAN_MEMCHECK, auto const send_extent = extent_of(send_type));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * send_extent);

  IF(KASPAN_MEMCHECK, auto const recv_extent = extent_of(recv_type));

  if (world_rank == root) {
    DEBUG_ASSERT_GE(recv_count, 0);
    DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

    KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * world_size * recv_extent);
    KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * world_size * recv_extent);
  }

  [[maybe_unused]] auto const rc = MPI_Gather_c(send_buffer, send_count, send_type, recv_buffer, recv_count, recv_type, root, comm_world);
  DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

  if (world_rank == root) {
    KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * world_size * recv_extent);
  }
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Gathers data from all processes to the root process.
 *
 * @tparam T The type of the data to gather.
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param recv_buffer The buffer to receive the data (significant only at root).
 * @param recv_count The number of elements to receive from each process (significant only at root).
 * @param root The rank of the root process.
 */
template<mpi_type_c T>
void
gather(
  T const*  send_buffer,
  MPI_Count send_count,
  T*        recv_buffer,
  MPI_Count recv_count,
  int       root = 0)
{
  gather(send_buffer, send_count, type<T>, recv_buffer, recv_count, type<T>, root);
}

/**
 * @brief Gathers data from all processes to the root process (overload for single elements).
 *
 * @tparam T The type of the data to gather.
 * @param send_value The value to send.
 * @param recv_buffer The buffer to receive the data (significant only at root).
 * @param root The rank of the root process.
 */
template<mpi_type_c T>
void
gather(
  T const& send_value,
  T*       recv_buffer,
  int      root = 0)
{
  gather(&send_value, 1, type<T>, recv_buffer, 1, type<T>, root);
}

/**
 * @brief Gathers data with varying counts from all processes to the root process (untyped).
 */
inline void
gatherv(
  void const*      send_buffer,
  MPI_Count        send_count,
  MPI_Datatype     send_type,
  void*            recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  displs,
  MPI_Datatype     recv_type,
  int              root = 0)
{
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
  DEBUG_ASSERT_NE(send_type, MPI_DATATYPE_NULL);
  DEBUG_ASSERT_NE(recv_type, MPI_DATATYPE_NULL);

  DEBUG_ASSERT_GE(send_count, 0);
  DEBUG_ASSERT(send_count == 0 || send_buffer != nullptr);

  IF(KASPAN_MEMCHECK, auto const send_extent = extent_of(send_type));
  KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(send_buffer, send_count * send_extent);

  IF(KASPAN_MEMCHECK, auto const recv_extent = extent_of(recv_type));

  if (world_rank == root) {
    DEBUG_ASSERT_NE(recv_counts, nullptr);
    DEBUG_ASSERT_NE(displs, nullptr);

    KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_counts, world_size * sizeof(MPI_Count));
    KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(displs, world_size * sizeof(MPI_Aint));

    IF(OR(KASPAN_DEBUG, KASPAN_MEMCHECK), auto const recv_count = std::accumulate(recv_counts, recv_counts + world_size, integral_cast<MPI_Count>(0)));

    DEBUG_ASSERT_GE(recv_count, 0);
    DEBUG_ASSERT(recv_count == 0 || recv_buffer != nullptr);

    KASPAN_MEMCHECK_CHECK_MEM_IS_ADDRESSABLE(recv_buffer, recv_count * recv_extent);
    KASPAN_MEMCHECK_MAKE_MEM_UNDEFINED(recv_buffer, recv_count * recv_extent);

    [[maybe_unused]] auto const rc = MPI_Gatherv_c(send_buffer, send_count, send_type, recv_buffer, recv_counts, displs, recv_type, root, comm_world);
    DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);

    KASPAN_MEMCHECK_CHECK_MEM_IS_DEFINED(recv_buffer, recv_count * recv_extent);
  } else {
    [[maybe_unused]] auto const rc = MPI_Gatherv_c(send_buffer, send_count, send_type, nullptr, nullptr, nullptr, recv_type, root, comm_world);
    DEBUG_ASSERT_EQ(rc, MPI_SUCCESS);
  }
  KASPAN_CALLGRIND_TOGGLE_COLLECT();
}

/**
 * @brief Gathers data with varying counts from all processes to the root process.
 *
 * @tparam T The type of the data to gather.
 * @param send_buffer The buffer containing the data to send.
 * @param send_count The number of elements to send.
 * @param recv_buffer The buffer to receive the data (significant only at root).
 * @param recv_counts The number of elements to receive from each process (significant only at root).
 * @param displs The displacements in the receive buffer (significant only at root).
 * @param root The rank of the root process.
 */
template<mpi_type_c T>
void
gatherv(
  T const*         send_buffer,
  MPI_Count        send_count,
  T*               recv_buffer,
  MPI_Count const* recv_counts,
  MPI_Aint const*  displs,
  int              root = 0)
{
  gatherv(send_buffer, send_count, type<T>, recv_buffer, recv_counts, displs, type<T>, root);
}

} // namespace kaspan::mpi_basic
