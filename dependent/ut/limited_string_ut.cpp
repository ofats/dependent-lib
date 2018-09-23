#include "dependent/limited_string.h"

#include <type_traits>

namespace {

// Since the type is completly constexpr, all the tests are compile time
// asserts.
namespace types {

using str = dependent::limited_string<16>;

static_assert(std::is_same_v<str::traits_type, std::char_traits<char>>);
static_assert(std::is_same_v<str::value_type, char>);
static_assert(std::is_same_v<str::size_type, unsigned char>);
static_assert(std::is_same_v<str::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<str::reference, char&>);
static_assert(std::is_same_v<str::pointer, char*>);
static_assert(std::is_same_v<str::const_reference, const char&>);
static_assert(std::is_same_v<str::const_pointer, const char*>);
static_assert(std::is_same_v<str::iterator, char*>);
static_assert(std::is_same_v<str::const_iterator, const char*>);
static_assert(
    std::is_same_v<str::reverse_iterator, std::reverse_iterator<char*>>);
static_assert(std::is_same_v<str::const_reverse_iterator,
                             std::reverse_iterator<const char*>>);

}  // namespace types

namespace constructor {

using str = dependent::limited_string<16>;

constexpr str x1{};
static_assert(x1.capacity() == x1.max_size());
static_assert(x1.size() == 0);
static_assert(x1 == "");

constexpr str x2{"a"};
static_assert(x2.size() == 1);
static_assert(x2 == "a");

constexpr str x3{16, 'a'};
static_assert(x3.size() == 16);

}  // namespace constructor

namespace big_size {

constexpr unsigned char longest_size =
    std::numeric_limits<unsigned char>::max() - 1;

using str = dependent::limited_string<longest_size>;
constexpr str x(longest_size, 'a');
static_assert(x.size() == longest_size);
static_assert(*x.rbegin() == 'a');

}  // namespace big_size

}  // namespace
