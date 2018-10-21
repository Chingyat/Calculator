#include "demangle.hpp"

#include <boost/core/demangle.hpp>

namespace lince {
std::string demangle(const std::string &Name)
{
    return boost::core::demangle(Name.c_str());
}
}