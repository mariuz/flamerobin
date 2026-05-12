/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FBCPP_STRUCT_BINDING_H
#define FBCPP_STRUCT_BINDING_H

#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>


namespace fbcpp
{
	///
	/// Concept constraining types to aggregates suitable for struct binding.
	///
	template <typename T>
	concept Aggregate = std::is_aggregate_v<T> && !std::is_array_v<T> && !std::is_union_v<T>;

	///
	/// Concept constraining types to tuple-like types (std::tuple, std::pair, std::array).
	///
	template <typename T>
	concept TupleLike = !Aggregate<T> && requires { typename std::tuple_size<T>::type; };
}  // namespace fbcpp

namespace fbcpp::impl::reflection
{
	/// Maximum number of struct fields supported.
	inline constexpr std::size_t maxFieldCount = 32;

	/// Helper to detect std::optional
	template <typename T>
	struct IsOptional : std::false_type
	{
	};

	template <typename T>
	struct IsOptional<std::optional<T>> : std::true_type
	{
	};

	template <typename T>
	inline constexpr bool isOptionalV = IsOptional<T>::value;

	///
	/// Helper to detect std::variant.
	///
	template <typename T>
	struct IsVariant : std::false_type
	{
	};

	template <typename... Ts>
	struct IsVariant<std::variant<Ts...>> : std::true_type
	{
	};

	template <typename T>
	inline constexpr bool isVariantV = IsVariant<T>::value;

	///
	/// Check if variant contains a specific type.
	///
	template <typename T, typename Variant>
	struct VariantContains : std::false_type
	{
	};

	template <typename T, typename... Ts>
	struct VariantContains<T, std::variant<Ts...>> : std::disjunction<std::is_same<T, Ts>...>
	{
	};

	template <typename T, typename Variant>
	inline constexpr bool variantContainsV = VariantContains<T, Variant>::value;

	///
	/// Helper to detect opaque (non-convertible) types that only match exact SQL types.
	/// Forward declarations for opaque types from types.h.
	///
	template <typename T>
	struct IsOpaqueType : std::false_type
	{
	};

	template <typename T>
	inline constexpr bool isOpaqueTypeV = IsOpaqueType<T>::value;

	/// Universal converter for aggregate initialization detection.
	struct UniversalType
	{
		template <typename T>
		constexpr operator T() const noexcept
		{
			if constexpr (isOptionalV<T>)
				return T{std::nullopt};
			else
				return T{};
		}
	};

	// Field count detection via SFINAE
	template <typename T, typename Seq, typename = void>
	struct IsBraceConstructibleImpl : std::false_type
	{
	};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif

	template <typename T, std::size_t... Is>
	struct IsBraceConstructibleImpl<T, std::index_sequence<Is...>,
		std::void_t<decltype(T{(void(Is), UniversalType{})...})>> : std::true_type
	{
	};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

	template <typename T, std::size_t N>
	inline constexpr bool isBraceConstructibleV = IsBraceConstructibleImpl<T, std::make_index_sequence<N>>::value;

	// Binary search for field count
	template <typename T, std::size_t Lo, std::size_t Hi>
	constexpr std::size_t detectFieldCount()
	{
		if constexpr (Lo == Hi)
			return Lo;
		else
		{
			constexpr std::size_t Mid = Lo + (Hi - Lo + 1) / 2;
			if constexpr (isBraceConstructibleV<T, Mid>)
				return detectFieldCount<T, Mid, Hi>();
			else
				return detectFieldCount<T, Lo, Mid - 1>();
		}
	}

	/// Number of fields in aggregate type T.
	template <typename T>
	inline constexpr std::size_t fieldCountV = detectFieldCount<T, 0, maxFieldCount>();

	// toTupleRef implementation for each field count (0-32)
	template <typename T>
	auto toTupleRef(T&& obj)
	{
		constexpr std::size_t N = fieldCountV<std::remove_cvref_t<T>>;

		if constexpr (N == 0)
			return std::tuple<>{};
		else if constexpr (N == 1)
		{
			auto&& [v1] = std::forward<T>(obj);
			return std::forward_as_tuple(v1);
		}
		else if constexpr (N == 2)
		{
			auto&& [v1, v2] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2);
		}
		else if constexpr (N == 3)
		{
			auto&& [v1, v2, v3] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3);
		}
		else if constexpr (N == 4)
		{
			auto&& [v1, v2, v3, v4] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4);
		}
		else if constexpr (N == 5)
		{
			auto&& [v1, v2, v3, v4, v5] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5);
		}
		else if constexpr (N == 6)
		{
			auto&& [v1, v2, v3, v4, v5, v6] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6);
		}
		else if constexpr (N == 7)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7);
		}
		else if constexpr (N == 8)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8);
		}
		else if constexpr (N == 9)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9);
		}
		else if constexpr (N == 10)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10);
		}
		else if constexpr (N == 11)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11);
		}
		else if constexpr (N == 12)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12);
		}
		else if constexpr (N == 13)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13);
		}
		else if constexpr (N == 14)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14);
		}
		else if constexpr (N == 15)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15);
		}
		else if constexpr (N == 16)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16);
		}
		else if constexpr (N == 17)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17);
		}
		else if constexpr (N == 18)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18] =
				std::forward<T>(obj);
			return std::forward_as_tuple(
				v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18);
		}
		else if constexpr (N == 19)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19] =
				std::forward<T>(obj);
			return std::forward_as_tuple(
				v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19);
		}
		else if constexpr (N == 20)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20] =
				std::forward<T>(obj);
			return std::forward_as_tuple(
				v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20);
		}
		else if constexpr (N == 21)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21] =
				std::forward<T>(obj);
			return std::forward_as_tuple(
				v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21);
		}
		else if constexpr (N == 22)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21,
				v22] = std::forward<T>(obj);
			return std::forward_as_tuple(
				v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22);
		}
		else if constexpr (N == 23)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23);
		}
		else if constexpr (N == 24)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24);
		}
		else if constexpr (N == 25)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24, v25] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24, v25);
		}
		else if constexpr (N == 26)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24, v25, v26] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24, v25, v26);
		}
		else if constexpr (N == 27)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24, v25, v26, v27] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24, v25, v26, v27);
		}
		else if constexpr (N == 28)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24, v25, v26, v27, v28] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28);
		}
		else if constexpr (N == 29)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24, v25, v26, v27, v28, v29] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29);
		}
		else if constexpr (N == 30)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24, v25, v26, v27, v28, v29, v30] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30);
		}
		else if constexpr (N == 31)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24, v25, v26, v27, v28, v29, v30, v31] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31);
		}
		else if constexpr (N == 32)
		{
			auto&& [v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
				v23, v24, v25, v26, v27, v28, v29, v30, v31, v32] = std::forward<T>(obj);
			return std::forward_as_tuple(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
				v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32);
		}
		else
			static_assert(N <= maxFieldCount, "Struct has too many fields for struct binding (max 32)");
	}

	/// Type of the tuple representing T's fields.
	template <typename T>
	using TupleType = decltype(toTupleRef(std::declval<T&>()));

	/// Type of the I-th field of aggregate T.
	template <typename T, std::size_t I>
	using FieldType = std::remove_reference_t<std::tuple_element_t<I, TupleType<T>>>;
}  // namespace fbcpp::impl::reflection

namespace fbcpp
{
	///
	/// Concept constraining types to std::variant types.
	///
	template <typename T>
	concept VariantLike = impl::reflection::isVariantV<std::remove_cvref_t<T>>;
}  // namespace fbcpp


#endif  // FBCPP_STRUCT_BINDING_H
