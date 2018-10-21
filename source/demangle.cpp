#include "demangle.hpp"

#include <cxxabi.h>
#include <memory>

namespace lince {

struct StrDeleter {
    void operator()(char *S) const noexcept
    {
        ::free(S);
    }
};

std::string demangle(const std::string &Name)
{
    int status;
    std::unique_ptr<char, StrDeleter> X(abi::__cxa_demangle(Name.c_str(), nullptr, nullptr, &status));
    return X.get();
}
}