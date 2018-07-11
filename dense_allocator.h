#include <array>
#include <cinttypes>
#include <forward_list>
#include <memory>
#include <vector>

namespace dependent_lib {

namespace detail {

// TODO: forward_list nodes are poorly aligned.
constexpr std::size_t memory_block_size = 4096;

template <typename T, typename BackingAllocator>
class single_dense_allocator {
  union cage {
    T _;
  };
  static_assert(sizeof(cage) == sizeof(T), "");
  static_assert(alignof(cage) == alignof(T), "");

  static constexpr std::size_t cage_count = memory_block_size / sizeof(T);

  using backing_allocator_type = BackingAllocator;
  using alloc_traits = std::allocator_traits<backing_allocator_type>;
  using memory_block = std::array<cage, cage_count>;
  using memory_blocks_allocator =
      typename alloc_traits::template rebind_alloc<memory_block>;
  using backing_allocator_for_t = typename std::allocator_traits<
        memory_blocks_allocator>::template rebind_alloc<T>;

  std::forward_list<memory_block, memory_blocks_allocator> memory_blocks_;
  std::vector<std::pair<
      typename std::allocator_traits<backing_allocator_for_t>::pointer,
      std::size_t>>
      to_delete_;
  cage* tail_;

  T* fit_allocation(std::size_t size) {
    auto* to_return = tail_;
    tail_ += size;
    return reinterpret_cast<T*>(to_return);
  }

 public:
  explicit single_dense_allocator(const backing_allocator_type& alloc)
      : memory_blocks_(alloc) {
    memory_blocks_.emplace_front();
    tail_ = memory_blocks_.front().begin();
  }

  single_dense_allocator(const single_dense_allocator&) = delete;
  single_dense_allocator& operator=(const single_dense_allocator&) = delete;

  ~single_dense_allocator() {
    backing_allocator_for_t a{memory_blocks_.get_allocator()};
    for (auto& p : to_delete_) {
      std::allocator_traits<backing_allocator_for_t>::deallocate(a, p.first,
                                                                 p.second);
    }
  }

  T* allocate(std::size_t size) {
    // TODO: thinking.
    if (memory_blocks_.front().end() - tail_ >=
        static_cast<std::ptrdiff_t>(size)) {
      return fit_allocation(size);
    }
    if (size <= cage_count) {
      memory_blocks_.emplace_front();
      tail_ = memory_blocks_.front().begin();
      return fit_allocation(size);
    }
    backing_allocator_for_t a{memory_blocks_.get_allocator()};
    auto p = std::allocator_traits<backing_allocator_for_t>::allocate(a, size);
    to_delete_.emplace_back(p, size);
    return static_cast<T*>(p);
  }
};

}  // namespace detail

template <typename Alloc, typename... Ts>
struct dense_allocators : detail::single_dense_allocator<Ts, Alloc>... {
  using backing_allocator_type = Alloc;
  using alloc_traits = std::allocator_traits<backing_allocator_type>;
  using pointer = typename alloc_traits::pointer;

  backing_allocator_type backing_allocator_;

  dense_allocators(const backing_allocator_type& a)
      : detail::single_dense_allocator<Ts, Alloc>(a)...,
        backing_allocator_(a) {}
};

template <typename T, typename DenseAllocator>
class dense_allocator_handler {
  using backing_allocator_type =
      typename DenseAllocator::backing_allocator_type;
  using alloc_traits = std::allocator_traits<backing_allocator_type>;

  DenseAllocator* dense_allocator_;

 public:
  using value_type = T;

  template <typename U>
  struct rebind {
    using other = dense_allocator_handler<U, DenseAllocator>;
  };

  explicit dense_allocator_handler(DenseAllocator* dense_allocator)
      : dense_allocator_(dense_allocator) {}
  dense_allocator_handler(const dense_allocator_handler&) = default;
  dense_allocator_handler& operator=(const dense_allocator_handler&) = default;
  ~dense_allocator_handler() = default;

  template <typename U, typename = std::enable_if_t<!std::is_same<U, T>::value>>
  dense_allocator_handler(const dense_allocator_handler<U, DenseAllocator>& x)
      : dense_allocator_(x.dense_allocator_) {}

  template <typename U, typename = std::enable_if_t<!std::is_same<U, T>::value>>
  dense_allocator_handler& operator=(
      const dense_allocator_handler<U, DenseAllocator>& x) {
    dense_allocator_ = x.dense_allocator_;
    return *this;
  }

  T* allocate(std::size_t size) {
    detail::single_dense_allocator<T, backing_allocator_type>& as_t_alloc(
        *dense_allocator_);
    return as_t_alloc.allocate(size);
  }

  void deallocate(T*, std::size_t) {}

  friend bool operator==(const dense_allocator_handler& x, const dense_allocator_handler& y) {
    return x.dense_allocator_ == y.dense_allocator_;
  }

  friend bool operator!=(const dense_allocator_handler& x, const dense_allocator_handler& y) {
    return !(x == y);
  }
};

}  // namespace dependent_lib
