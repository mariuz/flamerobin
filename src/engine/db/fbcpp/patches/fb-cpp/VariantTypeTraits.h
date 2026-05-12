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

#ifndef FBCPP_VARIANT_TYPE_TRAITS_H
#define FBCPP_VARIANT_TYPE_TRAITS_H

#include "config.h"
#include "types.h"
#include "StructBinding.h"

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif


namespace fbcpp::impl::reflection
{
	///
	/// Helper to check if a type is supported by the library for variant usage.
	///
	template <typename T>
	struct IsSupportedVariantType : std::false_type
	{
	};

	// Arithmetic types
	template <>
	struct IsSupportedVariantType<bool> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<std::int16_t> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<std::int32_t> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<std::int64_t> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<float> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<double> : std::true_type
	{
	};

	// Scaled numbers
	template <>
	struct IsSupportedVariantType<ScaledInt16> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<ScaledInt32> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<ScaledInt64> : std::true_type
	{
	};

	// String
	template <>
	struct IsSupportedVariantType<std::string> : std::true_type
	{
	};

	// Date/Time types
	template <>
	struct IsSupportedVariantType<Date> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<Time> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<Timestamp> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<TimeTz> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<TimestampTz> : std::true_type
	{
	};

	// Opaque Date/Time types
	template <>
	struct IsSupportedVariantType<OpaqueDate> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<OpaqueTime> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<OpaqueTimestamp> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<OpaqueTimeTz> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<OpaqueTimestampTz> : std::true_type
	{
	};

	// Blob
	template <>
	struct IsSupportedVariantType<BlobId> : std::true_type
	{
	};

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	// Boost multiprecision types
	template <>
	struct IsSupportedVariantType<BoostInt128> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<ScaledBoostInt128> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<BoostDecFloat16> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<BoostDecFloat34> : std::true_type
	{
	};

	// Opaque multiprecision types
	template <>
	struct IsSupportedVariantType<ScaledOpaqueInt128> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<OpaqueDecFloat16> : std::true_type
	{
	};
	template <>
	struct IsSupportedVariantType<OpaqueDecFloat34> : std::true_type
	{
	};
#endif

	// std::monostate is always allowed for NULL representation
	template <>
	struct IsSupportedVariantType<std::monostate> : std::true_type
	{
	};

	template <typename T>
	inline constexpr bool isSupportedVariantTypeV = IsSupportedVariantType<T>::value;

	///
	/// Recursively validate all variant alternatives are supported.
	///
	template <typename V, std::size_t I = 0>
	struct VariantAlternativesSupported : std::true_type
	{
	};

	template <typename V, std::size_t I>
	requires(I < std::variant_size_v<V>)
	struct VariantAlternativesSupported<V, I>
	{
		using Alt = std::variant_alternative_t<I, V>;
		static constexpr bool value = isSupportedVariantTypeV<Alt> && VariantAlternativesSupported<V, I + 1>::value;
	};

	template <typename V>
	inline constexpr bool variantAlternativesSupportedV = VariantAlternativesSupported<V>::value;
}  // namespace fbcpp::impl::reflection


// Specializations for opaque type detection
namespace fbcpp::impl::reflection
{
	template <>
	struct IsOpaqueType<ScaledOpaqueInt128> : std::true_type
	{
	};

	template <>
	struct IsOpaqueType<OpaqueDecFloat16> : std::true_type
	{
	};

	template <>
	struct IsOpaqueType<OpaqueDecFloat34> : std::true_type
	{
	};

	template <>
	struct IsOpaqueType<OpaqueDate> : std::true_type
	{
	};

	template <>
	struct IsOpaqueType<OpaqueTime> : std::true_type
	{
	};

	template <>
	struct IsOpaqueType<OpaqueTimestamp> : std::true_type
	{
	};

	template <>
	struct IsOpaqueType<OpaqueTimeTz> : std::true_type
	{
	};

	template <>
	struct IsOpaqueType<OpaqueTimestampTz> : std::true_type
	{
	};
}  // namespace fbcpp::impl::reflection


#endif  // FBCPP_VARIANT_TYPE_TRAITS_H
