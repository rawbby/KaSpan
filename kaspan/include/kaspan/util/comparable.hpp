#pragma once

#include <kaspan/util/arithmetic.hpp>

namespace kaspan {

template<typename T, typename U>
concept comparable_concept = arithmetic_concept<T> && arithmetic_concept<U>;

} // namespace kaspan
