#pragma once

#include <debug/Debug.hpp>
#include <pp/PP.hpp>
#include <util/ScopeGuard.hpp>

#include <string>
#include <utility>
#include <vector>

#ifdef KASPAN_STATISTIC
#define KASPAN_STATISTIC 1
#else
#define KASPAN_STATISTIC KASPAN_DEBUG
#endif

IF(KASPAN_STATISTIC, inline auto g_kaspan_statistic = (std::vector<std::pair<std::string, std::string>>{});)
#define KASPAN_STATISTIC_PUSH(KEY, VALUE) IF(KASPAN_STATISTIC, g_kaspan_statistic.emplace_back((KEY), (VALUE));)
