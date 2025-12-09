#pragma once

#include <format>

template<typename T>
concept FormattableConcept = requires { std::formatter<std::remove_cvref_t<T>>{}; };
