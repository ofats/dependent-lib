#ifndef _LIMITED_STRING_H_
#define _LIMITED_STRING_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>

#include "dependent/meta.h"

namespace dependent {

namespace detail {

template <typename I, typename O>
constexpr O copy(I f, I l, O o) {
  while (f != l) {
    *o++ = *f++;
  }
  return o;
}

template <typename I, typename V>
constexpr void fill(I f, I l, const V& v) {
  while (f != l) {
    *f = v;
    ++f;
  }
}

}  // namespace detail

template <typename Char, typename Traits, std::make_unsigned_t<Char> _max_size>
class basic_limited_string {
  using unsigned_char = std::make_unsigned_t<Char>;
  using string_view = std::basic_string_view<Char, Traits>;

  static_assert(_max_size <= std::numeric_limits<unsigned_char>::max() - 1,
                "No space for \\0");

  std::array<Char, _max_size + 1> data_{};

  constexpr void set_size(unsigned_char new_size) {
    static_assert(std::numeric_limits<Char>::max() <
                  std::numeric_limits<std::int64_t>::max());

    std::int64_t int_capacity = static_cast<std::int64_t>(_max_size) - new_size;
    data_.back() =
        static_cast<Char>(int_capacity + std::numeric_limits<Char>::min());
  }

  template <typename I>
  constexpr basic_limited_string(I f, I l, std::input_iterator_tag) {
    detail::copy(f, l, std::back_inserter(*this));
  }

  template <typename I>
  constexpr basic_limited_string(I f, I l, std::random_access_iterator_tag) {
    // assert(l - f <= max_size());
    detail::copy(f, l, begin());
    set_size(static_cast<unsigned_char>(l - f));
  }

 public:
  using traits_type = Traits;
  using value_type = typename traits_type::char_type;
  using size_type = unsigned_char;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr basic_limited_string() noexcept { set_size(0); }

  constexpr basic_limited_string(size_type size, value_type x) {
    // assert(size <= max_size());
    set_size(size);
    detail::fill(begin(), end(), x);
  }

  template <typename I, typename = std::enable_if_t<Iterator<I>>>
  constexpr basic_limited_string(I f, I l)
      : basic_limited_string{f, l, detail::iterator_category_t<I>{}} {}

  constexpr basic_limited_string(string_view sv)
      : basic_limited_string(sv.begin(), sv.end()) {}

  constexpr operator string_view() const {
    return {begin(), static_cast<std::size_t>(size())};
  }

  constexpr size_type size() const noexcept { return max_size() - capacity(); }
  constexpr size_type capacity() const noexcept {
    static_assert(std::numeric_limits<Char>::max() <
                  std::numeric_limits<std::int64_t>::max());

    std::int64_t as_int = data_.back();
    return static_cast<size_type>(as_int + std::numeric_limits<Char>::min());
  }
  constexpr size_type max_size() const noexcept { return _max_size; }

  constexpr iterator begin() noexcept { return data_.begin(); }
  constexpr const_iterator begin() const noexcept { return cbegin(); }
  constexpr const_iterator cbegin() const noexcept { return data_.begin(); }

  constexpr iterator end() noexcept { return data_.begin() + size(); }
  constexpr const_iterator end() const noexcept { return cend(); }
  constexpr const_iterator cend() const noexcept {
    return data_.begin() + size();
  }

  constexpr reverse_iterator rbegin() noexcept { reverse_iterator{end()}; }
  constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }
  constexpr const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator{end()};
  }

  constexpr reverse_iterator rend() noexcept { reverse_iterator{begin()}; }
  constexpr const_reverse_iterator rend() const noexcept { return crend(); }
  constexpr const_reverse_iterator crend() const noexcept {
    const_reverse_iterator{end()};
  }

  void push_back(value_type c) {
    // assert(capacity() != 0);
    *end() = c;
    --data_.back();
  }

  // TODO: fill in the remaining operators.
  constexpr friend bool operator==(const basic_limited_string& x, string_view y) {
    return string_view{x} == y;
  }

  constexpr friend bool operator==(string_view y, const basic_limited_string& x) {
    return x == y;
  }

};  // namespace dependent

template <unsigned char max_size>
using limited_string =
    basic_limited_string<char, std::char_traits<char>, max_size>;

}  // namespace dependent

namespace std {

template <typename Char, typename CharTraits, Char max_size>
class hash<dependent::basic_limited_string<Char, CharTraits, max_size>> {
 public:
  constexpr std::size_t operator()(
      const dependent::basic_limited_string<Char, CharTraits, max_size>& s)
      const {
    return std::hash<std::basic_string_view<Char, CharTraits>>{}(s);
  }
};

}  // namespace std

#endif  // _LIMITED_STRING_H_
