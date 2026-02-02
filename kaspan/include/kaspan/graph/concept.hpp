#pragma once

#include <kaspan/graph/base.hpp>

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
       { T::continuous } -> std::convertible_to<bool>;
       { T::ordered } -> std::convertible_to<bool>;
       { t.world_size() } -> std::same_as<i32>;
       { t.world_rank() } -> std::same_as<i32>;
       t.each_k([](vertex_t) {});
       t.each_ku([](vertex_t, vertex_t) {});
       t.each_u([](vertex_t) {});
     })

  && (!T::ordered || T::continuous)

  && (!T::continuous || requires(T t) {
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
       { T::continuous } -> std::convertible_to<bool>;
       { T::ordered } -> std::convertible_to<bool>;
       { t.world_size() } -> std::same_as<i32>;
       { t.world_rank() } -> std::same_as<i32>;
       t.each_k([](vertex_t) {});
       t.each_ku([](vertex_t, vertex_t) {});
       t.each_u([](vertex_t) {});
     })

  && (!T::ordered || T::continuous)

  && (!T::continuous ||
      requires(T t) {
        { t.begin() } -> std::same_as<vertex_t>;
        { t.end() } -> std::same_as<vertex_t>;
      })

  && (requires(T t) {
       { t.world_rank_of(std::declval<vertex_t>()) } -> std::same_as<i32>;
       { t.world_part_of(std::declval<i32>()) } -> part_view_concept;
       { t.view() } -> part_view_concept;
     });

template<class T>
concept graph_part_view_concept =
  requires(T t, vertex_t k) {
    t.each_k([](vertex_t) {});
    t.each_ku([](vertex_t, vertex_t) {});
    t.each_u([](vertex_t) {});
    t.each_v(k, [](vertex_t) {});
    t.each_uv(k, [](vertex_t, vertex_t) {});
    t.each_kv([](vertex_t, vertex_t) {});
    t.each_kuv([](vertex_t, vertex_t, vertex_t) {});
  };

template<class T>
concept graph_part_concept =
  requires(T t) {
    { t.view() } -> graph_part_view_concept;
  };

/**
 * TODO: Verify that all implementations of graph and part interfaces fully support the each interface.
 */

}
