#include "dependent/utils/stats_allocator.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "catch/catch.h"

#include "dependent/utils/stats_containers.h"

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
    dependent::stats_containers<tag>::vector<int> v(100);
    REQUIRE(stats::total_allocated_size() == v.capacity() * sizeof(int));
    REQUIRE(stats::allocated_size_for_t<int>() == v.capacity() * sizeof(int));
  }

  REQUIRE(stats::total_allocated_size() == 0u);
}

TEST_CASE("std_unordered_map_of_strings", "[stats_allocator, dependent_lib]") {
  struct tag {};
  using stats = dependent::area_stats<tag>;
  using containers = dependent::stats_containers<tag>;

  containers::unordered_map<containers::string, containers::string> m;

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
