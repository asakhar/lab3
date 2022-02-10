#ifndef TYPE_TRAITS_HPP
#define TYPE_TRAITS_HPP

#include <utility>
struct false_t {
  static constexpr bool value = false;
};
struct true_t {
  static constexpr bool value = true;
};
template <bool Const>
struct constant_t {
  static constexpr bool value = Const;
};
template <bool Const>
static constexpr bool not_v = !Const;

template <typename...>
using void_t = void;

template <typename T>
struct remove_rvalue_reference {
  using type = T;
};

template <typename T>
struct remove_rvalue_reference<T&&> {
  using type = T;
};
template <typename T>
using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;

template <typename T>
struct remove_extent {
  using type = T;
};
template <typename T>
struct remove_extent<T[]> {
  using type = T;
};
template <typename T, unsigned long long SIZE>
struct remove_extent<T[SIZE]> {
  using type = T;
};
template <typename T>
using remove_extent_t = typename remove_extent<T>::type;
template <typename T>
struct remove_reference {
  using type = T;
};

template<typename T, typename D>
struct is_same : public false_t {};

template<typename T>
struct is_same<T, T> : public true_t {};

template<typename T, typename D>
constexpr bool is_same_v = is_same<T, D>::value;

template<typename T, typename D>
concept same_as = is_same_v<T, D>;

template <typename T>
struct remove_reference<T&> {
  using type = T;
};
template <typename T>
struct remove_reference<T const&> {
  using type = T;
};
template <typename T>
struct remove_reference<T&&> {
  using type = T;
};
template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

template <typename T>
struct add_rvalue_reference {
  using type = remove_reference_t<T>&&;
};
template <typename T>
struct add_lvalue_reference {
  using type = remove_reference_t<T>&;
};

template <typename T>
using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

template <typename T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

template <typename T>
add_rvalue_reference_t<T> declval() noexcept;

template <typename T>
add_lvalue_reference_t<T> declvar() noexcept;

template <typename T>
T consume(T) noexcept;

template <typename, typename T, typename... Args>
struct is_constructible : public false_t {};

template <typename T, typename... Args>
struct is_constructible<void_t<decltype(T(declval<Args>()...))>, T, Args...>
    : public true_t {};

template <typename T, typename... Args>
constexpr bool is_constructible_v =
    is_constructible<void_t<>, T, Args...>::value;

template<typename T>
struct has_extent : public false_t {};

template<typename T>
struct has_extent <T[]> : public true_t {};

template<typename T, unsigned long long SIZE>
struct has_extent <T[SIZE]> : public true_t {};

template<typename T>
constexpr bool has_extent_v = has_extent<T>::value;

template <typename T>
constexpr bool is_default_constructible_v = is_constructible_v<T>;

template <typename T>
constexpr bool is_copy_constructible_v = is_constructible_v<T, T const&>;

template <typename T>
constexpr bool is_move_constructible_v = is_constructible_v<T, T&&>;

template <typename, typename T, typename D>
struct is_convertible : public false_t {};

template <typename T, typename D>
struct is_convertible<void_t<decltype(consume<D>(declval<T>()))>, T, D>
    : public true_t {};

template <typename T, typename D>
constexpr bool is_convertible_v = is_convertible<void_t<>, T, D>::value;

template <typename T, typename D>
concept convertible = is_convertible_v<D, T>;

template <typename, typename T, typename D>
struct is_assignable : public constant_t<is_convertible_v<D, T>> {};

template <typename T, typename D>
struct is_assignable<void_t<decltype(declvar<T>() = declval<D>())>, T, D>
    : public true_t {};

template <typename T, typename D>
constexpr bool is_assignable_v = is_assignable<void_t<>, T, D>::value;

template <typename T>
concept default_constructible = is_default_constructible_v<T>;

template <typename T>
constexpr T&& forward(remove_reference_t<T>& t) {
  return static_cast<T&&>(t);
}

template <typename T>
constexpr T&& forward(remove_reference_t<T>&& t) {
  return static_cast<T&&>(t);
}

using nullptr_t = decltype(nullptr);

#endif  // TYPE_TRAITS_HPP