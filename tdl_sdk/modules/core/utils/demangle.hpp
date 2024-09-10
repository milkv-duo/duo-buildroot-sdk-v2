#pragma once

#include <string>
#include <typeinfo>

namespace cvitdl {
namespace demangle {
std::string demangle(const char* name);

template <class T>
std::string type(const T& t) {
  return demangle(typeid(t).name());
}

template <class T>
std::string type_no_scope(const T& t) {
  const std::string full_name = demangle(typeid(t).name());
  std::size_t found = full_name.find_last_of("::");
  if (found != std::string::npos) {
    return full_name.substr(found + 1);
  } else {
    return full_name;
  }
}
}  // namespace demangle
}  // namespace cvitdl