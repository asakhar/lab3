#ifndef SMARTP_HPP
#define SMARTP_HPP

#include <compare>
#include <cstddef>
#include <stdexcept>
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
    return *this;
  }
  void reset(pointer ptr = nullptr) {
    invoke(m_del, this->m_ptr);
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
    return *this;
  }
  void reset(pointer ptr = nullptr) {
    invoke(m_del, this->m_ptr);
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
    return *this;
  }
  void reset(pointer ptr = nullptr) {
    invoke(m_del, this->m_ptr);
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

template <typename T, typename... Args>
requires has_extent_v<T> &&
    (same_as<remove_extent_t<T>, remove_reference_t<Args>>&&...)
        uniq_ptr<T> make_uniql(Args&&... args) {
  // size_t size = sizeof...(Args);
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

template <character_type T>
inline size_t stringlen(T const* str) {
  T const* end = str;
  while (*end) ++end;
  return end - str;
}

template <typename T>
class vector;
extern "C" void* memcpy(void* __restrict, void const* __restrict, size_t)
__THROW __nonnull((1, 2));

template <typename T>
class array {
 public:
  constexpr array() noexcept : m_capacity{0} {}
  explicit array(size_t capacity) noexcept
      : m_data{make_uniq<T[]>(capacity)}, m_capacity{capacity} {}
  array(T const* copy, size_t capacity) noexcept
      : m_data{make_uniq<T[]>(capacity)}, m_capacity{capacity} {
    for (size_t i = 0; i < capacity; ++i) m_data[i] = copy[i];
  }
  array(array const& copy)
      : m_data{make_uniq<T[]>(copy.capacity())}, m_capacity{0} {
    for (auto const& item : copy) {
      m_data[m_capacity++] = item;
    }
  }
  array(array&& move)
      : m_data{forward<uniq_ptr<T[]>>(move.m_data)},
        m_capacity{move.m_capacity} {
    move.m_capacity = 0;
  }
  array& operator=(array const& copy) {
    m_data = make_uniq<T[]>(copy.capacity());
    m_capacity = 0;
    for (auto const& item : copy) {
      m_data[m_capacity++] = item;
    }
    return *this;
  }
  array& operator=(array&& move) {
    m_data = forward<decltype(move.m_data)>(move.m_data);
    m_capacity = move.m_capacity;
    move.m_capacity = 0;
    return *this;
  }
  // template <character_type D>
  explicit array(T const* copy) noexcept : m_data{}, m_capacity{stringlen(copy)} {
    m_data = make_uniq<T[]>(m_capacity + 1);
    for (size_t i = 0; i < m_capacity; ++i) m_data[i] = copy[i];
    // memcpy(m_data.get(), copy, m_capacity * sizeof(T));
    m_data[m_capacity] = static_cast<T>('\0');
  }

  T& operator[](size_t idx) noexcept { return m_data[idx]; }
  T const& operator[](size_t idx) const noexcept { return m_data[idx]; }
  T& at(ssize_t idx) {
    if (idx < 0 || idx >= m_capacity)
      throw std::out_of_range("Array index is out of range");
    return m_data[idx];
  }
  T at(ssize_t idx) const {
    if (idx < 0 || idx >= m_capacity)
      throw std::out_of_range("Array index is out of range");
    return m_data[idx];
  }
  size_t capacity() const { return m_capacity; }
  T* data() { return m_data.get(); }
  T const* data() const { return m_data.get(); }
  template <bool isConst>
  class iterator {
   public:
    iterator& operator++() {
      ++idx;
      return *this;
    }
    iterator operator++(int) const {
      iterator res = *this;
      return ++res;
    }
    iterator& operator--() {
      if (idx >= 0) --idx;
      return *this;
    }
    iterator operator--(int) const {
      iterator res = *this;
      return --res;
    }
    conditional_const_t<T, isConst>& operator[](ssize_t i) const {
      return bound[idx + i];
    }
    iterator operator+(ssize_t i) const { return iterator(bound, idx + i); }
    iterator operator-(ssize_t i) const { return iterator(bound, idx - i); }
    conditional_const_t<T, isConst>& operator*() const { return bound[idx]; }
    conditional_const_t<T, isConst>* operator->() const { return &bound[idx]; }
    std::partial_ordering operator<=>(iterator const& it) const {
      if (*it.bound != *bound) return std::partial_ordering::unordered;
      if (idx > it.idx) return std::partial_ordering::greater;
      if (idx < it.idx) return std::partial_ordering::less;
      return std::partial_ordering::equivalent;
    }
    bool operator!=(iterator const& it) const {
      return &bound != &it.bound || idx != it.idx;
    }
    friend class array;
    friend class vector<T>;

   private:
    iterator(conditional_const_t<array, isConst>& bnd, ssize_t i)
        : bound{bnd}, idx{i} {};
    conditional_const_t<array, isConst>& bound;
    ssize_t idx;
  };
  iterator<false> begin() { return iterator<false>{*this, 0}; }
  iterator<false> end() {
    return iterator<false>{*this, static_cast<ssize_t>(capacity())};
  }
  iterator<true> begin() const { return iterator<true>{*this, 0}; }
  iterator<true> end() const {
    return iterator<true>{*this, static_cast<ssize_t>(capacity())};
  }
  iterator<false> rbegin() {
    return iterator<false>{*this, static_cast<ssize_t>(capacity()) - 1};
  }
  iterator<false> rend() { return iterator<false>{*this, -1}; }
  iterator<true> rbegin() const {
    return iterator<true>{*this, static_cast<ssize_t>(capacity()) - 1};
  }
  iterator<true> rend() const { return iterator<true>{*this, -1}; }

 protected:
  uniq_ptr<T[]> m_data;
  size_t m_capacity;
};

template <typename T>
class vector : public array<T> {
 public:
  constexpr vector() noexcept : array<T>{}, m_size{0} {}
  vector(size_t capacity) noexcept : array<T>{capacity}, m_size{0} {}
  vector(T const* copy, size_t size) noexcept
      : array<T>{copy, size}, m_size{size} {}
  vector(vector const& copy) : array<T>{copy}, m_size{copy.m_size} {}
  vector(vector&& move)
      : array<T>{forward<remove_reference<vector>>(move)}, m_size{move.m_size} {
    move.m_size = 0;
  }
  vector& operator=(vector const& copy) {
    m_size = copy.m_size;
    *this = dynamic_cast<array<T> const&>(copy);
    return *this;
  }
  vector& operator=(vector&& move) {
    m_size = move.m_size;
    *this = forward<remove_reference<array<T>>>(move);
    move.m_size = 0;
    return *this;
  }
  size_t size() const { return m_size; }
  void append(T const& item) {
    if (size() == this->capacity()) {
      this->m_capacity = this->capacity() << 1;
      if (this->m_capacity == 0) this->m_capacity = 4;
      auto newdata = make_uniq<T[]>(this->m_capacity);
      m_size = 0;
      for (auto&& itemo : *this) newdata[m_size++] = forward<T>(itemo);
      this->m_data = forward<decltype(newdata)>(newdata);
    }
    this->m_data[m_size++] = item;
  }
  // template <typename D>
  // requires is_character_v<D> vector<D>(D const* copy)
  // noexcept : array<T>(copy) { m_size = this->m_capacity - 1; }
  typename array<T>::template iterator<false> begin() {
    return typename array<T>::template iterator<false>{*this, 0};
  }
  typename array<T>::template iterator<false> end() {
    return typename array<T>::template iterator<false>{
        *this, static_cast<ssize_t>(size())};
  }
  typename array<T>::template iterator<true> begin() const {
    return typename array<T>::template iterator<true>{*this, 0};
  }
  typename array<T>::template iterator<true> end() const {
    return typename array<T>::template iterator<true>{
        *this, static_cast<ssize_t>(size())};
  }
  typename array<T>::template iterator<false> rbegin() {
    return typename array<T>::template iterator<false>{
        *this, static_cast<ssize_t>(size()) - 1};
  }
  typename array<T>::template iterator<false> rend() {
    return typename array<T>::template iterator<false>{*this, -1};
  }
  typename array<T>::template iterator<true> rbegin() const {
    return typename array<T>::template iterator<true>{
        *this, static_cast<ssize_t>(size()) - 1};
  }
  typename array<T>::template iterator<true> rend() const {
    return typename array<T>::template iterator<true>{*this, -1};
  }
  virtual ~vector() = default;
 protected:
  size_t m_size;
};
#endif  // SMARTP_HPP
