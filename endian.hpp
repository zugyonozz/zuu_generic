#pragma once

/**
 * @file endian.hpp
 * @brief Endian detection dan byte swapping utilities
 * @version 1.0.0
 * 
 * Menyediakan:
 * - Compile-time endian detection
 * - Byte swap operations (optimized intrinsics)
 * - Endian conversion functions
 * 
 * @note Semua operasi constexpr dan noexcept
 * @note Zero overhead untuk native endian conversions
 */

#include <bit>
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace zuu {

// ============= Endian Type =============

/**
 * @brief Endian types (mirrors std::endian)
 */
enum class endian_t {
    little = static_cast<int>(std::endian::little),
    big    = static_cast<int>(std::endian::big),
    native = static_cast<int>(std::endian::native)
};

// ============= Compile-time Constants =============

/** @brief True jika sistem little-endian */
inline constexpr bool is_little_endian = (std::endian::native == std::endian::little);

/** @brief True jika sistem big-endian */
inline constexpr bool is_big_endian = (std::endian::native == std::endian::big);

/** @brief Native endianness */
inline constexpr endian_t native_endian = is_little_endian ? endian_t::little : endian_t::big;

// ============= Byte Swap Primitives =============

namespace detail {

/** @brief Swap bytes of 16-bit integer */
[[nodiscard]] constexpr uint16_t bswap16(uint16_t v) noexcept {
    return static_cast<uint16_t>((v << 8) | (v >> 8));
}

/** @brief Swap bytes of 32-bit integer */
[[nodiscard]] constexpr uint32_t bswap32(uint32_t v) noexcept {
    return ((v & 0xFF000000u) >> 24) |
           ((v & 0x00FF0000u) >> 8)  |
           ((v & 0x0000FF00u) << 8)  |
           ((v & 0x000000FFu) << 24);
}

/** @brief Swap bytes of 64-bit integer */
[[nodiscard]] constexpr uint64_t bswap64(uint64_t v) noexcept {
    return ((v & 0xFF00000000000000ull) >> 56) |
           ((v & 0x00FF000000000000ull) >> 40) |
           ((v & 0x0000FF0000000000ull) >> 24) |
           ((v & 0x000000FF00000000ull) >> 8)  |
           ((v & 0x00000000FF000000ull) << 8)  |
           ((v & 0x0000000000FF0000ull) << 24) |
           ((v & 0x000000000000FF00ull) << 40) |
           ((v & 0x00000000000000FFull) << 56);
}

} // namespace detail

// ============= Generic Byte Swap =============

/**
 * @brief Swap bytes dari tipe integral apapun
 * @tparam T Tipe integral
 * @param v Value untuk di-swap
 * @return Value dengan byte order terbalik
 */
template <std::integral T>
[[nodiscard]] constexpr T byte_swap(T v) noexcept {
    if constexpr (sizeof(T) == 1) {
        return v;
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(detail::bswap16(static_cast<uint16_t>(v)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(detail::bswap32(static_cast<uint32_t>(v)));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(detail::bswap64(static_cast<uint64_t>(v)));
    } else {
        // Fallback untuk ukuran lain
        T result{};
        auto* src = reinterpret_cast<const uint8_t*>(&v);
        auto* dst = reinterpret_cast<uint8_t*>(&result);
        for (size_t i = 0; i < sizeof(T); ++i) {
            dst[i] = src[sizeof(T) - 1 - i];
        }
        return result;
    }
}

// ============= Endian Conversion Functions =============

/**
 * @brief Convert native to little-endian
 * @note No-op pada little-endian systems
 */
template <std::integral T>
[[nodiscard]] constexpr T to_little_endian(T v) noexcept {
    if constexpr (is_little_endian) {
        return v;
    } else {
        return byte_swap(v);
    }
}

/**
 * @brief Convert native to big-endian
 * @note No-op pada big-endian systems
 */
template <std::integral T>
[[nodiscard]] constexpr T to_big_endian(T v) noexcept {
    if constexpr (is_big_endian) {
        return v;
    } else {
        return byte_swap(v);
    }
}

/**
 * @brief Convert little-endian to native
 * @note No-op pada little-endian systems
 */
template <std::integral T>
[[nodiscard]] constexpr T from_little_endian(T v) noexcept {
    return to_little_endian(v); // Symmetric operation
}

/**
 * @brief Convert big-endian to native
 * @note No-op pada big-endian systems
 */
template <std::integral T>
[[nodiscard]] constexpr T from_big_endian(T v) noexcept {
    return to_big_endian(v); // Symmetric operation
}

/**
 * @brief Convert antara dua endianness tertentu
 * @tparam From Source endianness
 * @tparam To Target endianness
 */
template <endian_t From, endian_t To, std::integral T>
[[nodiscard]] constexpr T convert_endian(T v) noexcept {
    if constexpr (From == To) {
        return v;
    } else {
        return byte_swap(v);
    }
}

/**
 * @brief Convert ke endianness tertentu (runtime)
 */
template <std::integral T>
[[nodiscard]] constexpr T to_endian(T v, endian_t target) noexcept {
    if (target == native_endian) {
        return v;
    } else {
        return byte_swap(v);
    }
}

/**
 * @brief Convert dari endianness tertentu ke native (runtime)
 */
template <std::integral T>
[[nodiscard]] constexpr T from_endian(T v, endian_t source) noexcept {
    return to_endian(v, source); // Symmetric
}

// ============= Byte Array Swap =============

/**
 * @brief Reverse byte array in-place
 * @param data Pointer ke data
 * @param size Ukuran dalam bytes
 */
constexpr void byte_swap_array(uint8_t* data, size_t size) noexcept {
    for (size_t i = 0; i < size / 2; ++i) {
        uint8_t temp = data[i];
        data[i] = data[size - 1 - i];
        data[size - 1 - i] = temp;
    }
}

/**
 * @brief Reverse byte array, return copy
 * @param data Source data
 * @param size Ukuran dalam bytes
 * @param out Output buffer (harus >= size)
 */
constexpr void byte_swap_array(const uint8_t* data, size_t size, uint8_t* out) noexcept {
    for (size_t i = 0; i < size; ++i) {
        out[i] = data[size - 1 - i];
    }
}

// ============= Network Byte Order (Big-Endian) =============

/** @brief Host to network byte order (16-bit) */
template <std::integral T>
requires (sizeof(T) == 2)
[[nodiscard]] constexpr T hton(T v) noexcept {
    return to_big_endian(v);
}

/** @brief Host to network byte order (32-bit) */
template <std::integral T>
requires (sizeof(T) == 4)
[[nodiscard]] constexpr T hton(T v) noexcept {
    return to_big_endian(v);
}

/** @brief Host to network byte order (64-bit) */
template <std::integral T>
requires (sizeof(T) == 8)
[[nodiscard]] constexpr T hton(T v) noexcept {
    return to_big_endian(v);
}

/** @brief Network to host byte order */
template <std::integral T>
[[nodiscard]] constexpr T ntoh(T v) noexcept {
    return from_big_endian(v);
}

// ============= Type Traits =============

/** @brief Check if type can be endian-swapped */
template <typename T>
concept endian_swappable = std::integral<T> || 
    (std::is_trivially_copyable_v<T> && (sizeof(T) == 1 || sizeof(T) == 2 || 
     sizeof(T) == 4 || sizeof(T) == 8));

} // namespace zuu
