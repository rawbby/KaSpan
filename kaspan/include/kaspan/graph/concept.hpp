#pragma once

#include <kaspan/scc/base.hpp>

#include <concepts>
#include <type_traits>

namespace kaspan {

template<class T>
concept part_view_concept =

  std::is_move_constructible_v<T> && std::is_move_assignable_v<T>

  && std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>

  && (requires(T t, vertex_t i) {
       { t.n() } -> std::same_as<vertex_t>;
       { t.local_n() } -> std::same_as<vertex_t>;
       { t.to_local(i) } -> std::same_as<vertex_t>;
       { t.to_global(i) } -> std::same_as<vertex_t>;
       { t.has_local(i) } -> std::same_as<bool>;
       { t.continuous() } -> std::same_as<bool>;
       { t.ordered() } -> std::same_as<bool>;
       { t.world_size() } -> std::same_as<i32>;
       { t.world_rank() } -> std::same_as<i32>;
     })

  && (!T::ordered() || T::continuous())

  && (!T::continuous() || requires(T t) {
       { t.begin() } -> std::same_as<vertex_t>;
       { t.end() } -> std::same_as<vertex_t>;
     });

template<class T>
concept part_concept =

  std::is_move_constructible_v<T> && std::is_move_assignable_v<T>

  && (requires(T t, vertex_t i) {
       { t.n() } -> std::same_as<vertex_t>;
       { t.local_n() } -> std::same_as<vertex_t>;
       { t.to_local(i) } -> std::same_as<vertex_t>;
       { t.to_global(i) } -> std::same_as<vertex_t>;
       { t.has_local(i) } -> std::same_as<bool>;
       { t.continuous() } -> std::same_as<bool>;
       { t.ordered() } -> std::same_as<bool>;
       { t.world_size() } -> std::same_as<i32>;
       { t.world_rank() } -> std::same_as<i32>;
     })

  && (!T::ordered() || T::continuous())

  && (!T::continuous() ||
      requires(T t) {
        { t.begin() } -> std::same_as<vertex_t>;
        { t.end() } -> std::same_as<vertex_t>;
      })

  && (requires(T t, vertex_t i, i32 r) {
       { t.world_rank_of(i) } -> std::same_as<i32>;
       { t.world_part_of(r) } -> part_view_concept;
       { t.view() } -> part_view_concept;
     });

}
