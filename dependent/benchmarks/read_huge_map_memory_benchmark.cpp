#include <experimental/string_view>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <unordered_set>

#include "dependent/utils/stats_allocator.h"
#include "dependent/utils/stats_containers.h"

constexpr std::string_view c_items_separator = "@@@";

template <typename Tag>
using stats = dependent::stats_containers<Tag>;

class usage_error : public std::exception {
  std::string msg_;

 public:
  usage_error(std::string_view executable_name) {
    msg_ = "Usage error, expceted usage:";
    msg_ += executable_name;
    msg_ += " <path_to_strings>\n";
  }
  const char* what() const noexcept override { return msg_.c_str(); }
};

template <typename Container>
void read_into_container(std::string_view file_name, Container* container) {
  std::fstream in(std::string(file_name), std::ios::in);

  auto it = container->begin();

  std::string buffer;
  std::string element;
  while (in) {
    std::getline(in, buffer);
    if (buffer == c_items_separator) {
      if (!element.empty())
        it = container->insert(it, typename Container::value_type{element});
      element.clear();
      continue;
    }
    if (buffer.empty()) continue;
    element += buffer;
  }
}

int main(int argc, const char* argv[]) {
  static const usage_error usage_err(argv[0]);

  try {
    if (argc != 2) throw usage_err;

    struct tag {};
    stats<tag>::unordered_set<stats<tag>::string> dict;
    read_into_container(argv[1], &dict);

    std::cout << "Allocated size: "
              << dependent::area_stats<tag>::total_allocated_size()
              << std::endl;

  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
  }
}
