#include "catch.h"
#include "dependent.h"

#include <set>
#include <iostream>
#include <scoped_allocator>

namespace {

TEST_CASE("size_type", "[dependent_lib]") {
  using namespace dependent_lib::detail;

  static_assert(std::is_same<size_type_t<uint8_t>, int8_t>::value, "");
  static_assert(std::is_same<size_type_t<uint16_t>, int16_t>::value, "");
  static_assert(std::is_same<size_type_t<uint32_t>, int32_t>::value, "");
  static_assert(std::is_same<size_type_t<uint64_t>, int64_t>::value, "");

  struct big {
      uint64_t one, two;
  };
  static_assert(std::is_same<size_type_t<big>, int64_t>::value, "");
}

TEST_CASE("smoking_test", "[dependent_lib]") {
  short arr[] = {1, 2, 6, 2, 6};

  // init 1 vector
  {
    std::allocator<short> a{};
    dependent_lib::vector<short, std::allocator<short>> dv(std::begin(arr),
                                                           std::end(arr), a);
    REQUIRE(dv.size() == 5);
    dv.destroy(a);
  }
  {
    using allocator = std::scoped_allocator_adaptor<std::allocator<short>>;
    using vec_t = dependent_lib::vector<short, allocator>;
    std::set<vec_t, std::less<>, allocator> s;
    static_assert(std::uses_allocator<vec_t, allocator>::value, "");

    s.emplace(std::begin(arr), std::end(arr));
  }
}

// TODO: tests

}  // namespace
