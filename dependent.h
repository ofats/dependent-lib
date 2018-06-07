#ifndef _DEPENDENT_LIB_H_
#define _DEPENDENT_LIB_H_

#include <cstdint>
#include <tuple>
#include <type_traits>

namespace dependent_lib {

namespace detail {

using size_types = std::tuple<int8_t, int16_t, int32_t, int64_t>;
using bigest_size_type =
    std::tuple_element_t<std::tuple_size<size_types>::value - 1, size_types>;
static_assert(sizeof(bigest_size_type) >= sizeof(size_t), "");

template <typename T> constexpr size_t select_size_type() {
  if (sizeof(T) <= sizeof(int8_t))
    return 0;
  if (sizeof(T) <= sizeof(int16_t))
    return 1;
  if (sizeof(T) <= sizeof(int32_t))
    return 2;
  static_assert(std::tuple_size<size_types>::value == 4, "");
  return 3;
}

template <typename T>
using size_type_t = std::tuple_element_t<select_size_type<T>(), size_types>;

} // namespace detail

} // namespace dependent_lib

#endif // _DEPENDENT_LIB_H_
