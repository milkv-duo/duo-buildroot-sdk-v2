#include "demangle.hpp"
#ifdef __GNUG__
#include <cxxabi.h>
#include <cstdlib>
#include <memory>

namespace cvitdl {
namespace demangle {
std::string demangle(const char* name) {
  int status = -4;  // some arbitrary value to eliminate the compiler warning
  std::unique_ptr<char, void (*)(void*)> res{abi::__cxa_demangle(name, NULL, NULL, &status),
                                             std::free};

  return (status == 0) ? res.get() : name;
}
}  // namespace demangle
}  // namespace cvitdl

#else

namespace cvitdl {
namespace demangle {
// does nothing if not g++
std::string demangle(const char* name) { return name; }
}  // namespace demangle
}  // namespace cvitdl
#endif