#include "catch.h"
#include "dependent.h"

#include <iostream>

namespace {

TEST_CASE("size_type", "[meta]") {
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

}  // namespace
