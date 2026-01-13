#include <kaspan/debug/assert_eq.hpp>
#include <kaspan/debug/sub_process.hpp>
#include <kaspan/memory/buffer.hpp>
#include <kaspan/mpi_basic/world.hpp>
#include <kaspan/scc/base.hpp>
#include <kaspan/scc/part.hpp>
#include <kaspan/util/arithmetic.hpp>
#include <kaspan/util/pp.hpp>
#include <kaspan/util/scope_guard.hpp>

using namespace kaspan;

template<bool sorted_variant>
void
test_explicit_continuous_world_part()
{
  auto  storage = buffer(2 * mpi_basic::world_size * sizeof(vertex_t));
  auto* memory  = storage.data();

  auto const part = [&memory] {
    if constexpr (sorted_variant) {
      if (mpi_basic::world_root) {
        return explicit_sorted_continuous_world_part{ 10, 3, mpi_basic::world_rank, mpi_basic::world_size, &memory };
      }
      if (mpi_basic::world_rank == 1) {
        return explicit_sorted_continuous_world_part{ 10, 9, mpi_basic::world_rank, mpi_basic::world_size, &memory };
      }
      ASSERT_EQ(mpi_basic::world_rank, 2);
      return explicit_sorted_continuous_world_part{ 10, 10, mpi_basic::world_rank, mpi_basic::world_size, &memory };
    } else {
      if (mpi_basic::world_root) {
        return explicit_continuous_world_part{ 10, 0, 3, mpi_basic::world_rank, mpi_basic::world_size, &memory };
      }
      if (mpi_basic::world_rank == 1) {
        return explicit_continuous_world_part{ 10, 3, 9, mpi_basic::world_rank, mpi_basic::world_size, &memory };
      }
      ASSERT_EQ(mpi_basic::world_rank, 2);
      return explicit_continuous_world_part{ 10, 9, 10, mpi_basic::world_rank, mpi_basic::world_size, &memory };
    }
  }();

  ASSERT_EQ(part.n, 10);
  ASSERT_EQ(part.world_rank, mpi_basic::world_rank);
  ASSERT_EQ(part.world_size, mpi_basic::world_size);
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
    ASSERT_EQ(part.begin, 0);
    ASSERT_EQ(part.end, 3);
  }
  if (mpi_basic::world_rank == 1) {
    ASSERT_EQ(part.begin, 3);
    ASSERT_EQ(part.end, 9);
  }
  if (mpi_basic::world_rank == 2) {
    ASSERT_EQ(part.begin, 9);
    ASSERT_EQ(part.end, 10);
  }
}

i32
main(i32 argc, char** argv)
{
  static_assert(part_concept<explicit_continuous_part>);
  static_assert(world_part_concept<cyclic_part>);
  static_assert(world_part_concept<block_cyclic_part>);
  static_assert(world_part_concept<trivial_slice_part>);
  static_assert(world_part_concept<balanced_slice_part>);
  static_assert(world_part_concept<explicit_continuous_world_part>);
  static_assert(world_part_concept<explicit_sorted_continuous_world_part>);

  mpi_sub_process(argc, argv);
  KASPAN_DEFAULT_INIT();

  if (mpi_basic::world_size == 3) {
    test_explicit_continuous_world_part<false>();
    test_explicit_continuous_world_part<true>();
  }
}
