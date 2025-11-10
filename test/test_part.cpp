#include <debug/assert.hpp>
#include <debug/sub_process.hpp>
#include <memory/buffer.hpp>
#include <scc/part.hpp>
#include <util/scope_guard.hpp>

template<bool sorted_variant>
void
test_explicit_continuous_world_part()
{
  auto buffer = Buffer::create(2 * mpi_world_size * sizeof(vertex_t));
  auto* memory = buffer.data();

  auto const part = [&memory] {
    if constexpr (sorted_variant) {
      if (mpi_world_root)
        return ExplicitSortedContinuousWorldPart{ 10, 3, mpi_world_rank, mpi_world_size, memory };
      if (mpi_world_rank == 1)
        return ExplicitSortedContinuousWorldPart{ 10, 9, mpi_world_rank, mpi_world_size, memory };
      ASSERT_EQ(mpi_world_rank, 2);
      return ExplicitSortedContinuousWorldPart{ 10, 10, mpi_world_rank, mpi_world_size, memory };
    } else {
      if (mpi_world_root)
        return ExplicitContinuousWorldPart{ 10, 0, 3, mpi_world_rank, mpi_world_size, memory };
      if (mpi_world_rank == 1)
        return ExplicitContinuousWorldPart{ 10, 3, 9, mpi_world_rank, mpi_world_size, memory };
      ASSERT_EQ(mpi_world_rank, 2);
      return ExplicitContinuousWorldPart{ 10, 9, 10, mpi_world_rank, mpi_world_size, memory };
    }
  }();

  ASSERT_EQ(part.n, 10);
  ASSERT_EQ(part.world_rank, mpi_world_rank);
  ASSERT_EQ(part.world_size, mpi_world_size);
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

  if (mpi_world_rank == 0) {
    ASSERT_EQ(part.begin, 0);
    ASSERT_EQ(part.end, 3);
  }
  if (mpi_world_rank == 1) {
    ASSERT_EQ(part.begin, 3);
    ASSERT_EQ(part.end, 9);
  }
  if (mpi_world_rank == 2) {
    ASSERT_EQ(part.begin, 9);
    ASSERT_EQ(part.end, 10);
  }
}

i32
main(i32 argc, char** argv)
{
  static_assert(PartConcept<ExplicitContinuousPart>);
  static_assert(WorldPartConcept<CyclicPart>);
  static_assert(WorldPartConcept<BlockCyclicPart>);
  static_assert(WorldPartConcept<TrivialSlicePart>);
  static_assert(WorldPartConcept<BalancedSlicePart>);
  static_assert(WorldPartConcept<ExplicitContinuousWorldPart>);
  static_assert(WorldPartConcept<ExplicitSortedContinuousWorldPart>);

  constexpr i32 npc = 1;
  constexpr i32 npv[1]{ 3 };
  mpi_sub_process(argc, argv, npc, npv);

  KASPAN_DEFAULT_INIT();

  test_explicit_continuous_world_part<false>();
  test_explicit_continuous_world_part<true>();
}
