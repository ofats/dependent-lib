#include <array>
#include <cinttypes>
#include <forward_list>
#include <memory>
#include <vector>

namespace dependent {

namespace detail {

// TODO: forward_list nodes are poorly aligned.
constexpr std::size_t memory_block_size = 4096;

template <std::size_t sizeof_T, std::size_t alignof_T,
          typename BackingAllocator>
class single_dense_allocator {
  using cage = std::aligned_storage_t<sizeof_T, alignof_T>;
  static constexpr std::size_t cage_count = memory_block_size / sizeof_T;

  using backing_allocator_type = BackingAllocator;
  using alloc_traits = std::allocator_traits<backing_allocator_type>;
  using memory_block = std::array<cage, cage_count>;
  using memory_blocks_allocator =
      typename alloc_traits::template rebind_alloc<memory_block>;
  using backing_allocator_for_t = typename std::allocator_traits<
      memory_blocks_allocator>::template rebind_alloc<cage>;

  std::forward_list<memory_block, memory_blocks_allocator> memory_blocks_;
  std::vector<std::pair<
      typename std::allocator_traits<backing_allocator_for_t>::pointer,
      std::size_t>>
      to_delete_;
  cage* tail_;

  void* fit_allocation(std::size_t size) {
    auto* to_return = tail_;
    tail_ += size;
    return to_return;
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

  void* allocate(std::size_t size) {
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
    return p;
  }
};

}  // namespace detail

template <std::size_t size, std::size_t alignment>
using unknown_type = std::aligned_storage_t<size, alignment>;

template <typename Alloc, typename... Ts>
struct dense_allocators
    : detail::single_dense_allocator<sizeof(Ts), alignof(Ts), Alloc>... {
  using backing_allocator_type = Alloc;
  using alloc_traits = std::allocator_traits<backing_allocator_type>;
  using pointer = typename alloc_traits::pointer;

  backing_allocator_type backing_allocator_;

  // This is an only way I could find to make a reasonable error message with sizes.
  template <std::size_t size, std::size_t alignment>
  detail::single_dense_allocator<size, alignment, backing_allocator_type>& as_allocator_for_T() {
    return *this;
  }

  dense_allocators(const backing_allocator_type& a)
      : detail::single_dense_allocator<sizeof(Ts), alignof(Ts), backing_allocator_type>(a)...,
        backing_allocator_(a) {}
};

template <typename T, typename DenseAllocator>
class dense_allocator_handler {
  using backing_allocator_type =
      typename DenseAllocator::backing_allocator_type;
  using alloc_traits = std::allocator_traits<backing_allocator_type>;

  template <typename U, typename A>
  friend class dense_allocator_handler;

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
    auto& as_t_alloc = dense_allocator_->template as_allocator_for_T<sizeof(T), alignof(T)>();
    return static_cast<T*>(as_t_alloc.allocate(size));
  }

  void deallocate(T*, std::size_t) {}

  friend bool operator==(const dense_allocator_handler& x,
                         const dense_allocator_handler& y) {
    return x.dense_allocator_ == y.dense_allocator_;
  }

  friend bool operator!=(const dense_allocator_handler& x,
                         const dense_allocator_handler& y) {
    return !(x == y);
  }
};

}  // namespace dependent
