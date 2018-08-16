#include <cstdint>
#include <iterator>

namespace future_std_stubs {

namespace detail {

// To avoid accidental conversions between span and string
template <typename T, int tag>
class span_impl {
  T* f_ = nullptr;
  T* l_ = nullptr;

 public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;

  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  span_impl() = default;

  span_impl(pointer f, pointer l) : f_(f), l_(l) {}

  std::size_t size() const { return l_ - f_; }

  iterator begin() { return f_; }
  const_iterator begin() const { return f_; }
  const_iterator cbegin() const { return f_; }

  iterator end() { return l_; }
  const_iterator end() const { return l_; }
  const_iterator cend() const { return l_; }

  reverse_iterator rbegin() { return reverse_iterator{l_}; }
  const_reverse_iterator rbegin() const { return const_reverse_iterator{l_}; }
  const_reverse_iterator crbegin() const { return const_reverse_iterator{l_}; }

  reverse_iterator rend() { return reverse_iterator{l_}; }
  const_reverse_iterator rend() const { return const_reverse_iterator{l_}; }
  const_reverse_iterator crend() const { return const_reverse_iterator{l_}; }
};
}  // namespace detail

template <typename T>
using span = detail::span_impl<T, 0>;

template <typename Char, typename...>
using basic_string_view = detail::span_impl<Char, 1>;

using string_view = basic_string_view<char>;

}  // namespace future_std_stubs