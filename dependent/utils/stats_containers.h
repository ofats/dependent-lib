#ifndef _DEPENDENT_UTILS_STATS_CONTAINERS_H_
#define _DEPENDENT_UTILS_STATS_CONTAINERS_H_

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "dependent/utils/stats_allocator.h"

namespace std {

// Dealing with libstdc++ lack of support for hashing strings with a non-default
// allocator.
template <typename Tag, typename Char, typename CharTraits>
class hash<std::basic_string<Char, CharTraits,
                             dependent::stats_allocator<Char, Tag>>> {
  using sv = std::basic_string_view<Char, CharTraits>;

 public:
  constexpr std::size_t operator()(sv s) const { return std::hash<sv>{}(s); }
};

}  // namespace std

namespace dependent {

template <typename Tag>
struct stats_containers {
  template <typename T>
  using allocator = dependent::stats_allocator<T, Tag>;

  template <typename T>
  using vector = std::vector<T, allocator<T>>;

  template <typename Key, typename Hash = std::hash<Key>,
            typename KeyEqual = std::equal_to<Key>>
  using unordered_set =
      std::unordered_set<Key, Hash, KeyEqual, allocator<Key>>;

  template <typename Key, typename T, typename Hash = std::hash<Key>,
            typename KeyEqual = std::equal_to<Key>>
  using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual,
                                           allocator<std::pair<const Key, T>>>;

  template <typename Char, typename CharTraits = std::char_traits<Char>>
  using basic_string = std::basic_string<Char, CharTraits, allocator<Char>>;

  using string = basic_string<char>;
};

}  // namespace dependent

#endif  // _DEPENDENT_UTILS_STATS_CONTAINERS_H_
