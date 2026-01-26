#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/base.hpp>
#include <kaspan/graph/block_cyclic_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/cyclic_part.hpp>
#include <kaspan/graph/explicit_continuous_part.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/graph/trivial_slice_part.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>
#include <kaspan/util/scope_guard.hpp>

using namespace kaspan;

template<typename Part>
void
test_consistency(
  Part const& part)
{
  vertex_t const n       = part.n();
  vertex_t const local_n = part.local_n();
  i32 const      rank    = part.world_rank();

  // local -> global -> local
  for (vertex_t k = 0; k < local_n; ++k) {
    vertex_t const u = part.to_global(k);
    ASSERT(part.has_local(u));
    ASSERT_EQ(part.to_local(u), k);
    ASSERT_EQ(part.world_rank_of(u), rank);
  }

  // global -> local -> global
  vertex_t count = 0;
  for (vertex_t u = 0; u < n; ++u) {
    if (part.has_local(u)) {
      vertex_t const k = part.to_local(u);
      ASSERT_LT(k, local_n);
      ASSERT_EQ(part.to_global(k), u);
      ASSERT_EQ(part.world_rank_of(u), rank);
      ++count;
    } else {
      ASSERT_NE(part.world_rank_of(u), rank);
    }
  }
  ASSERT_EQ(count, local_n);

  if constexpr (Part::continuous) {
    ASSERT_EQ(part.end() - part.begin(), local_n);
    for (vertex_t k = 0; k < local_n; ++k) {
      ASSERT_EQ(part.to_global(k), part.begin() + k);
    }
  }
}

template<typename Part>
void
test_partition(
  Part const& part)
{
  test_consistency(part);
  test_consistency(part.view());
  for (i32 r = 0; r < part.world_size(); ++r) {
    test_consistency(part.world_part_of(r));
  }
}

void
test_all_partitions()
{
  for (vertex_t n = 0; n < 100; ++n) {
    test_partition(single_part{ n });
    test_partition(cyclic_part{ n });
    test_partition(block_cyclic_part{ n });    // default block size
    test_partition(block_cyclic_part{ n, 1 }); // minimum block size
    test_partition(block_cyclic_part{ n, 7 }); // arbitrary block size
    test_partition(trivial_slice_part{ n });
    test_partition(balanced_slice_part{ n });

    // Explicit partitions
    {
      balanced_slice_part      balanced{ n };
      explicit_continuous_part cont{ n, balanced.begin(), balanced.end() };
      test_partition(cont);

      explicit_sorted_part sorted{ n, balanced.end() };
      test_partition(sorted);
    }
  }
}

template<bool sorted_variant>
void
test_explicit_continuous_part()
{
  auto const part = [] {
    if constexpr (sorted_variant) {
      if (mpi_basic::world_root) {
        return explicit_sorted_part{ 10, 3 };
      }
      if (mpi_basic::world_rank == 1) {
        return explicit_sorted_part{ 10, 9 };
      }
      ASSERT_EQ(mpi_basic::world_rank, 2);
      return explicit_sorted_part{ 10, 10 };
    } else {
      if (mpi_basic::world_root) {
        return explicit_continuous_part{ 10, 0, 3 };
      }
      if (mpi_basic::world_rank == 1) {
        return explicit_continuous_part{ 10, 3, 9 };
      }
      ASSERT_EQ(mpi_basic::world_rank, 2);
      return explicit_continuous_part{ 10, 9, 10 };
    }
  }();

  ASSERT_EQ(part.n(), 10);
  ASSERT_EQ(part.world_rank(), mpi_basic::world_rank);
  ASSERT_EQ(part.world_size(), mpi_basic::world_size);
  ASSERT_EQ(part.world_rank_of(0), 0);
  ASSERT_EQ(part.world_rank_of(1), 0);
  ASSERT_EQ(part.world_rank_of(2), 0);
  ASSERT_EQ(part.world_rank_of(3), 1);
  ASSERT_EQ(part.world_rank_of(4), 1);
  ASSERT_EQ(part.world_rank_of(5), 1);
  ASSERT_EQ(part.world_rank_of(6), 1);
  ASSERT_EQ(part.world_rank_of(7), 1);
  ASSERT_EQ(part.world_rank_of(8), 1);
  ASSERT_EQ(part.world_rank_of(9), 2);

  if (mpi_basic::world_rank == 0) {
    ASSERT_EQ(part.begin(), 0);
    ASSERT_EQ(part.end(), 3);
  }
  if (mpi_basic::world_rank == 1) {
    ASSERT_EQ(part.begin(), 3);
    ASSERT_EQ(part.end(), 9);
  }
  if (mpi_basic::world_rank == 2) {
    ASSERT_EQ(part.begin(), 9);
    ASSERT_EQ(part.end(), 10);
  }
}

int
main(
  int    argc,
  char** argv)
{
  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  static_assert(part_view_concept<single_part_view>);
  static_assert(part_view_concept<cyclic_part_view>);
  static_assert(part_view_concept<block_cyclic_part_view>);
  static_assert(part_view_concept<trivial_slice_part_view>);
  static_assert(part_view_concept<balanced_slice_part_view>);
  static_assert(part_view_concept<explicit_continuous_part_view>);
  static_assert(part_view_concept<explicit_sorted_part_view>);

  static_assert(part_concept<single_part>);
  static_assert(part_concept<cyclic_part>);
  static_assert(part_concept<block_cyclic_part>);
  static_assert(part_concept<trivial_slice_part>);
  static_assert(part_concept<balanced_slice_part>);
  static_assert(part_concept<explicit_continuous_part>);
  static_assert(part_concept<explicit_sorted_part>);

  if (mpi_basic::world_size == 3) {
    test_explicit_continuous_part<false>();
    test_explicit_continuous_part<true>();
  }

  test_all_partitions();
}
