#ifndef SMARTPTRS_HPP
#define SMARTPTRS_HPP

#include <unistd.h>

#include <compare>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T, bool isConst = false>
struct conditional_const {
  using type = T;
};
template <typename T>
struct conditional_const<T, true> {
  using type = T const;
};
template <typename T, bool isConst>
using conditional_const_t = typename conditional_const<T, isConst>::type;

template<typename T>
class default_deleter {
  public:
   void free(T* ptr) {
     delete ptr;
   }
};

template <typename T>
class array {
 public:
  array(T* pointer = nullptr, size_t n = 0) : m_ptr{pointer}, m_size{n} {}
  array(T const* pointer, size_t n) : m_ptr{new T[n]}, m_size{n} {
    if constexpr (std::is_trivial_v<T>)
      memcpy(m_ptr, pointer, n * sizeof(T));
    else
      for (size_t i = 0; i < n; ++i) {
        m_ptr[i] = pointer[i];
      }
  }
  template <typename... Args>
  array(size_t n, Args&&... args) {
    m_ptr = new T[n];
    m_size = n;
    for (size_t i = 0; i < n; ++i) {
      new (m_ptr + i) T(std::forward<Args>(args)...);
    }
  }
  array(array const&) = delete;
  array& operator=(array const&) = delete;
  array(array&& other) : m_ptr{other.m_ptr}, m_size{other.m_size} {
    other.m_ptr = nullptr;
    other.m_size = 0;
  }
  array& operator=(array&& other) {
    reset();
    m_ptr = other.m_ptr;
    m_size = other.m_size;
    other.m_ptr = nullptr;
    other.m_size = 0;
    return *this;
  }
  void reset() { reset(nullptr, 0); }
  void reset(T* pointer, size_t n) {
    delete[] m_ptr;
    m_ptr = pointer;
    m_size = n;
  }
  T* release() {
    auto res = m_ptr;
    m_ptr = nullptr;
    return res;
  }
  size_t size() const { return m_size; }
  bool isnull() { return m_ptr == nullptr; }
  T* get() { return m_ptr; }
  T const* get() const { return m_ptr; }
  T& operator[](size_t idx) { return m_ptr[idx]; }
  T const& operator[](size_t idx) const { return m_ptr[idx]; }
  ~array() { reset(); }
  template <bool isConst>
  class iterator {
   public:
    iterator& operator++() {
      if (idx < bound.size()) ++idx;
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
      return bound[static_cast<ssize_t>(idx) + i];
    }
    iterator operator+(ssize_t i) const {
      return iterator(bound,
                      static_cast<size_t>(static_cast<ssize_t>(idx) + i));
    }
    iterator operator-(ssize_t i) const {
      return iterator(bound,
                      static_cast<size_t>(static_cast<ssize_t>(idx) - i));
    }
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
   private:
    iterator(conditional_const_t<array, isConst>& bnd, size_t i)
        : bound{bnd}, idx{i} {};
    conditional_const_t<array, isConst>& bound;
    size_t idx;
  };
  iterator<false> begin() { return iterator<false>{*this, 0}; }
  iterator<false> end() { return iterator<false>{*this, size()}; }
  iterator<true> begin() const { return iterator<true>{*this, 0}; }
  iterator<true> end() const { return iterator<true>{*this, size()}; }

 private:
  T* m_ptr = nullptr;
  size_t m_size = 0;
};

template <typename T>
class ptr {
 public:
  ptr() : m_ptr{nullptr} {}
  template <typename... Args>
  ptr(Args&&... args) : m_ptr(new T(std::forward<Args>(args)...)) {}
  ptr(T* pointer = nullptr) : m_ptr(pointer) {}
  ptr(ptr const&) = delete;
  ptr& operator=(ptr const&) = delete;
  ptr(ptr&& other) : m_ptr{other.m_ptr} { other.m_ptr = nullptr; }
  ptr& operator=(ptr&& other) {
    reset();
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
  }
  void reset(T* pointer = nullptr) {
    delete m_ptr;
    m_ptr = pointer;
  }
  T* release() {
    auto res = m_ptr;
    reset();
    return res;
  }
  bool isnull() { return m_ptr == nullptr; }
  T* get() { return m_ptr; }
  T const* get() const { return m_ptr; }
  T* operator->() { return m_ptr; }
  T const* operator->() const { return m_ptr; }
  T& operator*() { return *m_ptr; }
  T const& operator*() const { return *m_ptr; }
  ~ptr() { reset(); }

 private:
  T* m_ptr = nullptr;
};
#endif  // SMARTPTRS_HPP
