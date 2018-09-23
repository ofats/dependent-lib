#ifndef _LIMITED_STRING_H_
#define _LIMITED_STRING_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <string_view>
#include <utility>

#include "dependent/meta.h"

namespace dependent_lib {

template <typename Char, typename Traits, Char max_size>
class basic_limited_string {
  std::array<Char, max_size + 1> data_;

  template <typename I>
  void range_construct(I f, I l, std::input_iterator_tag) {
    std::copy(f, l, std::back_inserter(*this));
  }

  template <typename I>
  void range_construct(I f, I l, std::random_access_iterator_tag) {
    assert(l - f <= max_size);
    std::copy(f, l, begin());
  }

  using string_view = std::basic_string_view<Char, Traits>;

 public:
  using traits_type = Traits;
  using value_type = Char;
  using size_type = value_type;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  constexpr basic_limited_string() {
    data_.back() = max_size;
  }

  constexpr basic_limited_string(size_type sz, value_type val) {
    assert(sz <= max_size);
    data_.back() = max_size - sz;
    std::fill(begin(), end(), val);
  }

  template <typename I, typename = std::enable_if_t<Iterator<I>>>
  constexpr basic_limited_string(I f, I l) {
    range_construct(f, l, typename std::iterator_traits<I>::iterator_category{});
  }

  constexpr basic_limited_string(std::basic_string_view<Char, Traits> sv)
      : basic_limited_string(sv.begin(), sv.end()) {}

  constexpr operator std::basic_string_view<Char, Traits>() const {
    return {begin(), static_cast<std::size_t>(size())};
  }

  friend constexpr bool operator==(const basic_limited_string& x,
                                   const basic_limited_string& y) noexcept {
    return string_view{x} == string_view{y};
  }

  constexpr size_type size() const noexcept {
    return data_.size() - capacity();
  }

  constexpr size_type capacity() const noexcept {
      return data_.back();
  }

  constexpr iterator begin() noexcept { return data_.begin(); }
  constexpr const_iterator begin() const noexcept { return data_.begin(); }
  constexpr iterator end() noexcept { return data_.begin() + size(); }
  constexpr const_iterator end() const noexcept {
    return data_.begin() + size();
  }

  void push_back(value_type c) {
    assert(capacity() != 0);
    *end() = c;
    --data_.back();
  }
};

template <char max_size>
using limited_string =
    basic_limited_string<char, std::char_traits<char>, max_size>;

}  // dependent_lib

namespace std {

template <typename Char, typename CharTraits, Char max_size>
class hash<dependent_lib::basic_limited_string<Char, CharTraits, max_size>> {
  using sv = std::basic_string_view<Char, CharTraits>;

 public:
  constexpr std::size_t operator()(sv s) const { return std::hash<sv>{}(s); }
};

}  // namespace std


#endif  // _LIMITED_STRING_H_
