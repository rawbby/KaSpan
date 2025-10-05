#include <graph/Part.hpp>
#include <test/Assert.hpp>
#include <test/SubProcess.hpp>
#include <util/ScopeGuard.hpp>

template<bool sorted_variant>
void
test_explicit_continuous_world_part(int rank, int size)
{
  using Part = std::conditional_t<
    sorted_variant,
    ExplicitSortedContinuousWorldPart,
    ExplicitContinuousWorldPart>;

  auto result = [&] {
    if (rank == 0)
      return Part::create(10, 0, 3, rank, size);
    if (rank == 1)
      return Part::create(10, 3, 9, rank, size);
    ASSERT_EQ(rank, 2);
    return Part::create(10, 9, 10, rank, size);
  }();

  ASSERT(result.has_value());
  auto const part = std::move(result.value());

  ASSERT_EQ(part.n, 10);
  ASSERT_EQ(part.world_rank, rank);
  ASSERT_EQ(part.world_size, size);
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

  if (rank == 0) {
    ASSERT_EQ(part.begin, 0);
    ASSERT_EQ(part.end, 3);
  }
  if (rank == 1) {
    ASSERT_EQ(part.begin, 3);
    ASSERT_EQ(part.end, 9);
  }
  if (rank == 2) {
    ASSERT_EQ(part.begin, 9);
    ASSERT_EQ(part.end, 10);
  }
}

int
main(int argc, char** argv)
{
  static_assert(PartConcept<ExplicitContinuousPart>);
  static_assert(WorldPartConcept<CyclicPart>);
  static_assert(WorldPartConcept<BlockCyclicPart>);
  static_assert(WorldPartConcept<TrivialSlicePart>);
  static_assert(WorldPartConcept<BalancedSlicePart>);
  static_assert(WorldPartConcept<ExplicitContinuousWorldPart>);
  static_assert(WorldPartConcept<ExplicitSortedContinuousWorldPart>);

  constexpr int npc = 1;
  constexpr int npv[1]{ 3 };
  mpi_sub_process(argc, argv, npc, npv);

  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());

  int rank = 0;
  int size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  test_explicit_continuous_world_part<false>(rank, size);
  test_explicit_continuous_world_part<true>(rank, size);
}
