#pragma once

#include <ispan/util.hpp>

#include <cassert>
#include <cstdio>
#include <iostream>

inline void
print_time_result(kaspan::index_t run_times, double* avg_time)
{
  if (run_times > 0) {
    for (kaspan::index_t i = 0; i < 15; ++i) avg_time[i] = avg_time[i] / run_times * 1000;
    // printf("\nAverage Time Consumption for Running %d Times (ms)\n", run_times);
    // printf("Trim, %.3lf\n", avg_time[0]);
    // printf("Elephant SCC, %.3lf\n", avg_time[1]);
    // printf("Mice SCC, %.3lf\n", avg_time[2]);
    // printf("Total time, %.3lf\n", avg_time[3]);
    // printf("\n------Details------\n");
    // printf("Trim size_1, %.3lf\n", avg_time[4]);
    // printf("Pivot selection, %.3lf\n", avg_time[6]);
    // printf("FW BFS, %.3lf\n", avg_time[7]);
    // printf("BW BFS, %.3lf\n", avg_time[8]);
    // printf("Trim size_2, %.3lf\n", avg_time[5]);
    // printf("Trim size_3, %.3lf\n", avg_time[13]);
    // printf("Wcc, %.3lf\n", avg_time[9]);
    // printf("Mice fw-bw, %.3lf\n", avg_time[10]);
  }
}
