#pragma once

#include <debug/Statistic.hpp>
#include <pp/PP.hpp>

#include <kamping/measurements/timer.hpp>

#include <string>
#include <utility>
#include <vector>

#ifdef KASPAN_TIME
#define KASPAN_TIME 1
#else
#define KASPAN_TIME KASPAN_STATISTIC
#endif

#define KASPAN_TIME_START(KEY) IF(KASPAN_TIME, kamping::measurements::timer().start((KEY));)
#define KASPAN_TIME_STOP() IF(KASPAN_TIME, kamping::measurements::timer().stop_and_append();)
#define KASPAN_TIME_STOP_SUM() IF(KASPAN_TIME, kamping::measurements::timer().stop_and_add();)
