#include "dependent/future_std_stubs.h"

#include <algorithm>
#include "catch/catch.h"

template <typename T>
void stub() {}

template <typename R>
void range_types_test() {
  stub<typename R::value_type>();

  stub<typename R::reference>();
  stub<typename R::const_reference>();
  stub<typename R::pointer>();
  stub<typename R::const_pointer>();

  stub<typename R::size_type>();
  stub<typename R::difference_type>();

  stub<typename R::iterator>();
  stub<typename R::const_iterator>();
  stub<typename R::reverse_iterator>();
  stub<typename R::const_reverse_iterator>();
}

template <typename T>
void semi_regular() {
  T x1;
  T x2{x1};
  x1 = x2;
}

template <typename R>
void iterable(R r) {
  const R& cr = r;

  std::for_each(r.begin(), r.end(), [](const auto&) {});
  std::for_each(r.cbegin(), r.cend(), [](const auto&) {});
  std::for_each(cr.begin(), cr.end(), [](const auto&) {});
  std::for_each(cr.cbegin(), cr.cend(), [](const auto&) {});

  std::for_each(r.rbegin(), r.rend(), [](const auto&) {});
  std::for_each(r.crbegin(), r.crend(), [](const auto&) {});
  std::for_each(cr.rbegin(), cr.rend(), [](const auto&) {});
  std::for_each(cr.crbegin(), cr.crend(), [](const auto&) {});
}

using int_span = future_std_stubs::span<int>;
using future_std_stubs::string_view;

TEST_CASE("range types", "[future_std_stubs]") {
  range_types_test<int_span>();
  range_types_test<string_view>();
}

TEST_CASE("regular", "[future_std_stubs]") {
  semi_regular<int_span>();
  semi_regular<string_view>();
}

TEST_CASE("iteration", "[future_std_stubs]") {
  iterable(int_span());
  iterable(string_view());
}

TEST_CASE("size", "[future_std_stubs]") {
  int_span x;
  x.size();
  string_view y;
  y.size();
}

TEST_CASE("conversion sanity check", "[future_std_stubs]") {
  using char_span = future_std_stubs::span<char>;

  static_assert(!std::is_convertible<char_span, string_view>::value, "");
  static_assert(!std::is_convertible<string_view, char_span>::value, "");
}
