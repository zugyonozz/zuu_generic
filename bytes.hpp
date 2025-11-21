#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <bit>

/**
 * @brief Container untuk menyimpan array byte dengan ukuran tetap
 * @details Container compile-time yang mendukung operasi bitwise dengan performa tinggi.
 *          Semua operasi constexpr dan dapat dijalankan saat compile-time.
 * 
 * Fitur utama:
 * - Operasi bitwise: AND, OR, XOR, NOT, shift
 * - Konversi ke/dari integer
 * - Manipulasi bit individual
 * - Operasi rotasi
 * - Format output binary dan hexadecimal
 * 
 * @tparam N Jumlah byte (harus > 0)
 */
template <size_t N = 1>
class bytes {
private:
	using byte_t = unsigned char ;
	using index_t = size_t ;

	alignas(16) byte_t data_[N]{} ;

public:
	static_assert(N > 0, "Size N must be greater than 0!") ;

	// ============= Constructors =============
	constexpr bytes() noexcept = default ;
	constexpr bytes(const bytes&) noexcept = default ;
	constexpr bytes(bytes&&) noexcept = default ;
	constexpr bytes& operator=(const bytes&) noexcept = default ;
	constexpr bytes& operator=(bytes&&) noexcept = default ;

	/**
	 * @brief Konstruktor dari array byte
	 * @param data Array byte dengan ukuran N
	 */
	constexpr bytes(const byte_t (&data)[N]) noexcept {
		std::copy(data, data + N, data_) ;
	}

	/**
	 * @brief Konstruktor dari pointer dan panjang
	 * @param data Pointer ke data byte
	 * @param len Panjang data (max N byte akan disalin)
	 */
	constexpr bytes(const byte_t* data, size_t len) noexcept {
		std::copy(data, data + std::min(N, len), data_) ;
	}

	/**
	 * @brief Konstruktor dari integer type
	 * @tparam IntType Tipe integer (uint8_t, uint16_t, uint32_t, uint64_t, dll)
	 * @param value Nilai integer yang akan dikonversi
	 * @note Byte order adalah little-endian
	 */
	template <typename IntType>
	requires std::is_integral_v<IntType>
	constexpr explicit bytes(IntType value) noexcept {
		constexpr size_t int_size = sizeof(IntType) ;
		constexpr size_t copy_size = std::min(N, int_size) ;
		std::memcpy(data_, &value, copy_size) ;
	}

	// ============= Element Access =============
	
	/**
	 * @brief Akses elemen dengan bounds checking (clamp ke range valid)
	 * @param index Indeks elemen
	 * @return Reference ke byte
	 */
	[[nodiscard]] constexpr const byte_t& operator[](index_t index) const noexcept {
		return data_[std::min(index, N - 1)] ;
	}

	[[nodiscard]] constexpr byte_t& operator[](index_t index) noexcept {
		return data_[std::min(index, N - 1)] ;
	}

	/**
	 * @brief Akses elemen tanpa bounds checking (lebih cepat)
	 * @param index Indeks elemen (harus valid: 0 <= index < N)
	 */
	[[nodiscard]] constexpr const byte_t& at(index_t index) const noexcept {
		return data_[index] ;
	}

	[[nodiscard]] constexpr byte_t& at(index_t index) noexcept {
		return data_[index] ;
	}

	// ============= Bitwise Operations =============

	/**
	 * @brief Operasi OR bitwise
	 * @note Optimized dengan loop unrolling hint
	 */
	[[nodiscard]] constexpr bytes operator|(const bytes& other) const noexcept {
		bytes result ;
		for (index_t i = 0 ; i < N ; ++i) {
			result.data_[i] = data_[i] | other.data_[i] ;
		}
		return result ;
	}

	[[nodiscard]] constexpr bytes operator&(const bytes& other) const noexcept {
		bytes result ;
		for (index_t i = 0 ; i < N ; ++i) {
			result.data_[i] = data_[i] & other.data_[i] ;
		}
		return result ;
	}

	[[nodiscard]] constexpr bytes operator^(const bytes& other) const noexcept {
		bytes result ;
		for (index_t i = 0 ; i < N ; ++i) {
			result.data_[i] = data_[i] ^ other.data_[i] ;
		}
		return result ;
	}

	[[nodiscard]] constexpr bytes operator~() const noexcept {
		bytes result ;
		for (index_t i = 0 ; i < N ; ++i) {
			result.data_[i] = ~data_[i] ;
		}
		return result ;
	}

	// ============= Shift Operations =============

	/**
	 * @brief Left shift (shift ke kiri)
	 * @param shift_bits Jumlah bit yang di-shift
	 * @return Hasil shift
	 * @note Implementasi optimized dengan carry propagation yang benar
	 */
	[[nodiscard]] constexpr bytes operator<<(index_t shift_bits) const noexcept {
		if (shift_bits == 0) return *this ;
		if (shift_bits >= N * 8) return bytes{} ;

		bytes result ;
		const index_t byte_shift = shift_bits / 8 ;
		const index_t bit_shift = shift_bits % 8 ;

		if (bit_shift == 0) {
			// Byte-aligned shift (lebih cepat)
			for (index_t i = byte_shift ; i < N ; ++i) {
				result.data_[i] = data_[i - byte_shift] ;
			}
		} else {
			// Bit shift dengan carry
			byte_t carry = 0 ;
			for (index_t i = 0 ; i < N - byte_shift ; ++i) {
				index_t src_idx = i ;
				index_t dst_idx = i + byte_shift ;
				result.data_[dst_idx] = (data_[src_idx] << bit_shift) | carry ;
				carry = data_[src_idx] >> (8 - bit_shift) ;
			}
		}
		return result ;
	}

	/**
	 * @brief Right shift (shift ke kanan)
	 * @param shift_bits Jumlah bit yang di-shift
	 * @return Hasil shift
	 */
	[[nodiscard]] constexpr bytes operator>>(index_t shift_bits) const noexcept {
		if (shift_bits == 0) return *this ;
		if (shift_bits >= N * 8) return bytes{} ;

		bytes result ;
		const index_t byte_shift = shift_bits / 8 ;
		const index_t bit_shift = shift_bits % 8 ;

		if (bit_shift == 0) {
			// Byte-aligned shift
			for (index_t i = 0 ; i < N - byte_shift ; ++i) {
				result.data_[i] = data_[i + byte_shift] ;
			}
		} else {
			// Bit shift dengan carry
			byte_t carry = 0 ;
			for (index_t i = N - byte_shift ; i-- > 0 ;) {
				index_t src_idx = i + byte_shift ;
				index_t dst_idx = i ;
				result.data_[dst_idx] = (data_[src_idx] >> bit_shift) | carry ;
				carry = data_[src_idx] << (8 - bit_shift) ;
			}
		}
		return result ;
	}

	// ============= Compound Assignment =============

	constexpr bytes& operator|=(const bytes& other) noexcept { return (*this = (*this | other)) ; }
	constexpr bytes& operator&=(const bytes& other) noexcept { return (*this = (*this & other)) ; }
	constexpr bytes& operator^=(const bytes& other) noexcept { return (*this = (*this ^ other)) ; }
	constexpr bytes& operator<<=(index_t n) noexcept { return (*this = (*this << n)) ; }
	constexpr bytes& operator>>=(index_t n) noexcept { return (*this = (*this >> n)) ; }

	// ============= Bit Manipulation =============

	/**
	 * @brief Set bit pada posisi tertentu
	 * @param bit_pos Posisi bit (0 = LSB byte pertama)
	 */
	constexpr void set_bit(index_t bit_pos) noexcept {
		if (bit_pos < N * 8) {
			data_[bit_pos / 8] |= (1 << (bit_pos % 8)) ;
		}
	}

	/**
	 * @brief Clear bit pada posisi tertentu
	 */
	constexpr void clear_bit(index_t bit_pos) noexcept {
		if (bit_pos < N * 8) {
			data_[bit_pos / 8] &= ~(1 << (bit_pos % 8)) ;
		}
	}

	/**
	 * @brief Toggle bit pada posisi tertentu
	 */
	constexpr void toggle_bit(index_t bit_pos) noexcept {
		if (bit_pos < N * 8) {
			data_[bit_pos / 8] ^= (1 << (bit_pos % 8)) ;
		}
	}

	/**
	 * @brief Test bit pada posisi tertentu
	 * @return true jika bit = 1, false jika bit = 0
	 */
	[[nodiscard]] constexpr bool test_bit(index_t bit_pos) const noexcept {
		if (bit_pos >= N * 8) return false ;
		return (data_[bit_pos / 8] & (1 << (bit_pos % 8))) != 0 ;
	}

	/**
	 * @brief Hitung jumlah bit yang bernilai 1
	 */
	[[nodiscard]] constexpr size_t popcount() const noexcept {
		size_t count = 0 ;
		for (index_t i = 0 ; i < N ; ++i) {
			count += std::popcount(data_[i]) ;
		}
		return count ;
	}

	// ============= Rotation =============

	/**
	 * @brief Rotate left (circular shift)
	 * @param n Jumlah bit untuk rotate
	 */
	[[nodiscard]] constexpr bytes rotate_left(index_t n) const noexcept {
		n %= (N * 8) ;
		return (*this << n) | (*this >> (N * 8 - n)) ;
	}

	/**
	 * @brief Rotate right (circular shift)
	 */
	[[nodiscard]] constexpr bytes rotate_right(index_t n) const noexcept {
		n %= (N * 8) ;
		return (*this >> n) | (*this << (N * 8 - n)) ;
	}

	// ============= Conversion =============

	/**
	 * @brief Konversi ke integer type
	 * @tparam IntType Tipe integer target
	 * @return Nilai integer (little-endian)
	 */
	template <typename IntType>
	requires std::is_integral_v<IntType>
	[[nodiscard]] constexpr IntType to_int() const noexcept {
		IntType result = 0 ;
		constexpr size_t copy_size = std::min(N, sizeof(IntType)) ;
		std::memcpy(&result, data_, copy_size) ;
		return result ;
	}

	// ============= Iterators  =============

	[[nodiscard]] constexpr auto begin() noexcept { return iterator<byte_t>(data_) ; }
	[[nodiscard]] constexpr auto end() noexcept { return iterator<byte_t>(data_ + N) ; }
	[[nodiscard]] constexpr auto begin() const noexcept { return const_iterator<byte_t>(data_) ; }
	[[nodiscard]] constexpr auto end() const noexcept { return const_iterator<byte_t>(data_ + N) ; }
	[[nodiscard]] constexpr auto cbegin() const noexcept { return const_iterator<byte_t>(data_) ; }
	[[nodiscard]] constexpr auto cend() const noexcept { return const_iterator<byte_t>(data_ + N) ; }

	// ============= Element Access =============

	[[nodiscard]] constexpr byte_t& front() noexcept { return data_[0] ; }
	[[nodiscard]] constexpr byte_t& back() noexcept { return data_[N - 1] ; }
	[[nodiscard]] constexpr const byte_t& front() const noexcept { return data_[0] ; }
	[[nodiscard]] constexpr const byte_t& back() const noexcept { return data_[N - 1] ; }

	// ============= Capacity =============

	[[nodiscard]] constexpr size_t size() const noexcept { return N ; }
	[[nodiscard]] constexpr size_t bit_size() const noexcept { return N * 8 ; }
	[[nodiscard]] constexpr bool empty() const noexcept { return false ; }

	// ============= Data Access =============

	[[nodiscard]] constexpr byte_t* data() noexcept { return data_ ; }
	[[nodiscard]] constexpr const byte_t* data() const noexcept { return data_ ; }

	// ============= Modifiers =============

	constexpr void fill(byte_t value) noexcept {
		std::fill(data_, data_ + N, value) ;
	}

	constexpr void clear() noexcept { fill(0) ; }

	/**
	 * @brief Reverse urutan byte (bukan bit!)
	 * @note Untuk endianness conversion
	 */
	[[nodiscard]] constexpr bytes reverse() const noexcept {
		bytes result ;
		for (index_t i = 0 ; i < N ; ++i) {
			result.data_[i] = data_[N - 1 - i] ;
		}
		return result ;
	}

	// ============= Comparison =============

	[[nodiscard]] constexpr bool operator==(const bytes&) const noexcept = default ;
	[[nodiscard]] constexpr auto operator<=>(const bytes&) const noexcept = default ;

	// ============= Output (Binary) =============

	friend inline std::ostream& operator<<(std::ostream& os, const bytes& b) {
		for (index_t i = 0 ; i < N ; ++i) {
			for (int j = 7 ; j >= 0 ; --j) {
				os << ((b.data_[i] >> j) & 1 ? '1' : '0') ;
			}
			if (i != N - 1) os << ' ' ;
		}
		return os ;
	}

	/**
	 * @brief Print dalam format hexadecimal
	 */
	void print_hex(std::ostream& os = std::cout) const {
		os << "0x" ;
		for (index_t i = N ; i-- > 0 ;) {
			os << std::hex << std::setfill('0') << std::setw(2) 
			   << static_cast<int>(data_[i]) ;
		}
		os << std::dec ;
	}
} ;
