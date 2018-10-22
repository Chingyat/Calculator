#pragma once
#include <cassert>
#include <type_traits>
#include <typeinfo>

namespace lince {
template <typename Derived, typename Base>
std::enable_if_t<std::is_base_of_v<Base, Derived>, bool>
isa(const Base *B) noexcept {
  assert(B);
  return Derived::classof(B);
}

template <typename Derived, typename Base>
std::enable_if_t<std::is_base_of_v<Base, Derived>, bool>
isa(const Base &B) noexcept {
  return Derived::classof(&B);
}

template <typename Derived, typename Base,
          typename Result = std::conditional_t<std::is_const_v<Base>,
                                               const Derived *, Derived *>>
Result cast_or_null(Base *B) noexcept {
  if (!B)
    return nullptr;
  assert(isa<Derived>(B));
  return static_cast<Result>(B);
}

template <typename Derived, typename Base>
decltype(auto) cast(Base *B) noexcept {
  assert(B);
  return cast_or_null<Derived>(B);
}

template <typename Derived, typename Base,
          typename Result = std::conditional_t<std::is_const_v<Base>,
                                               const Derived &, Derived &>>
Result cast(Base &B) noexcept {
  assert(isa<Derived>(B));
  return static_cast<Result>(B);
}

template <typename Derived, typename Base>
decltype(auto) dyn_cast_or_null(Base *B) noexcept {
  return B && isa<Derived>(B) ? cast<Derived>(B) : nullptr;
}

template <typename Derived, typename Base>
decltype(auto) dyn_cast(Base *B) noexcept {
  assert(B);
  return dyn_cast_or_null<Derived>(B);
}

template <typename Derived, typename Base> decltype(auto) dyn_cast(Base &B) {
  if (isa<Derived>(B))
    return cast<Derived>(B);
  throw std::bad_cast();
}

} // namespace lince
