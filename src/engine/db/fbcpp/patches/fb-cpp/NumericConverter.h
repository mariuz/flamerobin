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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FBCPP_NUMERIC_CONVERTER_H
#define FBCPP_NUMERIC_CONVERTER_H

#include "config.h"
#include "fb-api.h"
#include "Client.h"
#include "Exception.h"
#include "types.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>


namespace fbcpp::impl
{
	template <typename T>
	struct NumberTypePriority
	{
		static constexpr int value = 0;
	};

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	struct NumberTypePriority<BoostDecFloat34>
	{
		static constexpr int value = 8;
	};

	template <>
	struct NumberTypePriority<BoostDecFloat16>
	{
		static constexpr int value = 7;
	};
#endif

	template <>
	struct NumberTypePriority<double>
	{
		static constexpr int value = 6;
	};

	template <>
	struct NumberTypePriority<float>
	{
		static constexpr int value = 5;
	};

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	struct NumberTypePriority<BoostInt128>
	{
		static constexpr int value = 4;
	};
#endif

	template <>
	struct NumberTypePriority<std::int64_t>
	{
		static constexpr int value = 3;
	};

	template <>
	struct NumberTypePriority<std::int32_t>
	{
		static constexpr int value = 2;
	};

	template <>
	struct NumberTypePriority<std::int16_t>
	{
		static constexpr int value = 1;
	};

	template <typename T>
	inline constexpr int NumberTypePriorityValue =
		NumberTypePriority<std::remove_cv_t<std::remove_reference_t<T>>>::value;

	template <typename T1, typename T2>
	using GreaterNumberType = std::conditional_t<(NumberTypePriorityValue<T1> >= NumberTypePriorityValue<T2>), T1, T2>;

	template <typename T>
	inline constexpr bool IsFloatingNumber = std::is_floating_point_v<T>
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		|| std::same_as<T, BoostDecFloat16> || std::same_as<T, BoostDecFloat34>
#endif
		;

	template <typename T>
	concept FloatingNumber = IsFloatingNumber<T>;

	template <typename T>
	concept IntegralNumber = std::is_integral_v<T>
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		|| std::same_as<T, BoostInt128>
#endif
		;

	template <typename T>
	struct MakeUnsigned
	{
		using type = std::make_unsigned_t<T>;
	};

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	struct MakeUnsigned<BoostInt128>
	{
		using type = boost::multiprecision::uint128_t;
	};
#endif

	template <typename T>
	using MakeUnsignedType = typename MakeUnsigned<T>::type;

	class NumericConverter final
	{
	public:
		explicit NumericConverter(Client& client)
			: client{&client}
		{
		}

		[[noreturn]] void throwNumericOutOfRange()
		{
			static constexpr std::intptr_t STATUS_NUMERIC_OUT_OF_RANGE[] = {
				isc_arith_except,
				isc_numeric_out_of_range,
				isc_arg_end,
			};

			throw DatabaseException(*client, STATUS_NUMERIC_OUT_OF_RANGE);
		}

		[[noreturn]] void throwConversionErrorFromString(const std::string& str)
		{
			const std::intptr_t STATUS_CONVERSION_ERROR_FROM_STRING[] = {
				isc_convert_error,
				reinterpret_cast<std::intptr_t>(str.c_str()),
				isc_arg_end,
			};

			throw DatabaseException(*client, STATUS_CONVERSION_ERROR_FROM_STRING);
		}

	public:
		template <IntegralNumber To, IntegralNumber From>
		To numberToNumber(const ScaledNumber<From>& from, int toScale)
		{
			const int scaleDiff = toScale - from.scale;

			using ComputeType = GreaterNumberType<decltype(from.value), To>;

			ComputeType result = static_cast<ComputeType>(from.value);

			if (scaleDiff != 0)
			{
				adjustScale(result, scaleDiff, static_cast<ComputeType>(std::numeric_limits<To>::min()),
					static_cast<ComputeType>(std::numeric_limits<To>::max()));
			}

			if (result < static_cast<ComputeType>(std::numeric_limits<To>::min()) ||
				result > static_cast<ComputeType>(std::numeric_limits<To>::max()))
			{
				throwNumericOutOfRange();
			}

			return static_cast<To>(result);
		}

		template <IntegralNumber To, FloatingNumber From>
		To numberToNumber(const From& from, int toScale)
		{
			using ComputeType = GreaterNumberType<double, From>;

			if constexpr (std::is_floating_point_v<From>)
			{
				if (std::isnan(from) || std::isinf(from))
					throwNumericOutOfRange();
			}

			ComputeType value{from};
			const ComputeType eps = conversionEpsilon<ComputeType>();

			if (toScale > 0)
				value /= powerOfTen(toScale);
			else if (toScale < 0)
				value *= powerOfTen(-toScale);

			if (value > 0)
				value += 0.5f + eps;
			else
				value -= 0.5f + eps;

			static const auto minLimit = static_cast<ComputeType>(std::numeric_limits<To>::min());
			static const auto maxLimit = static_cast<ComputeType>(std::numeric_limits<To>::max());

			if (value < minLimit)
			{
				if (value > minLimit - 1.0f)
					return std::numeric_limits<To>::min();
				throwNumericOutOfRange();
			}

			if (value > maxLimit)
			{
				if (value < maxLimit + 1.0f)
					return std::numeric_limits<To>::max();
				throwNumericOutOfRange();
			}

			return static_cast<To>(value);
		}

		template <FloatingNumber To, typename From>
		To numberToNumber(const ScaledNumber<From>& from, int toScale = 0)
		{
			assert(toScale == 0);

			using ComputeType = GreaterNumberType<double, To>;

			ComputeType value = static_cast<ComputeType>(from.value);  // FIXME: decfloat

			if (from.scale != 0)
			{
				if (std::abs(from.scale) > std::numeric_limits<To>::max_exponent10)
					throwNumericOutOfRange();

				if (from.scale > 0)
					value *= powerOfTen(from.scale);
				else if (from.scale < 0)
					value /= powerOfTen(-from.scale);
			}

			return static_cast<To>(value);
		}

		template <FloatingNumber To, FloatingNumber From>
		To numberToNumber(const From& from, int toScale = 0)
		{
			assert(toScale == 0);

			if constexpr (std::is_floating_point_v<From> && !std::is_floating_point_v<To>)
				return To{std::format("{:.16e}", from)};
			else
				return static_cast<To>(from);
		}

		template <IntegralNumber From>
		std::string numberToString(const ScaledNumber<From>& from)
		{
			char buffer[64];

			const bool isNegative = from.value < 0;
			const bool isMinLimit = from.value == std::numeric_limits<decltype(from.value)>::min();

			using UnsignedType = MakeUnsignedType<decltype(from.value)>;

			auto unsignedValue = isMinLimit ? static_cast<UnsignedType>(-(from.value + 1)) + 1
											: static_cast<UnsignedType>(isNegative ? -from.value : from.value);

			int digitCount = 0;

			do
			{
				buffer[digitCount++] = static_cast<char>((unsignedValue % 10) + '0');
				unsignedValue /= 10;
			} while (unsignedValue > 0);

			std::string result;

			if (isNegative)
				result += '-';

			if (from.scale >= 0)
			{
				for (int i = digitCount - 1; i >= 0; --i)
					result += buffer[i];

				result.append(static_cast<std::string::size_type>(from.scale), '0');
			}
			else
			{
				const int decimalPlaces = -from.scale;

				if (decimalPlaces >= static_cast<int>(digitCount))
				{
					result += "0.";
					const int leadingZeros = decimalPlaces - digitCount;
					result.append(static_cast<std::string::size_type>(leadingZeros), '0');

					for (int i = digitCount - 1; i >= 0; --i)
						result += buffer[i];
				}
				else
				{
					for (int i = digitCount - 1; i >= decimalPlaces; --i)
						result += buffer[i];

					result += '.';

					for (int i = decimalPlaces - 1; i >= 0; --i)
						result += buffer[i];
				}
			}

			return result;
		}

		template <FloatingNumber From>
		std::string numberToString(const From& from)
		{
			if constexpr (std::is_floating_point_v<From>)
			{
				if (std::isnan(from))
					return "NaN";
				if (std::isinf(from))
					return from > 0 ? "Infinity" : "-Infinity";
				return std::to_string(from);
			}
			else
				return from.str();
		}

		std::string opaqueInt128ToString(StatusWrapper* statusWrapper, const OpaqueInt128& opaqueInt128, int scale)
		{
			const auto int128Util = client->getInt128Util(statusWrapper);
			char buffer[fb::IInt128::STRING_SIZE + 1];
			int128Util->toString(statusWrapper, &opaqueInt128, scale, static_cast<unsigned>(sizeof(buffer)), buffer);
			return buffer;
		}

		std::string opaqueDecFloat16ToString(StatusWrapper* statusWrapper, const OpaqueDecFloat16& opaqueDecFloat16)
		{
			const auto decFloat16Util = client->getDecFloat16Util(statusWrapper);
			char buffer[fb::IDecFloat16::STRING_SIZE + 1];
			decFloat16Util->toString(statusWrapper, &opaqueDecFloat16, static_cast<unsigned>(sizeof(buffer)), buffer);
			return buffer;
		}

		std::string opaqueDecFloat34ToString(StatusWrapper* statusWrapper, const OpaqueDecFloat34& opaqueDecFloat34)
		{
			const auto decFloat34Util = client->getDecFloat34Util(statusWrapper);
			char buffer[fb::IDecFloat34::STRING_SIZE + 1];
			decFloat34Util->toString(statusWrapper, &opaqueDecFloat34, static_cast<unsigned>(sizeof(buffer)), buffer);
			return buffer;
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		OpaqueInt128 boostInt128ToOpaqueInt128(const BoostInt128& boostInt128)
		{
			const boost::multiprecision::uint128_t boostUInt128{boostInt128};

			OpaqueInt128 opaqueInt128;
			opaqueInt128.fb_data[0] = static_cast<std::uint64_t>(boostUInt128 & 0xFFFFFFFFFFFFFFFFULL);
			opaqueInt128.fb_data[1] = static_cast<std::uint64_t>(boostUInt128 >> 64);

			return opaqueInt128;
		}

		BoostInt128 opaqueInt128ToBoostInt128(const OpaqueInt128& opaqueInt128)
		{
			const auto high = static_cast<std::int64_t>(opaqueInt128.fb_data[1]);
			BoostInt128 boostInt128 = static_cast<BoostInt128>(high);
			boostInt128 <<= 64;
			boostInt128 += static_cast<BoostInt128>(opaqueInt128.fb_data[0]);
			return boostInt128;
		}

		OpaqueDecFloat16 boostDecFloat16ToOpaqueDecFloat16(
			StatusWrapper* statusWrapper, const BoostDecFloat16& boostDecFloat16)
		{
			const auto decFloat16Util = client->getDecFloat16Util(statusWrapper);
			OpaqueDecFloat16 opaqueDecFloat16;
			decFloat16Util->fromString(statusWrapper, boostDecFloat16.str().c_str(), &opaqueDecFloat16);
			return opaqueDecFloat16;
		}

		BoostDecFloat16 opaqueDecFloat16ToBoostDecFloat16(
			StatusWrapper* statusWrapper, const OpaqueDecFloat16& opaqueDecFloat16)
		{
			return BoostDecFloat16{opaqueDecFloat16ToString(statusWrapper, opaqueDecFloat16)};
		}

		OpaqueDecFloat34 boostDecFloat34ToOpaqueDecFloat34(
			StatusWrapper* statusWrapper, const BoostDecFloat34& boostDecFloat34)
		{
			const auto decFloat34Util = client->getDecFloat34Util(statusWrapper);
			OpaqueDecFloat34 opaqueDecFloat34;
			decFloat34Util->fromString(statusWrapper, boostDecFloat34.str().c_str(), &opaqueDecFloat34);
			return opaqueDecFloat34;
		}

		BoostDecFloat34 opaqueDecFloat34ToBoostDecFloat34(
			StatusWrapper* statusWrapper, const OpaqueDecFloat34& opaqueDecFloat34)
		{
			return BoostDecFloat34{opaqueDecFloat34ToString(statusWrapper, opaqueDecFloat34)};
		}
#endif

		// FIXME: move
		std::byte stringToBoolean(std::string_view value)
		{
			auto trimmed = value;

			while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())))
				trimmed.remove_prefix(1);

			while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
				trimmed.remove_suffix(1);

			std::string normalized{trimmed};
			std::transform(normalized.begin(), normalized.end(), normalized.begin(),
				[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

			if (normalized == "true")
				return std::byte{1};
			else if (normalized == "false")
				return std::byte{0};

			throwConversionErrorFromString(std::string{value});
		}

	private:
		double powerOfTen(int scale) noexcept  // FIXME: only for double?
		{
			/* FIXME:
			BoostDecFloat34 powerOfTenDecInternal(unsigned scale)
			{
			    return boost::multiprecision::pow(BoostDecFloat34{10}, scale);
			}
			*/

			static constexpr double UPPER_PART[] = {
				1.e000,
				1.e032,
				1.e064,
				1.e096,
				1.e128,
				1.e160,
				1.e192,
				1.e224,
				1.e256,
				1.e288,
			};

			static constexpr double LOWER_PART[] = {
				1.e00,
				1.e01,
				1.e02,
				1.e03,
				1.e04,
				1.e05,
				1.e06,
				1.e07,
				1.e08,
				1.e09,
				1.e10,
				1.e11,
				1.e12,
				1.e13,
				1.e14,
				1.e15,
				1.e16,
				1.e17,
				1.e18,
				1.e19,
				1.e20,
				1.e21,
				1.e22,
				1.e23,
				1.e24,
				1.e25,
				1.e26,
				1.e27,
				1.e28,
				1.e29,
				1.e30,
				1.e31,
			};

			assert((scale >= 0) && (scale < 320));

			const auto upper = UPPER_PART[scale >> 5];
			const auto lower = LOWER_PART[scale & 0x1F];

			return upper * lower;
		}

		template <typename T>
		void adjustScale(T& val, int scale, const T minLimit, const T maxLimit)
		{
			if (scale > 0)
			{
				int fraction = 0;

				do
				{
					if (scale == 1)
						fraction = int(val % 10);
					val /= 10;
				} while (--scale);

				if (fraction > 4)
					++val;
				else if (fraction < -4)
				{
					static_assert((-85 / 10 == -8) && (-85 % 10 == -5),
						"If we port to a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)), we'll have to change "
						"this depending on the platform");
					--val;
				}
			}
			else if (scale < 0)
			{
				do
				{
					if ((val > maxLimit / 10) || (val < minLimit / 10))
						throwNumericOutOfRange();

					val *= 10;

					if (val > maxLimit || val < minLimit)
						throwNumericOutOfRange();
				} while (++scale);
			}
		}

		template <typename T>
		T conversionEpsilon()
		{
			if constexpr (std::is_same_v<T, float>)
				return static_cast<T>(1e-5f);
			else if constexpr (std::is_same_v<T, double>)
				return static_cast<T>(1e-14);
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
			else if constexpr (std::same_as<T, BoostDecFloat16> || std::same_as<T, BoostDecFloat34>)
			{
				const auto epsilon = std::numeric_limits<T>::epsilon();
				return static_cast<T>(epsilon * static_cast<T>(10));
			}
#endif
			else
				return std::numeric_limits<T>::epsilon();
		}

	private:
		Client* client;
	};
}  // namespace fbcpp::impl


#endif  // FBCPP_NUMERIC_CONVERTER_H
