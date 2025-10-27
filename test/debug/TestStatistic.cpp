#define KASPAN_STATISTIC 1

#include <debug/Statistic.hpp>
#include <test/SubProcess.hpp>

#include <iostream>

int
main(int argc, char** argv)
{
  constexpr int npc      = 1;
  constexpr int npv[npc] = { 1 };
  mpi_sub_process(argc, argv, npc, npv);
  MPI_Init(nullptr, nullptr);
  SCOPE_GUARD(MPI_Finalize());
  {
    KASPAN_STATISTIC_SCOPE("outer");
    {
      KASPAN_STATISTIC_SCOPE("inner");
      kaspan_statistic_add("x", 1);
      kaspan_statistic_add("y", 1);
      kaspan_statistic_push("z_push");
      kaspan_statistic_add("z", 1);
      kaspan_statistic_pop();
      KASPAN_STATISTIC_SCOPE("minimal");
    }
    kaspan_statistic_add("f", 13);
  }
  {
    KASPAN_STATISTIC_SCOPE("second");
    kaspan_statistic_add("x", 1);
    {
      KASPAN_STATISTIC_SCOPE("second");
      kaspan_statistic_add("y", 1);
    }
    kaspan_statistic_add("z", 1);
  }
  constexpr auto expected_json = R"({"outer":{"duration":1,"inner":{"duration":1,"x":1,"y":1,"z_push":{"duration":1,"z":1},"minimal":{"duration":1}},"f":13},"second":{"duration":1,"x":1,"second":{"duration":1,"y":1},"z":1}})";
  // normalize duration as these are expected to be different every run
  for (auto& node : g_kaspan_statistic_nodes) {
    // it should never take more than ten milliseconds
    // to write timings even in debug
    ASSERT_LT(node.duration, 10'000'000);
    ASSERT_NE(node.duration, 0);
    node.duration = 1;
  }
  std::stringstream jsonBuilder;
  kaspan_statistic_write_json(jsonBuilder);
  auto const actual_json = jsonBuilder.str();
  std::cout << "actual:   " << actual_json << std::endl;
  std::cout << "expected: " << expected_json << std::endl;
  ASSERT_EQ(actual_json, expected_json);
}
