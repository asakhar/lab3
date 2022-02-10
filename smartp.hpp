#ifndef SMARTP_HPP
#define SMARTP_HPP

#include <compare>
#include <cstddef>
#include <type_traits>

#include "type_traits.hpp"

template <typename T>
void swap(T a, T b) {
  auto tmp = a;
  a = b;
  b = a;
}

template <typename T>
struct default_deleter_t {
  void operator()(T* pointer) { delete pointer; }
};
template <typename T>
struct default_deleter_t<T[]> {
  void operator()(T* pointer) { delete[] pointer; }
};
template <typename T>
struct default_allocator_t {
  template <typename... Args>
  T* operator()(Args&&... args) {
    return new T{forward(args)...};
  }
};
template <typename T>
struct default_allocator_t<T[]> {
  T* operator()(size_t size) { return new T[size]; }
};

template <typename T, typename D>
class uniq_ptr;

template <typename T, typename D>
auto operator<=>(uniq_ptr<T, D> const& self,
                 uniq_ptr<T, D> const& other) noexcept {
  return self.m_ptr < other.m_ptr   ? std::strong_ordering::less
         : self.m_ptr > other.m_ptr ? std::strong_ordering::greater
                                    : std::strong_ordering::equal;
}

#define COMPARISON_GENERATOR(oper)                           \
  template <typename T, typename D>                          \
  auto operator oper(uniq_ptr<T, D> const& self,             \
                     uniq_ptr<T, D> const& other) noexcept { \
    return self.m_ptr oper other.m_ptr;                      \
  }

#define COMPARISON_GENERATORS \
  COMPARISON_GENERATOR(==)    \
  COMPARISON_GENERATOR(!=)    \
  COMPARISON_GENERATOR(<=)    \
  COMPARISON_GENERATOR(>=)    \
  COMPARISON_GENERATOR(<)     \
  COMPARISON_GENERATOR(>)

#define COMPARISON_FRIEND(oper)                                    \
  friend auto operator oper<>(uniq_ptr<type, deleter> const& self, \
                              uniq_ptr<type, deleter> const& other) noexcept;
#define COMPARISON_FRIENDS \
  COMPARISON_FRIEND(==)    \
  COMPARISON_FRIEND(!=)    \
  COMPARISON_FRIEND(<=)    \
  COMPARISON_FRIEND(>=)    \
  COMPARISON_FRIEND(<)     \
  COMPARISON_FRIEND(>)

COMPARISON_GENERATORS

template <typename _Ty, typename _Deleter = default_deleter_t<_Ty>>
class uniq_ptr {
 public:
  using type = _Ty;
  using pointer = type*;
  using reference = type&;
  using deleter = remove_reference_t<_Deleter>;

  constexpr uniq_ptr() noexcept = default;
  constexpr uniq_ptr(nullptr_t) noexcept {};
  constexpr explicit uniq_ptr(deleter&& del) noexcept : m_del{del} {}
  virtual reference operator*() const noexcept { return *m_ptr; }
  virtual pointer operator->() const noexcept { return m_ptr; }
  virtual pointer get() const noexcept { return m_ptr; }
  virtual pointer release() noexcept {
    auto tmp = m_ptr;
    m_ptr = nullptr;
    return tmp;
  }
  explicit operator bool() const noexcept { return m_ptr != nullptr; }
  template <convertible<type> T>
  operator T*() const noexcept {
    return m_ptr;
  }
  explicit uniq_ptr(pointer ptr) noexcept : m_ptr{ptr} {}
  uniq_ptr(pointer ptr, deleter&& del) noexcept : m_ptr{ptr}, m_del{del} {}
  uniq_ptr(uniq_ptr const&) = delete;
  uniq_ptr(uniq_ptr&& move) : m_ptr{move.m_ptr} { move.m_ptr = nullptr; }
  uniq_ptr& operator=(uniq_ptr const&) = delete;
  uniq_ptr& operator=(uniq_ptr&& move) {
    reset(move.m_ptr);
    move.m_ptr = nullptr;
  }
  void reset(pointer ptr = nullptr) {
    m_del(this->m_ptr);
    this->m_ptr = ptr;
  }
  void swap(uniq_ptr other) { swap(this->m_ptr, other.m_ptr); }
  ~uniq_ptr() { reset(); }

  COMPARISON_FRIENDS

 protected:
  pointer m_ptr = nullptr;
  remove_rvalue_reference_t<_Deleter> m_del;
};

template <typename _Ty, typename _Deleter>
class uniq_ptr<_Ty[], _Deleter> {
 public:
  using type = _Ty;
  using pointer = type*;
  using reference = type&;
  using deleter = remove_reference_t<_Deleter>;

  constexpr uniq_ptr() noexcept = default;
  constexpr uniq_ptr(std::nullptr_t) noexcept {};
  constexpr explicit uniq_ptr(deleter&& del) noexcept : m_del{del} {}
  virtual reference operator*() const noexcept { return *m_ptr; }
  virtual pointer operator->() const noexcept { return m_ptr; }
  reference operator[](size_t idx) const noexcept { return this->m_ptr[idx]; }
  virtual pointer get() const noexcept { return m_ptr; }
  virtual pointer release() noexcept {
    auto tmp = m_ptr;
    m_ptr = nullptr;
    return tmp;
  }
  explicit operator bool() const noexcept { return m_ptr != nullptr; }
  template <convertible<type> T>
  operator T*() const noexcept {
    return m_ptr;
  }
  explicit uniq_ptr(pointer ptr) noexcept : m_ptr{ptr} {}
  uniq_ptr(pointer ptr, deleter&& del) noexcept : m_ptr{ptr}, m_del{del} {}
  uniq_ptr(uniq_ptr const&) = delete;
  uniq_ptr(uniq_ptr&& move) : m_ptr{move.m_ptr} { move.m_ptr = nullptr; }
  uniq_ptr& operator=(uniq_ptr const&) = delete;
  uniq_ptr& operator=(uniq_ptr&& move) {
    reset(move.m_ptr);
    move.m_ptr = nullptr;
  }
  void reset(pointer ptr = nullptr) {
    m_del(this->m_ptr);
    this->m_ptr = ptr;
  }
  void swap(uniq_ptr other) { swap(this->m_ptr, other.m_ptr); }
  ~uniq_ptr() { reset(); }

  COMPARISON_FRIENDS

 protected:
  pointer m_ptr = nullptr;
  remove_rvalue_reference_t<_Deleter> m_del;
};

template <typename _Ty>
class uniq_ptr<_Ty[], default_deleter_t<_Ty[]>> {
 public:
  using type = _Ty;
  using pointer = type*;
  using reference = type&;
  using deleter = default_deleter_t<type[]>;

  constexpr uniq_ptr() noexcept = default;
  constexpr uniq_ptr(std::nullptr_t) noexcept {};
  constexpr explicit uniq_ptr(deleter&& del) noexcept : m_del{del} {}
  virtual reference operator*() const noexcept { return *m_ptr; }
  virtual pointer operator->() const noexcept { return m_ptr; }
  reference operator[](size_t idx) const noexcept { return this->m_ptr[idx]; }
  virtual pointer get() const noexcept { return m_ptr; }
  virtual pointer release() noexcept {
    auto tmp = m_ptr;
    m_ptr = nullptr;
    return tmp;
  }
  explicit operator bool() const noexcept { return m_ptr != nullptr; }
  template <convertible<type> T>
  operator T*() const noexcept {
    return m_ptr;
  }
  explicit uniq_ptr(pointer ptr) noexcept : m_ptr{ptr} {}
  uniq_ptr(pointer ptr, deleter&& del) noexcept : m_ptr{ptr}, m_del{del} {}
  uniq_ptr(uniq_ptr const&) = delete;
  uniq_ptr(uniq_ptr&& move) : m_ptr{move.m_ptr} { move.m_ptr = nullptr; }
  uniq_ptr& operator=(uniq_ptr const&) = delete;
  uniq_ptr& operator=(uniq_ptr&& move) {
    reset(move.m_ptr);
    move.m_ptr = nullptr;
  }
  void reset(pointer ptr = nullptr) {
    m_del(this->m_ptr);
    this->m_ptr = ptr;
  }
  void swap(uniq_ptr other) { swap(this->m_ptr, other.m_ptr); }
  ~uniq_ptr() { reset(); }

  COMPARISON_FRIENDS

 protected:
  pointer m_ptr = nullptr;
  deleter m_del;
};

template <typename T, typename... Args>
requires is_constructible_v<T, Args...> uniq_ptr<T> make_uniq(Args&&... args) {
  return uniq_ptr<T>{new T(forward<Args>(args)...)};
}

template <typename T>
requires has_extent_v<T> && is_default_constructible_v<remove_extent_t<T>>
    uniq_ptr<T> make_uniq(size_t size) {
  return uniq_ptr<T>{new remove_extent_t<T>[size]};
}
extern "C" void* malloc(size_t)
__THROW __attribute_malloc__;
extern "C" void free(void*) __THROW;

template <typename T>
void destruct(T* ptr) {
  size_t* real = reinterpret_cast<size_t*>(ptr) - 1;
  size_t n = *real;
  for (size_t i = 0; i < n; ++i) {
    ptr[i].~T();
  }
  free(reinterpret_cast<void*>(real));
}

// template <typename T, typename... Args>
// requires has_extent_v<T> && (same_as<remove_extent_t<T>,
// remove_reference<Args>> && ...) uniq_ptr<T,
// decltype(&destruct<remove_extent_t<T>>)> make_uniq(Args&&... args) {
//   size_t size = sizeof...(Args);
//   size_t* allocated = reinterpret_cast<size_t*>(
//       malloc(size * sizeof(remove_extent_t<T>) + sizeof(size_t)));
//   allocated[0] = size;
//   remove_extent_t<T>* ptr =
//       reinterpret_cast<remove_extent_t<T>*>(allocated + 1);

//   for (size_t i = 0; i < size; ++i)
//     new (&ptr[i]) remove_extent_t<T>(forward<Args>(get<args));
//   return uniq_ptr<T, decltype(&destruct<remove_extent_t<T>>)>{
//       ptr, &destruct<remove_extent_t<T>>};
// }

template <typename T, typename... Args>
requires has_extent_v<T> &&
    (same_as<remove_extent_t<T>, remove_reference_t<Args>>&&...)
        uniq_ptr<T> make_uniql(Args&&... args) {
  size_t size = sizeof...(Args);
  auto ptr = new remove_extent_t<T>[] { forward<Args>(args)... };
  return uniq_ptr<T>{ptr};
}

template <typename T, typename... Args>
requires has_extent_v<T> uniq_ptr<T, decltype(&destruct<remove_extent_t<T>>)>
make_uniq(size_t size, Args&&... args) {
  size_t* allocated = reinterpret_cast<size_t*>(
      malloc(size * sizeof(remove_extent_t<T>) + sizeof(size_t)));
  allocated[0] = size;
  remove_extent_t<T>* ptr =
      reinterpret_cast<remove_extent_t<T>*>(allocated + 1);

  for (size_t i = 0; i < size; ++i)
    new (&ptr[i]) remove_extent_t<T>(forward<Args>(args)...);
  return uniq_ptr<T, decltype(&destruct<remove_extent_t<T>>)>{
      ptr, &destruct<remove_extent_t<T>>};
}

template <typename T, typename Deleter, typename... Args>
requires is_constructible<T, Args...>::value uniq_ptr<T> make_uniqd(
    Deleter&& del, Args&&... args) {
  return {new T{forward<Args>(args)...}, forward<T>(del)};
}

template <typename T, typename Deleter>
requires has_extent_v<T> && is_default_constructible_v<remove_extent_t<T>>
    uniq_ptr<T> make_uniqd(Deleter&& del, size_t size) {
  return {new remove_extent_t<T>[size], forward<Deleter>(del)};
}

template <typename T, typename Deleter, typename Allocator, typename... Args>
requires is_constructible<T, Args...>::value uniq_ptr<T> make_uniqda(
    Deleter&& del, Allocator&& allocator, Args&&... args) {
  return {allocator(forward<Args>(args)...), forward<T>(del)};
}

template <typename T, typename Deleter, typename Allocator>
requires has_extent_v<T> && is_default_constructible_v<remove_extent_t<T>>
    uniq_ptr<T> make_uniqda(Deleter&& del, Allocator&& allocator, size_t size) {
  return {allocator(size), forward<Deleter>(del)};
}

#endif  // SMARTP_HPP
