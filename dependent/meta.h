#ifndef _META_H_
#define _META_H_

#include <iterator>

namespace dependent_lib {

namespace detail {

template <typename I>
using iterator_category_t = typename std::iterator_traits<I>::iterator_category;

template <typename T, typename A, typename = void>
struct DependentTypeImpl : std::false_type {};

template <typename T, typename A>
struct DependentTypeImpl<  //
    T, A,                  //
    std::void_t<decltype(
        std::declval<T&>().destroy(std::declval<const A&>()))>  //
    > : std::uses_allocator<T, A> {};                           //

template <typename I, typename = void>
struct IteratorImpl : std::false_type {};

template <typename I>
struct IteratorImpl<I, std::void_t<iterator_category_t<I>>> : std::true_type {};

template <typename I, typename = void>
struct ForwardIteratorImpl : std::false_type {};

template <typename I>
struct ForwardIteratorImpl<
    I,  //
    std::enable_if_t<std::is_base_of<std::forward_iterator_tag,
                                     iterator_category_t<I>>::value>>
    : std::true_type {};

template <typename I, typename = void>
struct RandomIteratorImpl : std::false_type {};

template <typename I>
struct RandomIteratorImpl<
    I, std::enable_if_t<std::is_base_of<std::random_access_iterator_tag,
                                        iterator_category_t<I>>::value>>
    : std::true_type {};

template <typename R, typename = void>
struct ForwardRangeImpl : std::false_type {};

template <typename R>
struct ForwardRangeImpl<R, std::void_t<decltype(std::begin(std::declval<R>()),
                                                std::end(std::declval<R>()))>>
    : ForwardIteratorImpl<decltype(std::begin(std::declval<R>()))> {};

template <typename R>
constexpr auto forward_begin(std::remove_reference_t<R>& r) {
  return std::begin(r);
}

template <typename R>
auto forward_begin(std::remove_reference_t<R>&& r) {
  return std::make_move_iterator(std::begin(r));
}

template <typename R>
auto forward_end(std::remove_reference_t<R>& r) {
  return std::end(r);
}

template <typename R>
auto forward_end(std::remove_reference_t<R>&& r) {
  return std::make_move_iterator(std::end(r));
}

}  // namespace detail

template <typename T, typename A>
constexpr bool DependentType = detail::DependentTypeImpl<T, A>::value;

template <typename I>
constexpr bool Iterator = detail::IteratorImpl<I>::value;

template <typename I>
constexpr bool ForwardIterator = detail::ForwardIteratorImpl<I>::value;

template <typename I>
constexpr bool RandomAccessIterator = detail::RandomIteratorImpl<I>::value;

template <typename R>
constexpr bool ForwardRange = detail::ForwardRangeImpl<R>::value;

}  // namespace dependent_lib

#endif  // _META_H_
