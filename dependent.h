#ifndef _DEPENDENT_LIB_H_
#define _DEPENDENT_LIB_H_

#include <cstdint>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <cassert>

namespace dependent_lib {

namespace detail {

using size_types = std::tuple<int8_t, int16_t, int32_t, int64_t>;
using bigest_size_type =
    std::tuple_element_t<std::tuple_size<size_types>::value - 1, size_types>;
static_assert(sizeof(bigest_size_type) >= sizeof(size_t), "");

template <typename T>
constexpr std::size_t select_size_type() {
  if (sizeof(T) <= sizeof(int8_t)) return 0;
  if (sizeof(T) <= sizeof(int16_t)) return 1;
  if (sizeof(T) <= sizeof(int32_t)) return 2;
  static_assert(std::tuple_size<size_types>::value == 4, "");
  return 3;
}

template <typename T>
using size_type_t = std::tuple_element_t<select_size_type<T>(), size_types>;

template <typename T>
std::ptrdiff_t required_space_in_types(std::ptrdiff_t size) {
  if (size < std::numeric_limits<size_type_t<T>>::max()) {
    return size + 1;
  }
  std::terminate();
}

template <typename I, typename Alloc>
void destroy(I f, I l, Alloc a) {
  using ri = std::reverse_iterator<I>;
  for (ri it{l}; it != ri{f}; ++it)
    std::allocator_traits<Alloc>::destroy(a, it.base());
}

}  // namespace detail

template <typename T, typename Alloc>
class vector {
 public:
  using allocator_type = Alloc;
  using alloc_traits = std::allocator_traits<Alloc>;
  using pointer = typename alloc_traits::pointer;
  using size_type = std::ptrdiff_t;
  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<T*>;

 private:
  std::ptrdiff_t size_offset() const noexcept { return 1; }

  detail::size_type_t<T>& small_size() {
    return *reinterpret_cast<detail::size_type_t<T>*>(data_);
  }

  const detail::size_type_t<T>& small_size() const {
    return *reinterpret_cast<const detail::size_type_t<T>*>(data_);
  }

  pointer data_;

 public:
  template <typename I>
  vector(I f, I l, allocator_type a) {
    auto dist = std::distance(f, l);
    auto space_in_types = detail::required_space_in_types<T>(dist);
    auto allocated_memory = space_in_types * sizeof(T);
    data_ = alloc_traits::allocate(a, allocated_memory);
    small_size() = dist;

    T* t_begin = &*data_ + (space_in_types - dist);
    T* t_cur = t_begin;
    try {
      for (; f != l; ++f, ++t_cur) alloc_traits::construct(a, t_cur, *f);
    } catch (...) {
      detail::destroy(t_begin, t_cur, a);
      alloc_traits::deallocate(a, data_, allocated_memory);
    }
  }

  ~vector() {

  }

  void destroy(allocator_type a) {
    detail::destroy(begin(), end(), a);
    alloc_traits::deallocate(a, data_, (size() + size_offset()) * sizeof(T));
    data_ = nullptr;
  }

  friend bool operator<(const vector& x, const vector& y) {
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }

  iterator begin() { return &*data_ + size_offset(); }

  iterator end() { return begin() + size(); }

  const_iterator begin() const { return &*data_ + size_offset(); }

  const_iterator end() const { return begin() + size(); }

  reverse_iterator rbegin() { return end(); }

  reverse_iterator rend() { return begin(); }

  size_type size() const noexcept { return small_size(); }

};

}  // namespace dependent_lib

#endif  // _DEPENDENT_LIB_H_
