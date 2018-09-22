#ifndef _DEPENDEDENT_UTILS_STATS_ALLOCATOR_H_
#define _DEPENDEDENT_UTILS_STATS_ALLOCATOR_H_

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// Wrapper for std::allocator to collect some memory stats.
// (total memory usage, memory usage for specific types etc)
//
// The metrics counting is not thread safe.

namespace dependent {

namespace detail {

// Stats for single type.
// Stats for a single type are linked into an intrusive list for an area.
struct memory_stats_for_t {
  std::size_t allocated_size = 0u;
  const memory_stats_for_t* next = nullptr;

  memory_stats_for_t(const memory_stats_for_t*& list)
      : next{std::exchange(list, this)} {}
};

}  // namespace detail

template <typename Tag>
class area_stats {
  using t_stats = detail::memory_stats_for_t;

  // Global list of stats for a single area.
  static const t_stats*& stats_list() {
    static const t_stats* r;
    return r;
  }

  template <typename T>
  static t_stats& stats_for_t() {
    static t_stats r{stats_list()};
    return r;
  }

 public:
  template <typename T>
  static std::size_t allocated_size_for_t() {
    return stats_for_t<T>().allocated_size;
  }

  static std::size_t total_allocated_size() {
    std::size_t res = 0;
    for (const t_stats* head = stats_list(); head; head = head->next) {
      res += head->allocated_size;
    }
    return res;
  }

  template <typename T>
  static void report_allocation(std::size_t size) {
    stats_for_t<T>().allocated_size += size;
  }

  template <typename T>
  static void report_deallocation(std::size_t size) {
    stats_for_t<T>().allocated_size -= size;
  }
};

template <typename T, typename Tag>
class stats_allocator {
  using global_stats = area_stats<Tag>;

  using std_traits = std::allocator_traits<std::allocator<T>>;

 public:
  using value_type = T;
  using propagate_on_container_move_assignment = std::true_type;

  using pointer = typename std_traits::pointer;
  using size_type = typename std_traits::size_type;
  using const_void_pointer = typename std_traits::const_void_pointer;

  constexpr stats_allocator() noexcept = default;
  constexpr stats_allocator(const stats_allocator&) noexcept = default;
  constexpr stats_allocator& operator=(const stats_allocator&) noexcept =
      default;

  template <typename U>
  stats_allocator(const stats_allocator<U, Tag>&) {}

  static pointer allocate(size_type n, const_void_pointer hint = 0) {
    global_stats::template report_allocation<T>(n * sizeof(T));
    return std::allocator<T>().allocate(n, hint);
  }

  static void deallocate(pointer p, size_type n) {
    global_stats::template report_deallocation<T>(n * sizeof(T));
    std::allocator<T>().deallocate(p, n);
  }
};

}  // namespace dependent

#endif  // _DEPENDEDENT_UTILS_STATS_ALLOCATOR_H_
