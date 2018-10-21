#include "demangle.hpp"

#include <cxxabi.h>

namespace lince {
std::string demangle(const std::string &Name)
{
   int status;
   char *s = abi::__cxa_demangle(Name.c_str(), 0, 0, &status);
   std::string r(s);
   free(s);
   return r;
}
}
