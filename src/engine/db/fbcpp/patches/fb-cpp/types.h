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

#ifndef FBCPP_TYPES_H
#define FBCPP_TYPES_H

#include "fb-api.h"
#include "config.h"
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Represents a numeric value with an explicit decimal scale.
	///
	template <typename T>
	struct ScaledNumber final
	{
		bool operator==(const ScaledNumber&) const noexcept = default;

		///
		/// Unscaled numeric value.
		///
		T value{};

		///
		/// Decimal scale applied to `value`.
		///
		int scale = 0;
	};

	///
	/// Signed 16-bit scaled number.
	///
	using ScaledInt16 = ScaledNumber<std::int16_t>;

	///
	/// Signed 32-bit scaled number.
	///
	using ScaledInt32 = ScaledNumber<std::int32_t>;

	///
	/// Signed 64-bit scaled number.
	///
	using ScaledInt64 = ScaledNumber<std::int64_t>;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	///
	/// 128-bit integer using Boost.Multiprecision.
	///
	using BoostInt128 = boost::multiprecision::int128_t;

	///
	/// Scaled 128-bit integer backed by Boost.Multiprecision.
	///
	using ScaledBoostInt128 = ScaledNumber<BoostInt128>;
#endif

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	///
	/// 16-digit decimal floating point using Boost.Multiprecision.
	///
	using BoostDecFloat16 = boost::multiprecision::number<boost::multiprecision::cpp_dec_float<16>>;

	///
	/// 34-digit decimal floating point using Boost.Multiprecision.
	///
	using BoostDecFloat34 = boost::multiprecision::number<boost::multiprecision::cpp_dec_float<34>>;
#endif

	///
	/// Firebird SQL calendar date.
	///
	using Date = std::chrono::year_month_day;

	///
	/// Firebird SQL time-of-day with microsecond resolution.
	///
	using Time = std::chrono::hh_mm_ss<std::chrono::microseconds>;

	///
	/// Combined date and time with microsecond precision.
	///
	struct Timestamp final
	{
		bool operator==(const Timestamp& o) const noexcept
		{
			return date == o.date && time.to_duration() == o.time.to_duration();
		}

		///
		/// Converts to std::chrono::local_time<std::chrono::microseconds>.
		/// Important: in Windows, std::chrono::local_time<std::chrono::microseconds> cannot represent all the valid
		/// date range supported by Firebird.
		///
		std::chrono::local_time<std::chrono::microseconds> toLocalTime() const noexcept
		{
			return std::chrono::local_days{date} + time.to_duration();
		}

		///
		/// Builds a timestamp from a local-time value.
		///
		static Timestamp fromLocalTime(std::chrono::local_time<std::chrono::microseconds> value) noexcept
		{
			const auto days = std::chrono::floor<std::chrono::days>(value);
			const Date dateValue{days};
			const auto timeOfDay = std::chrono::duration_cast<std::chrono::microseconds>(value - days);

			return Timestamp{dateValue, Time{timeOfDay}};
		}

		///
		/// Calendar date component.
		///
		Date date{};

		///
		/// Time-of-day component.
		///
		Time time{std::chrono::microseconds::zero()};
	};

	///
	/// Local time bound to a time zone.
	///
	struct TimeTz final
	{
		bool operator==(const TimeTz& o) const noexcept
		{
			return utcTime.to_duration() == o.utcTime.to_duration() && zone == o.zone;
		}

		///
		/// UTC-normalised time-of-day.
		///
		Time utcTime;

		///
		/// Time zone identifier.
		///
		std::string zone;
	};

	///
	/// Timestamp bound to a time zone.
	///
	struct TimestampTz final
	{
		bool operator==(const TimestampTz&) const noexcept = default;

		///
		/// UTC-normalised timestamp.
		///
		Timestamp utcTimestamp;

		///
		/// Time zone identifier.
		///
		std::string zone;
	};

	///
	/// Opaque 128-bit integer exposed by the Firebird API.
	///
	using OpaqueInt128 = FB_I128;

	///
	/// Opaque 16-digit decimal floating point exposed by the Firebird API.
	///
	using OpaqueDecFloat16 = FB_DEC16;

	///
	/// Opaque 34-digit decimal floating point exposed by the Firebird API.
	///
	using OpaqueDecFloat34 = FB_DEC34;

	///
	/// Scaled Firebird opaque 128-bit integer.
	///
	using ScaledOpaqueInt128 = ScaledNumber<OpaqueInt128>;

	///
	/// Wrapper for Firebird date values.
	///
	struct alignas(alignof(ISC_DATE)) OpaqueDate final
	{
		bool operator==(const OpaqueDate&) const noexcept = default;

		///
		/// Raw Firebird date representation.
		///
		ISC_DATE value;
	};

	///
	/// Wrapper for Firebird time values.
	///
	struct alignas(alignof(ISC_TIME)) OpaqueTime final
	{
		bool operator==(const OpaqueTime&) const noexcept = default;

		///
		/// Raw Firebird time representation.
		///
		ISC_TIME value;
	};

	///
	/// Wrapper for Firebird timestamp values.
	///
	struct alignas(alignof(ISC_TIMESTAMP)) OpaqueTimestamp final
	{
		bool operator==(const OpaqueTimestamp& o) const noexcept
		{
			return value.timestamp_date == o.value.timestamp_date && value.timestamp_time == o.value.timestamp_time;
		}

		///
		/// Raw Firebird timestamp representation.
		///
		ISC_TIMESTAMP value;
	};

	///
	/// Wrapper for Firebird time-with-time-zone values.
	///
	struct alignas(alignof(ISC_TIME_TZ)) OpaqueTimeTz final
	{
		bool operator==(const OpaqueTimeTz& o) const noexcept
		{
			return value.utc_time == o.value.utc_time && value.time_zone == o.value.time_zone;
		}

		///
		/// Raw Firebird time-with-time-zone representation.
		///
		ISC_TIME_TZ value;
	};

	///
	/// Wrapper for Firebird timestamp-with-time-zone values.
	///
	struct alignas(alignof(ISC_TIMESTAMP_TZ)) OpaqueTimestampTz final
	{
		bool operator==(const OpaqueTimestampTz& o) const noexcept
		{
			return value.utc_timestamp.timestamp_date == o.value.utc_timestamp.timestamp_date &&
				value.utc_timestamp.timestamp_time == o.value.utc_timestamp.timestamp_time &&
				value.time_zone == o.value.time_zone;
		}

		///
		/// Raw Firebird timestamp-with-time-zone representation.
		///
		ISC_TIMESTAMP_TZ value;
	};

	// FIXME: test
	///
	/// Stream insertion helper that renders the scaled number as `value` followed by `e` and the scale.
	///
	template <typename T>
	std::ostream& operator<<(std::ostream& os, const fbcpp::ScaledNumber<T>& scaledNumber)
	{
		os << scaledNumber.value << "e" << scaledNumber.scale;
		return os;
	}
}  // namespace fbcpp


// FIXME: test
///
/// Formatter for `fbcpp::ScaledNumber` that mirrors the stream output contract.
///
template <typename T>
struct std::formatter<fbcpp::ScaledNumber<T>> : std::formatter<T>
{
	///
	/// Parses the format specifier forwarded to the underlying formatter.
	///
	constexpr auto parse(format_parse_context& ctx)
	{
		return std::formatter<T>::parse(ctx);
	}

	///
	/// Emits the scaled number using the native formatter for the value followed by the scale suffix.
	///
	template <typename FormatContext>
	auto format(const fbcpp::ScaledNumber<T>& scaledNumber, FormatContext& ctx) const
	{
		return std::format_to(
			ctx.out(), "{}e{}", std::formatter<T>::format(scaledNumber.value, ctx), scaledNumber.scale);
	}
};


#endif  // FBCPP_TYPES_H
