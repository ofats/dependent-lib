#ifndef _DEPENDENT_H_
#define _DEPENDENT_H_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <scoped_allocator>
#include <tuple>
#include <type_traits>

#include "dependent/future_std_stubs.h"
#include "dependent/meta.h"

namespace dependent {

// This is a cusomization point for the prefered span.
template <typename T>
using span = future_std_stubs::span<T>;

namespace detail {

using size_types = std::tuple<uint8_t, uint16_t, uint32_t, uint64_t>;
using bigest_size_type =
    std::tuple_element_t<std::tuple_size<size_types>::value - 1, size_types>;
static_assert(sizeof(bigest_size_type) >= sizeof(size_t), "");

template <typename T>
constexpr std::size_t select_size_type() {
  if (sizeof(T) <= sizeof(uint8_t)) return 0;
  if (sizeof(T) <= sizeof(uint16_t)) return 1;
  if (sizeof(T) <= sizeof(uint32_t)) return 2;
  static_assert(std::tuple_size<size_types>::value == 4, "");
  return 3;
}

template <typename T>
using size_type_t = std::tuple_element_t<select_size_type<T>(), size_types>;

template <typename T>
constexpr std::ptrdiff_t required_space_in_types(std::ptrdiff_t size) {
  if (size < std::numeric_limits<size_type_t<T>>::max()) {
    return size + 1;
  }
  constexpr std::size_t size_t_offset =
      (alignof(std::size_t) + sizeof(std::size_t)) / sizeof(T);
  return 1 + size_t_offset + size;
}

template <typename I, typename Alloc>
void destroy(I f, I l, Alloc a) {
  using ri = std::reverse_iterator<I>;
  for (ri it{l}; it != ri{f}; ++it)
    std::allocator_traits<Alloc>::destroy(a, it.base());
}

template <typename A>
struct allocator_adaptor_impl : A {
 private:
  using a_traits = std::allocator_traits<A>;

  template <typename T>
  auto maybe_destroy(T* x) noexcept
      -> std::enable_if_t<DependentType<T, allocator_adaptor_impl>> {
    x->destroy(*this);
  }

  template <typename T>
  auto maybe_destroy(T*) noexcept
      -> std::enable_if_t<!DependentType<T, allocator_adaptor_impl>> {}

 public:
  using A::A;

  template <typename T>
  struct rebind {
    using other =
        allocator_adaptor_impl<typename a_traits::template rebind_alloc<T>>;
  };

  template <typename T>
  void destroy(T* x) noexcept {
    maybe_destroy(x);
    a_traits::destroy(*this, x);
  }

  template <typename T, typename U>
  void destroy(std::pair<T, U>* p) noexcept {
    maybe_destroy(&p->first);
    maybe_destroy(&p->second);
    a_traits::destroy(*this, p);
  }
};

template <typename T, typename U>
using forward_const_t = std::conditional_t<std::is_const<T>::value, const U, U>;

template <typename U, typename T>
auto unaligned_read(T* ptr) -> std::pair<T*, forward_const_t<T, U>&> {
  using u_ptr = forward_const_t<T, U>*;
  auto* raw_ptr =
      reinterpret_cast<void*>(const_cast<std::remove_const_t<T>*>(ptr));

  // Read U.
  std::size_t fake_size = std::numeric_limits<std::size_t>::max();
  raw_ptr = std::align(alignof(U), sizeof(U), raw_ptr, fake_size);
  assert(raw_ptr);
  const u_ptr as_u = static_cast<u_ptr>(raw_ptr);

  // Advance ptr.
  raw_ptr = reinterpret_cast<void*>(const_cast<U*>(as_u + 1));
  raw_ptr = std::align(alignof(T), sizeof(T), raw_ptr, fake_size);
  assert(raw_ptr);

  return {static_cast<std::remove_const_t<T>*>(raw_ptr), *as_u};
}

template <typename T, typename Span, typename Alloc>
struct leaky_vector {
  using allocator_type = Alloc;
  using alloc_traits = std::allocator_traits<Alloc>;
  using pointer = typename alloc_traits::pointer;
  using size_type = std::size_t;
  using span_type = Span;

  static constexpr std::size_t big_size_marker =
      std::numeric_limits<size_type_t<T>>::max();

  pointer data_;

  detail::size_type_t<T>& small_size() {
    return *reinterpret_cast<detail::size_type_t<T>*>(data_);
  }

  const detail::size_type_t<T>& small_size() const {
    return *reinterpret_cast<const detail::size_type_t<T>*>(data_);
  }

  T* unaligned_write_size(size_type dist) {
    if (dist < big_size_marker) {
      small_size() = dist;
      return &*data_ + 1;
    }

    small_size() = big_size_marker;
    auto p = unaligned_read<std::size_t>(&*data_ + 1);
    p.second = static_cast<std::size_t>(dist);
    return p.first;
  }

  std::pair<const T*, const T*> begin_end() const noexcept {
    const T* begin = &*data_ + 1;
    if (small_size() < big_size_marker) {
      return {begin, begin + small_size()};
    }
    std::size_t big_size;
    std::tie(begin, big_size) = unaligned_read<std::size_t>(begin);
    return {begin, begin + big_size};
  }

  std::pair<T*, T*> begin_end() noexcept {
    auto p = static_cast<const leaky_vector*>(this)->begin_end();
    return {const_cast<T*>(p.first), const_cast<T*>(p.second)};
  }

  size_type size() const noexcept {
    auto p = begin_end();
    return p.second - p.first;
  }

  constexpr static std::size_t required_allocation_size(std::ptrdiff_t dist) {
    auto space_in_types = detail::required_space_in_types<T>(dist);
    return space_in_types;
  }

  template <typename I, typename = std::enable_if_t<ForwardIterator<I>>>
  leaky_vector(std::allocator_arg_t, allocator_type a, I f, I l) {
    auto dist = std::distance(f, l);
    auto allocated_memory = required_allocation_size(dist);
    data_ = alloc_traits::allocate(a, allocated_memory);

    T* t_begin = unaligned_write_size(dist);
    T* t_cur = t_begin;
    try {
      for (; f != l; ++f, ++t_cur) alloc_traits::construct(a, t_cur, *f);
    } catch (...) {
      detail::destroy(t_begin, t_cur, a);
      alloc_traits::deallocate(a, data_, allocated_memory);
    }
  }

  template <typename R, typename = std::enable_if_t<ForwardRange<R>>>
  leaky_vector(std::allocator_arg_t, allocator_type a, R&& r)
      : leaky_vector(std::allocator_arg, a, forward_begin<R>(r),
                     forward_end<R>(r)) {}

  leaky_vector(std::allocator_arg_t, allocator_type, leaky_vector&& rhs)
      : leaky_vector(std::move(rhs)) {}

  span_type as_span() const noexcept {
    auto p = begin_end();
    return {p.first, p.second};
  }

  friend bool operator==(const leaky_vector& x,
                         const leaky_vector& y) noexcept {
    auto xs = x.as_span();
    auto ys = y.as_span();
    return std::equal(xs.begin(), xs.end(), ys.begin(), ys.end());
  }

  friend bool operator!=(const leaky_vector& x,
                         const leaky_vector& y) noexcept {
    return !(x == y);
  }

  friend bool operator<(const leaky_vector& x, const leaky_vector& y) noexcept {
    auto xs = x.as_span();
    auto ys = y.as_span();
    return std::lexicographical_compare(xs.begin(), xs.end(), ys.begin(),
                                        ys.end());
  }

  friend bool operator>(const leaky_vector& x, const leaky_vector& y) noexcept {
    return y < x;
  }

  friend bool operator<=(const leaky_vector& x,
                         const leaky_vector& y) noexcept {
    return !(x > y);
  }

  friend bool operator>=(const leaky_vector& x,
                         const leaky_vector& y) noexcept {
    return !(x < y);
  }
};

}  // namespace detail

template <typename A>
using allocator_adaptor =
    std::scoped_allocator_adaptor<detail::allocator_adaptor_impl<A>>;

template <typename T, typename Alloc>
class vector;

template <typename T, typename Alloc>
class leaky_vector : detail::leaky_vector<T, span<const T>, Alloc> {
  friend class vector<T, Alloc>;
  using base = detail::leaky_vector<T, span<const T>, Alloc>;
  base& as_base() noexcept { return *this; };
  const base& as_base() const noexcept { return *this; };

 public:
  using base::as_span;

  template <typename... Args>
  leaky_vector(Args&&... args) : base(std::forward<Args>(args)...) {}

  friend bool operator==(const leaky_vector& x, const leaky_vector& y) {
    return x.as_base() < y.as_base();
  }

  friend bool operator!=(const leaky_vector& x, const leaky_vector& y) {
    return !(x == y);
  }

  friend bool operator<(const leaky_vector& x, const leaky_vector& y) {
    return x.as_base() < y.as_base();
  }

  friend bool operator>(const leaky_vector& x, const leaky_vector& y) {
    return y < x;
  }

  friend bool operator<=(const leaky_vector& x, const leaky_vector& y) {
    return !(x > y);
  }

  friend bool operator>=(const leaky_vector& x, const leaky_vector& y) {
    return !(x < y);
  }
};

template <typename T, typename Alloc>
class vector : public leaky_vector<T, Alloc> {
  using base = leaky_vector<T, Alloc>;
  using alloc_traits = std::allocator_traits<Alloc>;

 public:
  using allocator_type = typename base::allocator_type;

  template <typename... Args>
  vector(Args&&... args) : base(std::forward<Args>(args)...) {}

  void destroy(allocator_type a) {
    T *f, *l;
    std::tie(f, l) = this->begin_end();
    detail::destroy(f, l, a);
    alloc_traits::deallocate(a, this->data_,
                             this->required_allocation_size(l - f));
    this->data_ = nullptr;
  }
};

}  // namespace dependent

#endif  // _DEPENDENT_LIB_H_
