#ifndef SMARTPTRS_HPP
#define SMARTPTRS_HPP

#include <cstdlib>
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

template <typename T>
struct is_array_like {
  constexpr static bool value = false;
};

template <typename T>
struct is_array_like<T[]> {
  constexpr static bool value = true;
};

template <typename T>
constexpr bool is_array_like_v = is_array_like<T>::value;

template <typename T>
concept array_like = is_array_like_v<T>;

template <typename T>
concept not_array_like = !is_array_like_v<T>;

template <typename T>
struct remove_array {
  using type = T;
};

template <typename T>
struct remove_array<T[]> {
  using type = T;
};

template <typename T>
using remove_array_t = typename remove_array<T>::type;

template <typename T>
class default_deleter {
 public:
  using type = T;
  using ptr = T*;

  void free(ptr pointer) { delete pointer; }
};

template <typename T, typename... Args>
concept constructible = std::is_constructible_v<T, Args...>;

template <typename T>
concept default_constructible = std::is_default_constructible_v<T>;
template<typename T, typename D>
concept same_as_and_default_constructible = std::is_same_v<T, D> && std::is_default_constructible_v<D>; 
template<typename T, typename D>
concept same_as_and_not_default_constructible = std::is_same_v<T, D> && !std::is_default_constructible_v<D>; 

template <typename T>
class default_deleter<T[]> {
 public:
  using type = T;
  using ptr = T*;
  void free(ptr pointer) { delete[] pointer; }
};

template <typename T>
class default_alloc {
 public:
  using type = T;
  template <typename... Args>
  T* alloc(Args&&... args) {
    return new T{std::forward<Args>(args)...};
  }
};
template <typename T, typename D>
concept same_type_as =
    std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                   std::remove_cv_t<std::remove_reference_t<D>>>;

#include <concepts>


template<typename...Args, typename T>
concept constructible_from = std::is_constructible_v<T, Args...>;

template <typename T>
class default_alloc<T[]> {
 public:
  using type = T;
  type* alloc(size_t size) { return new type[size]; }
  template<constructible<type>...Args>
  type* alloc(size_t size, Args&&...args) {
    auto tmp = alloc(size);
    for(size_t i = 0; i < size; ++i)
      *(tmp+i) = type{std::forward<Args>(args)...};
    return tmp;
  }
};


template <typename T>
concept deleter = requires(T del, typename T::ptr pointer) {
  del.free(pointer);
};
template <typename T>
concept allocator = requires(T allocator) {
  allocator.alloc();
}
|| requires(T allocator) { allocator.alloc(1ul); };

template <typename T, deleter Deleter, allocator Alloc>
class ptr_traits {
 public:
  ptr_traits(Deleter&& del, Alloc&& alloc)
      : m_ptr{nullptr},
        m_deleter(std::forward<Deleter>(del)),
        m_alloc(std::forward<Alloc>(alloc)) {};
  using type = remove_array_t<T>;
  using ptr = type*;
  using ref = type&;
  void reset() {
    m_deleter.free(m_ptr);
    m_ptr = nullptr;
  }
  bool isnull() const { return m_ptr == nullptr; }
  ptr get() const { return m_ptr; }
  ptr operator->() const { return m_ptr; }
  ref operator*() const { return *m_ptr; }
  virtual ~ptr_traits() { reset(); }

 protected:
  ptr m_ptr;
  Deleter m_deleter;
  Alloc m_alloc;
};

template <typename T, deleter Deleter,
          allocator Alloc>
class sptr : public ptr_traits<T, Deleter, Alloc> {
 public:
  using base = ptr_traits<T, Deleter, Alloc>;
  using ptr = typename base::ptr;
  using type = typename base::type;
  using ref = typename base::ref;
  sptr(Deleter&& del = default_deleter<T>{}, Alloc&& alloc = default_alloc<T>{})
      : base(del, alloc) {}
  sptr(ptr pointer, Deleter&& del = default_deleter<T>{},
       Alloc&& alloc = default_alloc<T>{})
      : base(del, alloc) {
    this->m_ptr = pointer;
  }
  sptr(sptr const&) = delete;
  sptr& operator=(sptr const&) = delete;
  sptr(sptr&& other, Deleter&& del = default_deleter<T>{},
       Alloc&& alloc = default_alloc<T>{})
      : base(del, alloc) {
    this->m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
  }
  ptr& operator=(ptr&& other) {
    this->reset();
    this->m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
  }
  template <typename... Args>
  sptr(Args&&... args, Deleter&& del = default_deleter<T>{},
       Alloc&& alloc = default_alloc<T>{})
      : base(del, alloc) {
    this->m_ptr = new T(std::forward<Args>(args)...);
  }
  void reset(ptr pointer) {
    this->m_deleter.free(this->m_ptr);
    this->m_ptr = pointer;
  }
  ptr release() {
    auto tmp = this->m_ptr;
    this->m_ptr = nullptr;
    return tmp;
  }
};

template <typename T, deleter Deleter, allocator Alloc>
class sptr<T[], Deleter, Alloc> : public ptr_traits<T[], Deleter, Alloc> {
 public:
  using base = ptr_traits<T[], Deleter, Alloc>;
  using ptr = typename base::ptr;
  using type = typename base::type;
  using ref = typename base::ref;
  using default_del = default_deleter<type[]>;
  using default_all = default_alloc<type[]>;
  sptr(Deleter&& del = default_del{}, Alloc&& alloc = default_all{})
      : ptr_traits<T[], Deleter, Alloc>(std::forward<Deleter>(del), std::forward<Alloc>(alloc)), m_size{0} {}
  sptr(ptr pointer, size_t size, Deleter&& del = default_del{},
       Alloc&& alloc = default_all{})
      : base(std::forward<Deleter>(del), std::forward<Alloc>(alloc)), m_size{size} {
    this->m_ptr = pointer;
  }
  sptr(sptr const&) = delete;
  sptr& operator=(sptr const&) = delete;
  sptr(sptr&& other, Deleter&& del = default_del{},
       Alloc&& alloc = default_all{})
      : base(std::forward<Deleter>(del), std::forward<Alloc>(alloc)) {
    this->m_ptr = other.m_ptr;
    m_size = other.m_size;
    other.m_size = 0;
    other.m_ptr = nullptr;
  }
  ptr& operator=(ptr&& other) {
    this->reset();
    this->m_ptr = other.m_ptr;
    m_size = other.m_size;
    other.m_ptr = nullptr;
    other.m_size = 0;
  }
  sptr(size_t size, Deleter&& del = default_del{},
       Alloc&& alloc = default_all{})
      : base(std::forward<Deleter>(del), std::forward<Alloc>(alloc)), m_size{size} {
    this->m_ptr = alloc.alloc(size);
  }
  void reset(ptr pointer, size_t size) {
    this->m_deleter.free(this->m_ptr);
    this->m_ptr = pointer;
    m_size = size;
  }
  std::pair<ptr, size_t> release() {
    std::pair<ptr, size_t> tmp{this->m_ptr, m_size};
    this->m_ptr = nullptr;
    return tmp;
  }
  size_t size() const { return m_size; }
  T& operator[](size_t idx) const { return this->m_ptr[idx]; }
  template <bool isConst>
  class iterator {
   public:
    iterator& operator++() {
      if (idx < static_cast<ssize_t>(bound.size())) ++idx;
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
    iterator operator+(ssize_t i) const {
      return iterator(bound,
                      idx + i);
    }
    iterator operator-(ssize_t i) const {
      return iterator(bound,
                      idx - i);
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
    friend class sptr;
   private:
    iterator(conditional_const_t<sptr, isConst>& bnd, ssize_t i)
        : bound{bnd}, idx{i} {};
    conditional_const_t<sptr, isConst>& bound;
    ssize_t idx;
  };
  iterator<false> begin() { return iterator<false>{*this, 0}; }
  iterator<false> end() { return iterator<false>{*this, static_cast<ssize_t>(size())}; }
  iterator<true> begin() const { return iterator<true>{*this, 0}; }
  iterator<true> end() const { return iterator<true>{*this, static_cast<ssize_t>(size())}; }
  iterator<false> rbegin() { return iterator<false>{*this, static_cast<ssize_t>(size())-1}; }
  iterator<false> rend() { return iterator<false>{*this, -1}; }
  iterator<true> rbegin() const { return iterator<true>{*this, static_cast<ssize_t>(size())-1}; }
  iterator<true> rend() const { return iterator<true>{*this, -1}; }

 private:
  size_t m_size;
};



  template <typename T, deleter Deleter = default_deleter<T>, allocator Alloc = default_alloc<T>, typename... Args>
  auto make_smart(size_t size, Deleter&& del = default_deleter<T>{},
       Alloc&& alloc = default_alloc<T>{}) {
    return sptr<T, Deleter, Alloc>{size, std::forward<Deleter>(del), std::forward<Alloc>(alloc)};
  }
  // template <typename T, deleter Deleter, allocator Alloc, typename... Args>
  // auto make_smart_v(Args&& ...args, Deleter&& del = default_deleter<T>{},
  //      Alloc&& alloc = default_alloc<T>{}) {
  //   return sptr<T, Deleter, Alloc>{alloc.alloc(std::forward<Args>(args)...), std::forward<Deleter>(del), std::forward<Alloc>(alloc)};
  // }
  

template <typename T, deleter Deleter = default_deleter<T>,
          allocator Alloc = default_alloc<T>>
using smart_ptr = sptr<T, Deleter, Alloc>;

#endif  // SMARTPTRS_HPP
