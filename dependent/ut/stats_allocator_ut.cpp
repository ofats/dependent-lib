#include "dependent/stats_allocator.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "catch/catch.h"

namespace {

template <template <typename...> class A, typename... TailArgs>
void type_checks() {
  using traits = std::allocator_traits<A<int, TailArgs...>>;

  static_assert(std::is_same_v<int, typename traits::value_type>);
  static_assert(std::is_same_v<int*, typename traits::pointer>);
  static_assert(std::is_same_v<const int*, typename traits::const_pointer>);

  static_assert(std::is_same_v<void*, typename traits::void_pointer>);
  static_assert(
      std::is_same_v<const void*, typename traits::const_void_pointer>);

  static_assert(
      std::is_same_v<std::ptrdiff_t, typename traits::difference_type>);
  static_assert(std::is_same_v<std::size_t, typename traits::size_type>);

  static_assert(!traits::propagate_on_container_copy_assignment::value);
  static_assert(traits::propagate_on_container_move_assignment::value);
  static_assert(!traits::propagate_on_container_swap::value);
  static_assert(traits::is_always_equal::value);

  static_assert(std::is_same_v<A<char, TailArgs...>,
                               typename traits::template rebind_alloc<char>>);
}

// Dealing with libstdc++ lack of support for hashing strings with a non-default
// allocator.
struct string_hash {
  template <typename Char, typename Traits, typename Alloc>
  std::size_t operator()(
      const std::basic_string<Char, Traits, Alloc>& str) const {
    using view = std::basic_string_view<Char, Traits>;
    return std::hash<view>{}(str);
  }
};

template <typename Tag>
struct stats_containers {
  template <typename T>
  using vector = std::vector<T, dependent::stats_allocator<T, Tag>>;

  template <typename Key, typename T, typename Hash = string_hash,
            typename KeyEqual = std::equal_to<Key>>
  using unordered_map = std::unordered_map<
      Key, T, Hash, KeyEqual,
      dependent::stats_allocator<std::pair<const Key, T>, Tag>>;

  template <typename Char, typename CharTraits = std::char_traits<Char>>
  using basic_string = std::basic_string<Char, CharTraits,
                                         dependent::stats_allocator<Char, Tag>>;

  using string = basic_string<char>;
};

}  // namespace

TEST_CASE("type_checks", "[stats_allocator, dependent_lib]") {
  // Sanity check
  type_checks<std::allocator>();

  // Actual test
  struct tag {};

  type_checks<dependent::stats_allocator, tag>();

  using alloc = dependent::stats_allocator<int, tag>;
  static_assert(std::is_trivially_default_constructible_v<alloc>);
  static_assert(std::is_trivially_copyable_v<alloc>);
  static_assert(std::is_trivially_destructible_v<alloc>);
}

TEST_CASE("std_vector_reserve", "[stats_allocator, dependent_lib]") {
  struct tag {};
  using stats = dependent::area_stats<tag>;

  REQUIRE(stats::total_allocated_size() == 0u);

  {
    stats_containers<tag>::vector<int> v(100);
    REQUIRE(stats::total_allocated_size() == v.capacity() * sizeof(int));
    REQUIRE(stats::allocated_size_for_t<int>() == v.capacity() * sizeof(int));
  }

  REQUIRE(stats::total_allocated_size() == 0u);
}

TEST_CASE("std_unordered_map_of_strings", "[stats_allocator, dependent_lib]") {
  struct tag {};
  using stats = dependent::area_stats<tag>;

  stats_containers<tag>::unordered_map<stats_containers<tag>::string,
                                       stats_containers<tag>::string>
      m;

  REQUIRE(stats::total_allocated_size() == 0);

  m["short"] = "short";  // short string

  REQUIRE(stats::total_allocated_size() > 0);         // unordered map staff
  REQUIRE(stats::allocated_size_for_t<char>() == 0);  // strings are short;

  auto& entry = m["short"];
  entry = "loooooooooooooooooooooooooooooooooooooong string";

  // I think that +1 has to do with '\0' - since you can't override it.
  // reading libc++ code is quite hard in this area, so I'm not sure.
  REQUIRE(stats::allocated_size_for_t<char>() == entry.capacity() + 1);
}
