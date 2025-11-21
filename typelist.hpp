#pragma once

#include <type_traits>

namespace zuu {
	namespace impl {
		namespace help {
			struct max {
				template <typename T, typename U>
				requires (std::is_arithmetic_v<T> && std::is_arithmetic_v<U>)
				constexpr auto operator()(T a, U b) const noexcept {
					using R = std::common_type_t<T, U> ;
					return (a > b) ? static_cast<R>(a) : static_cast<R>(b) ;
				}

				template <typename T, typename U, typename ...Ts>
				requires (std::is_arithmetic_v<T> && std::is_arithmetic_v<U> && (std::is_arithmetic_v<Ts> && ...))
				constexpr auto operator()(T a, U b, Ts... o) const noexcept {
					using R = std::common_type_t<T, U, Ts...> ;
					auto ab = (*this)(a, b) ;
					if constexpr (sizeof...(o) == 0) return static_cast<R>(ab) ;
					else return (*this)(static_cast<R>(ab), static_cast<R>(o)...) ;
				}
			} ;
		}


		template <typename ... Ts> 
		struct type_list { 
			static constexpr size_t count = sizeof...(Ts) ; 
		} ;

		template <size_t, typename> 
		struct type_at ;

		template <size_t N, typename T, typename ... Ts> 
		struct type_at<N, type_list<T, Ts...>> { 
			using type = typename type_at<N - 1, type_list<Ts...>>::type ; 
		} ;

		template <typename T, typename ... Ts> 
		struct type_at<0, type_list<T, Ts...>> { 
			using type = T ; 
		} ;

		template <typename, typename> 
		struct index_of ;

		template <typename T, typename ... Ts> 
		struct index_of<T, type_list<T, Ts...>> { 
			static constexpr size_t index = 0 ; 
		} ;

		template <typename T, typename U, typename ... Us> 
		struct index_of<T, type_list<U, Us...>> { 
			static constexpr size_t index = 1 + index_of<T, type_list<Us...>>::index ; 
		} ;

		template <typename T>  
		struct index_of<T, type_list<>> { 
			static constexpr size_t index = ~0U ; 
		} ;

		template <typename, typename> 
		struct contains ;

		template <typename T> 
		struct contains<T, type_list<>> : std::false_type {} ;

		template <typename T, typename U, typename ... Us> 
		struct contains<T, type_list<U, Us...>> : std::bool_constant<std::is_same_v<T, U> || contains<T, type_list<Us...>>::value> {} ;
	}

	template <typename ... Ts>
	struct type_list_t {
		static constexpr size_t count = impl::type_list<Ts...>::count ;
		static constexpr size_t total_size = (0 + ... + sizeof(Ts)) ;

		static constexpr size_t max_size = []() constexpr -> size_t {
			if constexpr (count == 0) return 0 ;
			else if constexpr (count == 1) return sizeof(typename impl::type_at<0, impl::type_list<Ts...>>::type) ;
			else return impl::help::max{}(sizeof(Ts)...) ;
		}() ;

		static constexpr size_t max_align = []() constexpr -> size_t {
			if constexpr (count == 0) return 1 ;
			else if constexpr (count == 1) return alignof(typename impl::type_at<0, impl::type_list<Ts...>>::type) ;
			else return impl::help::max{}(alignof(Ts)...) ;
		}() ;

		template <typename T> 
		static constexpr size_t index_of = impl::index_of<T, impl::type_list<Ts...>>::index ;

		template <typename T> 
		static constexpr bool contains = impl::contains<T, impl::type_list<Ts...>>::value ;

		template <size_t N> 
		using type = typename impl::type_at<N, impl::type_list<Ts...>>::type ;
	} ;
}
