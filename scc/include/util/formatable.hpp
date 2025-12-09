#pragma once

#include <format>
#include <type_traits>

namespace internal {
template<typename T>
concept Formattable = requires { std::formatter<std::remove_cvref_t<T>>{}; };
}

template<typename... Args>
concept FormattableConcept = (internal::Formattable<Args> and ...);
