#pragma once

/**
 * @file composer.hpp
 * @brief Type-punning union untuk konversi type ke raw bytes
 * @version 1.0.0
 * 
 * Menyediakan akses low-level ke representasi memori dari tipe apapun.
 * Dioptimasi untuk zero-overhead abstraction.
 */

#include "endian.hpp"
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

namespace zuu {

/**
 * @brief Union untuk type punning antara T dan raw bytes
 * @tparam T Tipe yang akan di-compose (harus trivially copyable untuk safety)
 * 
 * @note Menggunakan union untuk type punning yang well-defined di C++20
 * @note Semua operasi adalah constexpr dan noexcept
 * 
 * @example
 * ```cpp
 * composer<int> c(42);
 * auto bytes = c.as_bytes();  // std::span<const uint8_t, 4>
 * ```
 */
template <typename T>
requires std::is_trivially_copyable_v<T>
union composer {
private:
    T value_;
    uint8_t raw_[sizeof(T)];

public:
    // ============= Type Aliases =============
    using value_type = T;
    using byte_type = uint8_t;
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

    static constexpr size_type byte_size = sizeof(T);

    // ============= Constructors =============
    
    constexpr composer() noexcept : value_{} {}
    constexpr composer(const composer&) noexcept = default;
    constexpr composer(composer&&) noexcept = default;
    constexpr composer& operator=(const composer&) noexcept = default;
    constexpr composer& operator=(composer&&) noexcept = default;

    /** @brief Construct dari value (copy) */
    constexpr explicit composer(const T& value) noexcept : value_(value) {}
    
    /** @brief Construct dari value (move) */
    constexpr explicit composer(T&& value) noexcept : value_(std::move(value)) {}

    /** @brief Construct dari raw bytes */
    constexpr explicit composer(const uint8_t* data, size_type len) noexcept : value_{} {
        const size_type copy_len = len < byte_size ? len : byte_size;
        for (size_type i = 0; i < copy_len; ++i) raw_[i] = data[i];
    }

    // ============= Value Access =============
    
    [[nodiscard]] constexpr reference value() noexcept { return value_; }
    [[nodiscard]] constexpr const_reference value() const noexcept { return value_; }
    [[nodiscard]] constexpr reference operator*() noexcept { return value_; }
    [[nodiscard]] constexpr const_reference operator*() const noexcept { return value_; }
    [[nodiscard]] constexpr pointer operator->() noexcept { return &value_; }
    [[nodiscard]] constexpr const_pointer operator->() const noexcept { return &value_; }

    // ============= Byte Access =============
    
    [[nodiscard]] constexpr uint8_t* data() noexcept { return raw_; }
    [[nodiscard]] constexpr const uint8_t* data() const noexcept { return raw_; }
    [[nodiscard]] constexpr size_type size() const noexcept { return byte_size; }

    /** @brief Akses byte dengan bounds checking */
    [[nodiscard]] constexpr uint8_t& byte_at(size_type i) noexcept { 
        return raw_[i < byte_size ? i : byte_size - 1]; 
    }
    [[nodiscard]] constexpr uint8_t byte_at(size_type i) const noexcept { 
        return raw_[i < byte_size ? i : byte_size - 1]; 
    }

    // ============= Span Access (Zero-copy) =============
    
    [[nodiscard]] constexpr std::span<uint8_t, byte_size> as_bytes() noexcept { 
        return std::span<uint8_t, byte_size>(raw_); 
    }
    [[nodiscard]] constexpr std::span<const uint8_t, byte_size> as_bytes() const noexcept { 
        return std::span<const uint8_t, byte_size>(raw_); 
    }

    // ============= Iterators =============
    
    [[nodiscard]] constexpr uint8_t* begin() noexcept { return raw_; }
    [[nodiscard]] constexpr uint8_t* end() noexcept { return raw_ + byte_size; }
    [[nodiscard]] constexpr const uint8_t* begin() const noexcept { return raw_; }
    [[nodiscard]] constexpr const uint8_t* end() const noexcept { return raw_ + byte_size; }
    [[nodiscard]] constexpr const uint8_t* cbegin() const noexcept { return raw_; }
    [[nodiscard]] constexpr const uint8_t* cend() const noexcept { return raw_ + byte_size; }

    // ============= Comparison =============
    
    [[nodiscard]] constexpr bool operator==(const composer& o) const noexcept {
        return value_ == o.value_;
    }
    [[nodiscard]] constexpr auto operator<=>(const composer& o) const noexcept 
    requires std::three_way_comparable<T> {
        return value_ <=> o.value_;
    }

    // ============= Conversion =============
    
    [[nodiscard]] constexpr explicit operator T() const noexcept { return value_; }

    // ============= Endian Conversion =============

    /**
     * @brief Convert value ke little-endian byte order
     * @return composer dengan bytes dalam little-endian order
     * @note No-op pada little-endian systems
     */
    [[nodiscard]] constexpr composer to_little_endian() const noexcept 
    requires std::integral<T> {
        return composer(zuu::to_little_endian(value_));
    }

    /**
     * @brief Convert value ke big-endian byte order
     * @return composer dengan bytes dalam big-endian order
     * @note No-op pada big-endian systems
     */
    [[nodiscard]] constexpr composer to_big_endian() const noexcept 
    requires std::integral<T> {
        return composer(zuu::to_big_endian(value_));
    }

    /**
     * @brief Convert value ke network byte order (big-endian)
     * @return composer dengan bytes dalam network order
     */
    [[nodiscard]] constexpr composer to_network() const noexcept 
    requires std::integral<T> {
        return to_big_endian();
    }

    /**
     * @brief Interpret bytes sebagai little-endian dan convert ke native
     * @return composer dengan native byte order
     */
    [[nodiscard]] constexpr composer from_little_endian() const noexcept 
    requires std::integral<T> {
        return composer(zuu::from_little_endian(value_));
    }

    /**
     * @brief Interpret bytes sebagai big-endian dan convert ke native
     * @return composer dengan native byte order
     */
    [[nodiscard]] constexpr composer from_big_endian() const noexcept 
    requires std::integral<T> {
        return composer(zuu::from_big_endian(value_));
    }

    /**
     * @brief Interpret bytes sebagai network order dan convert ke native
     * @return composer dengan native byte order
     */
    [[nodiscard]] constexpr composer from_network() const noexcept 
    requires std::integral<T> {
        return from_big_endian();
    }

    /**
     * @brief Convert ke endianness tertentu (runtime selection)
     * @param target Target endianness
     * @return composer dengan bytes dalam target order
     */
    [[nodiscard]] constexpr composer to_endian(endian_t target) const noexcept 
    requires std::integral<T> {
        return composer(zuu::to_endian(value_, target));
    }

    /**
     * @brief Swap byte order (reverse bytes)
     * @return composer dengan bytes terbalik
     */
    [[nodiscard]] constexpr composer byte_swapped() const noexcept 
    requires std::integral<T> {
        return composer(zuu::byte_swap(value_));
    }

    /**
     * @brief Swap byte order in-place
     */
    constexpr void swap_bytes() noexcept 
    requires std::integral<T> {
        value_ = zuu::byte_swap(value_);
    }

    // ============= Endian for Non-Integral (via raw bytes) =============

    /**
     * @brief Reverse raw bytes (for non-integral types)
     * @return composer dengan bytes terbalik
     */
    [[nodiscard]] constexpr composer reversed() const noexcept {
        composer result;
        for (size_type i = 0; i < byte_size; ++i) {
            result.raw_[i] = raw_[byte_size - 1 - i];
        }
        return result;
    }

    /**
     * @brief Reverse raw bytes in-place
     */
    constexpr void reverse() noexcept {
        for (size_type i = 0; i < byte_size / 2; ++i) {
            uint8_t temp = raw_[i];
            raw_[i] = raw_[byte_size - 1 - i];
            raw_[byte_size - 1 - i] = temp;
        }
    }
};

// Deduction guide
template <typename T>
composer(T) -> composer<T>;

} // namespace zuu
