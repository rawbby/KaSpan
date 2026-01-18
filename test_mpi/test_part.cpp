#include <kaspan/debug/assert.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/block_cyclic_part.hpp>
#include <kaspan/graph/concept.hpp>
#include <kaspan/graph/cyclic_part.hpp>
#include <kaspan/graph/explicit_continuous_part.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/graph/trivial_slice_part.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/util/mpi_basic.hpp>
#include <kaspan/util/pp.hpp>
#include <kaspan/util/scope_guard.hpp>

using namespace kaspan;

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
  static_assert(part_view_concept<balanced_slice_part_view>);
  static_assert(part_view_concept<explicit_continuous_part_view>);
  static_assert(part_view_concept<explicit_sorted_part_view>);

  static_assert(part_concept<single_part>);
  static_assert(part_concept<cyclic_part>);
  static_assert(part_concept<block_cyclic_part>);
  static_assert(part_concept<trivial_slice_part>);
  static_assert(part_concept<balanced_slice_part>);
  static_assert(part_concept<balanced_slice_part>);
  static_assert(part_concept<explicit_continuous_part>);
  static_assert(part_concept<explicit_sorted_part>);

  if (mpi_basic::world_size == 3) {
    test_explicit_continuous_part<false>();
    test_explicit_continuous_part<true>();
  }
}
