#pragma once

/**
 * @file typelist.hpp
 * @brief Compile-time type list utilities
 * @version 1.1.0
 * 
 * Menyediakan metaprogramming utilities untuk manipulasi daftar tipe.
 * Semua operasi compile-time dengan zero runtime overhead.
 */

#include <cstddef>
#include <type_traits>

namespace zuu {

namespace detail {

// ============= Max Helper =============

template <typename T, typename U>
[[nodiscard]] constexpr auto max_val(T a, U b) noexcept {
    return a > b ? a : b;
}

template <typename T, typename... Ts>
[[nodiscard]] constexpr auto max_val(T a, Ts... rest) noexcept {
    if constexpr (sizeof...(rest) == 0) return a;
    else return max_val(a, max_val(rest...));
}

// ============= Type List Implementation =============

template <typename... Ts>
struct type_list {
    static constexpr size_t count = sizeof...(Ts);
};

// Type at index
template <size_t N, typename List>
struct type_at_impl;

template <size_t N, typename T, typename... Ts>
struct type_at_impl<N, type_list<T, Ts...>> {
    using type = typename type_at_impl<N - 1, type_list<Ts...>>::type;
};

template <typename T, typename... Ts>
struct type_at_impl<0, type_list<T, Ts...>> {
    using type = T;
};

// Index of type
template <typename T, typename List>
struct index_of_impl;

template <typename T>
struct index_of_impl<T, type_list<>> {
    static constexpr size_t value = static_cast<size_t>(-1);
};

template <typename T, typename... Ts>
struct index_of_impl<T, type_list<T, Ts...>> {
    static constexpr size_t value = 0;
};

template <typename T, typename U, typename... Us>
struct index_of_impl<T, type_list<U, Us...>> {
    static constexpr size_t value = 1 + index_of_impl<T, type_list<Us...>>::value;
};

// Contains type
template <typename T, typename List>
struct contains_impl;

template <typename T>
struct contains_impl<T, type_list<>> : std::false_type {};

template <typename T, typename U, typename... Us>
struct contains_impl<T, type_list<U, Us...>> 
    : std::bool_constant<std::is_same_v<T, U> || contains_impl<T, type_list<Us...>>::value> {};

} // namespace detail

// ============= Public Interface =============

/**
 * @brief Compile-time type list dengan query operations
 * @tparam Ts Daftar tipe
 * 
 * @example
 * ```cpp
 * using list = type_list_t<int, double, float>;
 * static_assert(list::count == 3);
 * static_assert(list::contains<int>);
 * static_assert(list::index_of<double> == 1);
 * using second = list::type<1>;  // double
 * ```
 */
template <typename... Ts>
struct type_list_t {
    // ============= Size Info =============
    
    /** @brief Jumlah tipe dalam list */
    static constexpr size_t count = sizeof...(Ts);
    
    /** @brief Total ukuran semua tipe (sum) */
    static constexpr size_t total_size = (0 + ... + sizeof(Ts));
    
    /** @brief Ukuran tipe terbesar */
    static constexpr size_t max_size = []() constexpr -> size_t {
        if constexpr (count == 0) return 0;
        else if constexpr (count == 1) return sizeof(typename detail::type_at_impl<0, detail::type_list<Ts...>>::type);
        else return detail::max_val(sizeof(Ts)...);
    }();
    
    /** @brief Alignment terbesar */
    static constexpr size_t max_align = []() constexpr -> size_t {
        if constexpr (count == 0) return 1;
        else if constexpr (count == 1) return alignof(typename detail::type_at_impl<0, detail::type_list<Ts...>>::type);
        else return detail::max_val(alignof(Ts)...);
    }();

    // ============= Type Queries =============
    
    /** @brief Get index dari tipe T (-1 jika tidak ada) */
    template <typename T>
    static constexpr size_t index_of = detail::index_of_impl<T, detail::type_list<Ts...>>::value;
    
    /** @brief Cek apakah T ada dalam list */
    template <typename T>
    static constexpr bool contains = detail::contains_impl<T, detail::type_list<Ts...>>::value;
    
    /** @brief Get tipe pada index N */
    template <size_t N>
    requires (N < count)
    using type = typename detail::type_at_impl<N, detail::type_list<Ts...>>::type;

    // ============= Type Traits =============
    
    /** @brief Cek apakah semua tipe trivially copyable */
    static constexpr bool all_trivial = (std::is_trivially_copyable_v<Ts> && ...);
    
    /** @brief Cek apakah semua tipe nothrow move constructible */
    static constexpr bool all_nothrow_move = (std::is_nothrow_move_constructible_v<Ts> && ...);
    
    /** @brief Cek apakah semua tipe nothrow default constructible */
    static constexpr bool all_nothrow_default = (std::is_nothrow_default_constructible_v<Ts> && ...);
};

// ============= Type Traits =============

/** @brief Check if type is a type_list_t */
template <typename T>
struct is_type_list : std::false_type {};

template <typename... Ts>
struct is_type_list<type_list_t<Ts...>> : std::true_type {};

template <typename T>
inline constexpr bool is_type_list_v = is_type_list<T>::value;

} // namespace zuu
