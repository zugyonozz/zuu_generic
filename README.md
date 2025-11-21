# zuu::generic - Lightweight Variant Library

Fast, lightweight discriminated union untuk C++20+.

## 📁 Struktur File

```
include/
├── typelist.hpp   # Compile-time type list utilities
├── bytes.hpp      # Fixed-size byte array dengan bitwise ops  
├── composer.hpp   # Type punning union
├── endian.hpp     # Endian detection & conversion
└── generic.hpp    # Main variant container (depends on above)
```

## ⚡ Perbandingan dengan std::variant

| Feature | `zuu::generic` | `std::variant` |
|---------|---------------|----------------|
| Header-only | ✅ | ✅ |
| Zero allocation | ✅ | ✅ |
| constexpr | ✅ Full | ✅ Full |
| Exception on bad access | ✅ `std::bad_cast` | ✅ `std::bad_variant_access` |
| `valueless_by_exception` | ❌ Tidak perlu* | ✅ Ada |
| Index type | Auto-sized (1-4 bytes) | Fixed `size_t` |
| Trivial types only | ✅ Required | ❌ Any type |
| Visit overhead | Minimal (fold expr) | Minimal |

*Karena hanya mendukung trivially copyable types, tidak ada exception saat construct.

## 🚀 Quick Start

```cpp
#include "generic.hpp"
using namespace zuu;

// Definisi
generic<int, double, float> value(42);

// Check tipe
if (value.holds<int>()) { ... }

// Access (safe)
int i = value.get<int>();           // throws jika salah
int* p = value.get_if<int>();       // nullptr jika salah
int& r = value.get_unchecked<int>(); // UB jika salah (fast)

// Visit
auto result = value.visit([](auto& v) {
    return static_cast<double>(v);
});

// Overload pattern
value.visit_void(overload{
    [](int i)    { /* handle int */ },
    [](double d) { /* handle double */ },
    [](float f)  { /* handle float */ }
});

// Modify
value.emplace<double>(3.14);
value = 100;  // assign int
value.reset(); // valueless
```

## 📊 Memory Layout

```
┌─────────────────────────────────────┐
│  generic<int, double, float>        │
├─────────────────────────────────────┤
│  data_[8]  (max_size, aligned)      │
│  index_    (uint8_t, karena < 256)  │
├─────────────────────────────────────┤
│  Total: 9 bytes + padding = 16      │
└─────────────────────────────────────┘

vs std::variant: typically 16+ bytes
```

## 🔧 Optimizations

1. **Auto-sized index**: `uint8_t` untuk ≤255 types, `uint16_t` untuk ≤65535
2. **Aligned storage**: Automatic alignment untuk SIMD compatibility
3. **No virtual calls**: All dispatch via compile-time fold expressions
4. **Trivial operations**: Copy/move adalah bitwise copy
5. **Branch-free visit**: Uses short-circuit `||` fold

## 📋 API Reference

### `generic<Ts...>`

#### Constructors
- `generic()` - Default (valueless)
- `generic(const T&)` - From value (copy)
- `generic(T&&)` - From value (move)

#### Observers
- `has_value()` → `bool`
- `index()` → index type
- `holds<T>()` → `bool`

#### Access
- `get<T>()` → `T&` (throws on mismatch)
- `get_unchecked<T>()` → `T&` (UB on mismatch)
- `get_if<T>()` → `T*` (nullptr on mismatch)

#### Modifiers
- `emplace<T>(args...)` → `T&`
- `operator=(const T&)`
- `reset()` - Make valueless
- `swap(other)`

#### Visitation
- `visit(F)` → `R` (return value)
- `visit_void(F)` - Side effects only

#### Static Info
- `type_count` - Number of types
- `max_size` - Largest type size
- `max_align` - Largest alignment
- `storage_size()` - Actual storage bytes

### Endian Functions (`endian.hpp`)

#### Constants
- `is_little_endian` - `bool`, compile-time
- `is_big_endian` - `bool`, compile-time
- `native_endian` - `endian_t`, native byte order

#### Free Functions
- `byte_swap(T)` → `T` - Reverse bytes
- `to_little_endian(T)` → `T` - Native to LE
- `to_big_endian(T)` → `T` - Native to BE
- `from_little_endian(T)` → `T` - LE to native
- `from_big_endian(T)` → `T` - BE to native
- `to_endian(T, endian_t)` → `T` - Runtime conversion
- `hton(T)` → `T` - Host to network order
- `ntoh(T)` → `T` - Network to host order

### `composer<T>`

Type punning utility untuk konversi ke raw bytes.

```cpp
composer<int> c(42);
auto bytes = c.as_bytes();  // std::span<const uint8_t, 4>
```

### `bytes<N>`

Fixed-size byte array dengan operasi bitwise.

```cpp
bytes<4> a(0xFF00FF00u);
bytes<4> b(0x00FF00FFu);
auto c = a ^ b;           // XOR
c.set_bit(0);            // Set LSB
auto count = c.popcount(); // Count 1s
```

### `endian.hpp`

Endian detection dan conversion utilities.

```cpp
// Compile-time detection
static_assert(is_little_endian);  // atau is_big_endian

// Integer conversion
uint32_t val = 0x12345678;
auto le = to_little_endian(val);   // No-op pada LE system
auto be = to_big_endian(val);      // Swap bytes pada LE system
auto native = from_big_endian(be); // Convert back

// Network byte order
uint16_t port = 8080;
auto net_port = hton(port);  // Host to network
auto host_port = ntoh(net_port);  // Network to host

// Byte swap
auto swapped = byte_swap(val);  // Reverse all bytes

// Runtime selection
auto result = to_endian(val, endian_t::big);
```

**composer endian methods:**
```cpp
composer<uint32_t> c(0x12345678);
auto c_le = c.to_little_endian();   // Convert to LE
auto c_be = c.to_big_endian();      // Convert to BE
auto c_net = c.to_network();        // Same as BE
c.swap_bytes();                     // In-place swap
auto c_rev = c.reversed();          // Reverse raw bytes
```

**bytes endian methods:**
```cpp
bytes<4> b(0xAABBCCDD);
auto b_le = b.to_little_endian();   // Convert to LE
auto b_be = b.to_big_endian();      // Convert to BE
auto b_net = b.to_network();        // Same as BE
b.make_big_endian();                // In-place conversion

// Integer with endian
auto val = b.to_int<uint32_t>(endian_t::big);  // Parse as BE
auto b2 = bytes<4>::from_big_endian_int(0x1234);  // Create BE bytes
```

## ⚠️ Limitations

1. **Trivially copyable only** - No `std::string`, `std::vector`, dll
2. **No recursive types** - Tidak bisa self-referential
3. **No monostate** - Gunakan `reset()` untuk valueless

## 🔨 Build Requirements

- C++20 or later
- Tested: GCC 11+, Clang 14+, MSVC 19.29+
