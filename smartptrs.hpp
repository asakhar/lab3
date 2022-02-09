#ifndef SMARTPTRS_HPP
#define SMARTPTRS_HPP

#include <unistd.h>

#include <cstddef>
#include <type_traits>
#include <utility>

template <typename T>
class array {
 public:
  array(T* pointer, size_t n) : m_ptr{pointer}, m_size{n} {}
  template <typename... Args>
  array(size_t n, Args&&... args) {
    m_ptr = new T[n];
    m_size = n;
    for (size_t i = 0; i < n; ++i) {
      new (m_ptr+i) T(std::forward<Args>(args)...);
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
  void reset() {
    reset(nullptr, 0);
  }
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
  T const& operator[](size_t idx) const {
    return m_ptr[idx];
  }
  ~array() { reset(); }

 private:
  T* m_ptr = nullptr;
  size_t m_size = 0;
};

template <typename T>
class ptr {
 public:
  template<typename... Args>
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
