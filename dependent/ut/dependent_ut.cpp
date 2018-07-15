#include "catch/catch.h"
#include "dependent/dense_allocator.h"
#include "dependent/dependent.h"

#include <map>
#include <scoped_allocator>
#include <set>

namespace {

constexpr short arr1[] = {1, 2, 6, 2, 6};

TEST_CASE("size_type", "[dependent_lib]") {
  using namespace dependent_lib::detail;

  static_assert(std::is_same<size_type_t<int8_t>, uint8_t>::value, "");
  static_assert(std::is_same<size_type_t<int16_t>, uint16_t>::value, "");
  static_assert(std::is_same<size_type_t<int32_t>, uint32_t>::value, "");
  static_assert(std::is_same<size_type_t<int64_t>, uint64_t>::value, "");

  struct big {
    uint64_t one, two;
  };
  static_assert(std::is_same<size_type_t<big>, uint64_t>::value, "");
}

TEST_CASE("required_size_in_types", "[dependent_lib]") {
  using namespace dependent_lib::detail;
  constexpr int64_t uint32_max = std::numeric_limits<uint32_t>::max();

  // fits
  static_assert(required_space_in_types<char>(9) == 10, "");
  static_assert(required_space_in_types<char>(99) == 100, "");
  static_assert(required_space_in_types<int32_t>(3500) == 3501, "");
  static_assert(required_space_in_types<int32_t>(uint32_max - 1) == uint32_max,
                "");

  // doesn't fit
  static_assert(required_space_in_types<char>(128) == 129, "");
  static_assert(required_space_in_types<char>(256) == 257 + 16, "");
  static_assert(required_space_in_types<char>(3500) == 3501 + 16, "");
  static_assert(required_space_in_types<int32_t>(uint32_max) == uint32_max + 5,
                "");
}

TEST_CASE("concepts", "[dependent_lib]") {
  using namespace dependent_lib;

  static_assert(!ForwardIterator<int>, "");
  static_assert(ForwardIterator<int*>, "");
  static_assert(ForwardIterator<std::vector<int>::iterator>, "");
  static_assert(ForwardIterator<std::vector<int>::const_iterator>, "");
  static_assert(ForwardIterator<std::map<int, int>::iterator>, "");

  static_assert(ForwardRange<std::vector<int>>, "");
  static_assert(!ForwardRange<int>, "");
  static_assert(ForwardRange<int(&)[3]>, "");
  static_assert(ForwardRange<decltype(arr1)>, "");
}

TEST_CASE("destroy", "[dependent_lib]") {
  std::allocator<short> a{};
  dependent_lib::vector<short, std::allocator<short>> dv(
      std::allocator_arg_t{}, a, std::begin(arr1), std::end(arr1));
  REQUIRE(dv.as_span().size() == 5);
  dv.destroy(a);
}

TEST_CASE("set_of_vector", "[dependent_lib]") {
  using vec_allocator = dependent_lib::allocator_adaptor<std::allocator<short>>;
  using vec_t = dependent_lib::vector<short, vec_allocator>;
  static_assert(dependent_lib::DependentType<vec_t, vec_allocator>, "");
  static_assert(std::uses_allocator<vec_t, vec_allocator>::value, "");

  using set_allocator = dependent_lib::allocator_adaptor<std::allocator<vec_t>>;
  static_assert(std::uses_allocator<vec_t, set_allocator>::value, "");

  std::set<vec_t, std::less<>, set_allocator> s;

  s.emplace(std::begin(arr1), std::end(arr1));
}

TEST_CASE("vector_of_vector", "[dependent_lib]") {
  using vec_allocator = dependent_lib::allocator_adaptor<std::allocator<short>>;
  using vec_t = dependent_lib::vector<short, vec_allocator>;
  static_assert(dependent_lib::DependentType<vec_t, vec_allocator>, "");
  static_assert(std::uses_allocator<vec_t, vec_allocator>::value, "");

  using outer_allocator =
      dependent_lib::allocator_adaptor<std::allocator<vec_t>>;
  static_assert(std::uses_allocator<vec_t, outer_allocator>::value, "");

  std::vector<vec_t, outer_allocator> v;

  v.emplace_back(std::begin(arr1), std::end(arr1));
}

TEST_CASE("big_vector_of_vector", "[dependent_lib]") {
  using vec_allocator = dependent_lib::allocator_adaptor<std::allocator<short>>;
  using vec_t = dependent_lib::vector<short, vec_allocator>;
  static_assert(dependent_lib::DependentType<vec_t, vec_allocator>, "");
  static_assert(std::uses_allocator<vec_t, vec_allocator>::value, "");

  using outer_allocator =
      dependent_lib::allocator_adaptor<std::allocator<vec_t>>;
  static_assert(std::uses_allocator<vec_t, outer_allocator>::value, "");

  std::vector<vec_t, outer_allocator> v;

  std::vector<short> input(1'000'000, 1);
  v.emplace_back(input);

  auto sp = v.back().as_span();
  REQUIRE(std::all_of(sp.begin(), sp.end(), [](auto v) { return v == 1; }));
}

TEST_CASE("map_of_vector", "[dependent_lib]") {
  using vec_allocator = dependent_lib::allocator_adaptor<std::allocator<int>>;
  using vec_t = dependent_lib::vector<int, vec_allocator>;
  using outer_allocator = dependent_lib::allocator_adaptor<
      std::allocator<std::pair<const int, vec_t>>>;

  std::map<int, vec_t, std::less<>, outer_allocator> mp;
  mp.emplace(3, arr1);
}

TEST_CASE("vector_with_dense_allocator", "[dependent_lib]") {
  using dense_allocators =
      dependent_lib::dense_allocators<std::allocator<int>, int>;
  using allocator_handle =
      dependent_lib::dense_allocator_handler<int, dense_allocators>;
  dense_allocators resourse(std::allocator<int>{});
  std::vector<int, allocator_handle> v(allocator_handle{&resourse});
  for (int i = 0; i < 10'000; ++i)
    v.insert(v.end(), std::begin(arr1), std::end(arr1));
}

TEST_CASE("vector_of_densly_allocated_strings",
          "[dependent_lib, dense_allocator]") {
  using dense_allocators =
      dependent_lib::dense_allocators<std::allocator<char>, char>;
  using allocator_handle =
      dependent_lib::dense_allocator_handler<char, dense_allocators>;
  using string_with_allocator =
      std::basic_string<char, std::char_traits<char>, allocator_handle>;
  using resulting_allocator =
      std::scoped_allocator_adaptor<std::allocator<string_with_allocator>,
                                    allocator_handle>;

  dense_allocators allocs(std::allocator<char>{});
  std::vector<string_with_allocator, resulting_allocator> v(
      {std::allocator<string_with_allocator>{}, allocator_handle(&allocs)});
  v.emplace_back("abc");
  v.emplace_back(50, 'a');
}

TEST_CASE("map_dependent_vectors", "[dependent_lib, dense_allocator]") {
  using backing_allocator = std::allocator<void>;
  using dense_allocators =
      dependent_lib::dense_allocators<backing_allocator, char,
                                      dependent_lib::unknown_type<48, 8>>;

  using raw_vec_t_handle =
      dependent_lib::dense_allocator_handler<char, dense_allocators>;
  using vec_t_handle = dependent_lib::allocator_adaptor<raw_vec_t_handle>;
  using vec_t = dependent_lib::vector<char, vec_t_handle>;

  using raw_map_handle = dependent_lib::dense_allocator_handler<
          std::pair<const vec_t, vec_t>, dense_allocators>;
  using map_handle =
      dependent_lib::allocator_adaptor<raw_map_handle>;
  using map_t = std::map<vec_t, vec_t, std::less<>, map_handle>;

  dense_allocators allocs(backing_allocator{});
  map_t container(&allocs);
  container.emplace(std::string("abc"), std::string("def"));
}

TEST_CASE("vector_dependent_vectors", "[dependent_lib, dense_allocator]") {
  using backing_allocator = std::allocator<void>;
  using dense_allocators =
      dependent_lib::dense_allocators<backing_allocator, int,
                                      dependent_lib::unknown_type<8, 8>>;

  using raw_vec_t_handle =
      dependent_lib::dense_allocator_handler<int, dense_allocators>;
  using vec_t_handle = dependent_lib::allocator_adaptor<raw_vec_t_handle>;
  using vec_t = dependent_lib::vector<int, vec_t_handle>;

  using raw_outter_vec_handle =
      dependent_lib::dense_allocator_handler<vec_t, dense_allocators>;
  using outter_vec_handle =
      dependent_lib::allocator_adaptor<raw_outter_vec_handle>;
  using outter_vec_t = std::vector<vec_t, outter_vec_handle>;

  dense_allocators allocs(backing_allocator{});
  outter_vec_t container(&allocs);
  container.emplace_back(std::begin(arr1), std::end(arr1));
}

}  // namespace
