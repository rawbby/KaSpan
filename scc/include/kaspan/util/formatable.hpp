#pragma once

#include <format>
#include <type_traits>

namespace kaspan {

namespace internal {
template<typename T>
concept Formattable = requires { std::formatter<std::remove_cvref_t<T>>{}; };
}

template<typename... Args>
concept formattable_concept = (internal::Formattable<Args> and ...);

} // namespace kaspan
