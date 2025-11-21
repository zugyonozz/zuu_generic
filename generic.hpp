#pragma once

/**
 * @file generic.hpp
 * @brief Lightweight variant container dengan fokus performa
 * @version 1.0.0
 * 
 * Alternatif ringan untuk std::variant dengan fitur:
 * - Zero dynamic allocation
 * - Compile-time type safety
 * - Optimized visit pattern
 * - Small footprint (size + max alignment)
 * 
 * @note Semua tipe harus trivially copyable untuk performa optimal
 */

#include "typelist.hpp"
#include "composer.hpp"
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace zuu {

// ============= Forward Declarations =============

template <typename... Ts>
requires (sizeof...(Ts) > 0)
class generic;

// ============= Traits =============

namespace detail {

/** @brief Index type berdasarkan jumlah tipe */
template <size_t N>
using index_type = std::conditional_t<N <= 255, uint8_t,
                   std::conditional_t<N <= 65535, uint16_t, uint32_t>>;

/** @brief Invalid index sentinel */
template <typename IndexT>
inline constexpr IndexT npos = static_cast<IndexT>(-1);

/** @brief Check apakah semua tipe trivially copyable */
template <typename... Ts>
inline constexpr bool all_trivial_v = (std::is_trivially_copyable_v<Ts> && ...);

/** @brief Check apakah semua tipe nothrow move constructible */
template <typename... Ts>
inline constexpr bool all_nothrow_move_v = (std::is_nothrow_move_constructible_v<Ts> && ...);

} // namespace detail

// ============= Overload Helper =============

/**
 * @brief Helper untuk visitor overload pattern
 * @example
 * ```cpp
 * g.visit(overload{
 *     [](int i) { ... },
 *     [](double d) { ... }
 * });
 * ```
 */
template <typename... Fs>
struct overload : Fs... {
    using Fs::operator()...;
};
template <typename... Fs> overload(Fs...) -> overload<Fs...>;

// ============= Generic Class =============

/**
 * @brief Lightweight discriminated union (variant)
 * @tparam Ts Tipe-tipe yang dapat disimpan (min 1, semua harus trivially copyable)
 * 
 * Memory layout:
 * - index_: 1-4 bytes (tergantung jumlah tipe)
 * - data_: max(sizeof(Ts)...) bytes, aligned ke max(alignof(Ts)...)
 * 
 * @note Tidak ada dynamic allocation
 * @note Semua operasi noexcept jika tipe mendukung
 */
template <typename... Ts>
requires (sizeof...(Ts) > 0)
class generic {
    static_assert(detail::all_trivial_v<Ts...>, 
        "All types must be trivially copyable for optimal performance");

public:
    // ============= Type Aliases =============
    using list_t = type_list_t<Ts...>;
    using index_type = detail::index_type<sizeof...(Ts)>;
    
    static constexpr size_t type_count = sizeof...(Ts);
    static constexpr size_t max_size = list_t::max_size;
    static constexpr size_t max_align = list_t::max_align;
    static constexpr index_type npos = detail::npos<index_type>;

private:
    // Storage dengan alignment yang benar
    alignas(max_align) uint8_t data_[max_size]{};
    index_type index_ = npos;

    // ============= Internal Helpers =============

    template <typename T>
    static constexpr index_type index_of_v = static_cast<index_type>(list_t::template index_of<T>);

    /** @brief Copy data dari value ke storage */
    template <typename T>
    constexpr void store(const T& value) noexcept {
        std::memcpy(data_, &value, sizeof(T));
    }

    /** @brief Get pointer ke stored value */
    template <typename T>
    [[nodiscard]] constexpr T* ptr() noexcept {
        return std::launder(reinterpret_cast<T*>(data_));
    }

    template <typename T>
    [[nodiscard]] constexpr const T* ptr() const noexcept {
        return std::launder(reinterpret_cast<const T*>(data_));
    }

    // ============= Visit Implementation =============

    template <typename R, typename F, size_t... Is>
    [[nodiscard]] constexpr R visit_impl(F&& f, std::index_sequence<Is...>) {
        R result{};
        ((index_ == Is ? (result = std::forward<F>(f)(*ptr<typename list_t::template type<Is>>()), true) 
                       : false) || ...);
        return result;
    }

    template <typename R, typename F, size_t... Is>
    [[nodiscard]] constexpr R visit_impl(F&& f, std::index_sequence<Is...>) const {
        R result{};
        ((index_ == Is ? (result = std::forward<F>(f)(*ptr<typename list_t::template type<Is>>()), true) 
                       : false) || ...);
        return result;
    }

    template <typename F, size_t... Is>
    constexpr void visit_void_impl(F&& f, std::index_sequence<Is...>) {
        ((index_ == Is ? (std::forward<F>(f)(*ptr<typename list_t::template type<Is>>()), true) 
                       : false) || ...);
    }

    template <typename F, size_t... Is>
    constexpr void visit_void_impl(F&& f, std::index_sequence<Is...>) const {
        ((index_ == Is ? (std::forward<F>(f)(*ptr<typename list_t::template type<Is>>()), true) 
                       : false) || ...);
    }

public:
    // ============= Constructors =============

    /** @brief Default: valueless state */
    constexpr generic() noexcept = default;
    constexpr generic(const generic&) noexcept = default;
    constexpr generic(generic&&) noexcept = default;
    constexpr generic& operator=(const generic&) noexcept = default;
    constexpr generic& operator=(generic&&) noexcept = default;

    /** @brief Construct dari value */
    template <typename T>
    requires (list_t::template contains<T>)
    constexpr generic(const T& value) noexcept 
        : index_(index_of_v<T>) {
        store(value);
    }

    /** @brief Construct dari value (move) */
    template <typename T>
    requires (list_t::template contains<T>)
    constexpr generic(T&& value) noexcept 
        : index_(index_of_v<std::decay_t<T>>) {
        store(std::forward<T>(value));
    }

    // ============= Modifiers =============

    /** @brief In-place construct tipe T */
    template <typename T, typename... Args>
    requires (list_t::template contains<T> && std::is_constructible_v<T, Args...>)
    constexpr T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        T temp(std::forward<Args>(args)...);
        store(temp);
        index_ = index_of_v<T>;
        return *ptr<T>();
    }

    /** @brief Assign value baru */
    template <typename T>
    requires (list_t::template contains<T>)
    constexpr generic& operator=(const T& value) noexcept {
        store(value);
        index_ = index_of_v<T>;
        return *this;
    }

    /** @brief Reset ke valueless state */
    constexpr void reset() noexcept {
        index_ = npos;
    }

    /** @brief Swap dengan generic lain */
    constexpr void swap(generic& other) noexcept {
        generic temp = *this;
        *this = other;
        other = temp;
    }

    // ============= Observers =============

    /** @brief Cek apakah memiliki value */
    [[nodiscard]] constexpr bool has_value() const noexcept { return index_ != npos; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    /** @brief Get current type index */
    [[nodiscard]] constexpr index_type index() const noexcept { return index_; }

    /** @brief Cek apakah menyimpan tipe T */
    template <typename T>
    requires (list_t::template contains<T>)
    [[nodiscard]] constexpr bool holds() const noexcept {
        return index_ == index_of_v<T>;
    }

    // ============= Access =============

    /** @brief Get reference (throws jika tipe salah) */
    template <typename T>
    requires (list_t::template contains<T>)
    [[nodiscard]] constexpr T& get() {
        if (index_ != index_of_v<T>) throw std::bad_cast();
        return *ptr<T>();
    }

    template <typename T>
    requires (list_t::template contains<T>)
    [[nodiscard]] constexpr const T& get() const {
        if (index_ != index_of_v<T>) throw std::bad_cast();
        return *ptr<T>();
    }

    /** @brief Get tanpa check (UB jika salah, tapi fast) */
    template <typename T>
    requires (list_t::template contains<T>)
    [[nodiscard]] constexpr T& get_unchecked() noexcept { return *ptr<T>(); }

    template <typename T>
    requires (list_t::template contains<T>)
    [[nodiscard]] constexpr const T& get_unchecked() const noexcept { return *ptr<T>(); }

    /** @brief Get pointer (nullptr jika tipe salah) */
    template <typename T>
    requires (list_t::template contains<T>)
    [[nodiscard]] constexpr T* get_if() noexcept {
        return index_ == index_of_v<T> ? ptr<T>() : nullptr;
    }

    template <typename T>
    requires (list_t::template contains<T>)
    [[nodiscard]] constexpr const T* get_if() const noexcept {
        return index_ == index_of_v<T> ? ptr<T>() : nullptr;
    }

    // ============= Visitation =============

    /** @brief Visit dengan return value */
    template <typename F>
    [[nodiscard]] constexpr auto visit(F&& f) {
        using R = std::common_type_t<decltype(f(std::declval<Ts&>()))...>;
        return visit_impl<R>(std::forward<F>(f), std::make_index_sequence<type_count>{});
    }

    template <typename F>
    [[nodiscard]] constexpr auto visit(F&& f) const {
        using R = std::common_type_t<decltype(f(std::declval<const Ts&>()))...>;
        return visit_impl<R>(std::forward<F>(f), std::make_index_sequence<type_count>{});
    }

    /** @brief Visit tanpa return value (untuk side effects) */
    template <typename F>
    constexpr void visit_void(F&& f) {
        visit_void_impl(std::forward<F>(f), std::make_index_sequence<type_count>{});
    }

    template <typename F>
    constexpr void visit_void(F&& f) const {
        visit_void_impl(std::forward<F>(f), std::make_index_sequence<type_count>{});
    }

    // ============= Comparison =============

    [[nodiscard]] constexpr bool operator==(const generic& o) const noexcept {
        if (index_ != o.index_) return false;
        if (index_ == npos) return true;
        return std::memcmp(data_, o.data_, max_size) == 0;
    }

    // ============= Raw Access =============

    [[nodiscard]] constexpr const uint8_t* data() const noexcept { return data_; }
    [[nodiscard]] constexpr uint8_t* data() noexcept { return data_; }
    [[nodiscard]] static constexpr size_t storage_size() noexcept { return max_size; }
};

// ============= Deduction Guides =============

template <typename T>
generic(T) -> generic<T>;

// ============= Helper Functions =============

/** @brief Factory function dengan auto deduction */
template <typename T>
[[nodiscard]] constexpr auto make_generic(T&& value) noexcept {
    return generic<std::decay_t<T>>(std::forward<T>(value));
}

/** @brief Free function swap */
template <typename... Ts>
constexpr void swap(generic<Ts...>& a, generic<Ts...>& b) noexcept {
    a.swap(b);
}

} // namespace zuu
