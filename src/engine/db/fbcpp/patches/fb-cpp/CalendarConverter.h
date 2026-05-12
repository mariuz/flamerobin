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

#ifndef FBCPP_CALENDAR_CONVERTER_H
#define FBCPP_CALENDAR_CONVERTER_H

#include "config.h"
#include "fb-api.h"
#include "Client.h"
#include "Exception.h"
#include "types.h"
#include <array>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <format>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>


namespace fbcpp::impl
{
	// FIXME: review methods
	class CalendarConverter final
	{
	public:
		explicit CalendarConverter(Client& client)
			: client{&client}
		{
		}

	public:
		OpaqueDate dateToOpaqueDate(const Date& date)
		{
			if (!date.ok())
				throwInvalidDateValue();

			const auto yearValue = static_cast<unsigned>(static_cast<int>(date.year()));
			const auto monthValue = static_cast<unsigned>(date.month());
			const auto dayValue = static_cast<unsigned>(date.day());

			if (yearValue <= 0)
				throwInvalidDateValue();

			return OpaqueDate{client->getUtil()->encodeDate(static_cast<unsigned>(yearValue), monthValue, dayValue)};
		}

		Date opaqueDateToDate(OpaqueDate date)
		{
			unsigned year;
			unsigned month;
			unsigned day;

			client->getUtil()->decodeDate(date.value, &year, &month, &day);

			return Date{std::chrono::year{static_cast<int>(year)}, std::chrono::month{month}, std::chrono::day{day}};
		}

		Date stringToDate(std::string_view value)
		{
			static const std::regex pattern(R"(^\s*([0-9]{4})\s*-\s*([0-9]{2})\s*-\s*([0-9]{2})\s*$)");

			const std::string stringValue{value};

			std::smatch matches;
			if (!std::regex_match(stringValue, matches, pattern))
				throwConversionErrorFromString(std::string{value});

			const auto makeComponentView = [&](const std::size_t index)
			{
				return std::string_view{stringValue.data() + static_cast<std::size_t>(matches.position(index)),
					static_cast<std::size_t>(matches.length(index))};
			};

			const auto parseComponent = [&](const std::size_t index)
			{
				int result;
				const auto component = makeComponentView(index);
				const auto [ptr, ec] =
					std::from_chars(component.data(), component.data() + component.size(), result, 10);

				if (ec != std::errc{} || ptr != component.data() + component.size())
					throwConversionErrorFromString(std::string{value});

				return result;
			};

			const int year = parseComponent(1);
			const unsigned month = static_cast<unsigned>(parseComponent(2));
			const unsigned day = static_cast<unsigned>(parseComponent(3));

			const Date date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};

			if (!date.ok())
				throwInvalidDateValue();

			return date;
		}

		OpaqueDate stringToOpaqueDate(std::string_view value)
		{
			return dateToOpaqueDate(stringToDate(value));
		}

		std::string opaqueDateToString(OpaqueDate date)
		{
			return std::format("{:%Y-%m-%d}", std::chrono::local_days{opaqueDateToDate(date)});
		}

		OpaqueTime timeToOpaqueTime(const Time& time)
		{
			const auto hours = static_cast<unsigned>(time.hours().count());
			const auto minutes = static_cast<unsigned>(time.minutes().count());
			const auto seconds = static_cast<unsigned>(time.seconds().count());
			const auto subseconds = static_cast<unsigned>(time.subseconds().count() / 100);

			OpaqueTime opaqueTime;
			opaqueTime.value = client->getUtil()->encodeTime(hours, minutes, seconds, subseconds);

			return opaqueTime;
		}

		Time opaqueTimeToTime(OpaqueTime time)
		{
			unsigned hours;
			unsigned minutes;
			unsigned seconds;
			unsigned subseconds;

			const auto util = client->getUtil();
			util->decodeTime(time.value, &hours, &minutes, &seconds, &subseconds);

			const auto timeOfDay = std::chrono::hours{hours} + std::chrono::minutes{minutes} +
				std::chrono::seconds{seconds} + std::chrono::microseconds{static_cast<std::int64_t>(subseconds) * 100};

			return Time{timeOfDay};
		}

		Time stringToTime(std::string_view value)
		{
			static const std::regex pattern(
				R"(^\s*([0-9]{2})\s*:\s*([0-9]{2})\s*:\s*([0-9]{2})(?:\s*\.\s*([0-9]{1,4}))?\s*$)");

			const std::string stringValue{value};

			std::smatch matches;
			if (!std::regex_match(stringValue, matches, pattern))
				throwConversionErrorFromString(std::string{value});

			const auto makeComponentView = [&](const std::size_t index)
			{
				return std::string_view{stringValue.data() + static_cast<std::size_t>(matches.position(index)),
					static_cast<std::size_t>(matches.length(index))};
			};

			const auto parseComponent = [&](const std::size_t index)
			{
				unsigned result;
				const auto component = makeComponentView(index);
				const auto [ptr, ec] =
					std::from_chars(component.data(), component.data() + component.size(), result, 10);

				if (ec != std::errc{} || ptr != component.data() + component.size())
					throwConversionErrorFromString(std::string{value});

				return result;
			};

			const auto hours = parseComponent(1);
			const auto minutes = parseComponent(2);
			const auto seconds = parseComponent(3);

			unsigned fractions = 0;
			if (matches[4].matched && matches.length(4) > 0)
			{
				const auto fractionComponent = makeComponentView(4);
				const auto [ptr, ec] = std::from_chars(
					fractionComponent.data(), fractionComponent.data() + fractionComponent.size(), fractions, 10);

				if (ec != std::errc{} || ptr != fractionComponent.data() + fractionComponent.size())
					throwConversionErrorFromString(std::string{value});

				for (auto remaining = 4 - static_cast<int>(fractionComponent.size()); remaining > 0; --remaining)
					fractions *= 10;
			}

			if (hours >= 24 || minutes >= 60 || seconds >= 60)
				throwInvalidTimeValue();

			const auto timeOfDay = std::chrono::hours{hours} + std::chrono::minutes{minutes} +
				std::chrono::seconds{seconds} + std::chrono::microseconds{static_cast<std::int64_t>(fractions) * 100};

			if (timeOfDay >= std::chrono::hours{24})
				throwInvalidTimeValue();

			return Time{std::chrono::duration_cast<std::chrono::microseconds>(timeOfDay)};
		}

		OpaqueTime stringToOpaqueTime(std::string_view value)
		{
			return timeToOpaqueTime(stringToTime(value));
		}

		std::string opaqueTimeToString(OpaqueTime time)
		{
			const auto converted = opaqueTimeToTime(time);
			const auto subseconds = static_cast<unsigned>(converted.subseconds().count() / 100);

			return std::format("{:02}:{:02}:{:02}.{:04}", static_cast<unsigned>(converted.hours().count()),
				static_cast<unsigned>(converted.minutes().count()), static_cast<unsigned>(converted.seconds().count()),
				subseconds);
		}

		OpaqueTimeTz timeTzToOpaqueTimeTz(StatusWrapper* statusWrapper, const TimeTz& timeTz)
		{
			const auto duration = timeTz.utcTime.to_duration();

			if (duration.count() < 0)
				throwInvalidTimeValue();

			const auto dayDuration = std::chrono::hours{24};
			if (duration >= dayDuration)
				throwInvalidTimeValue();

			if (duration.count() % 100 != 0)
				throwInvalidTimeValue();

			OpaqueTimeTz opaque{};

			client->getUtil()->encodeTimeTz(statusWrapper, &opaque.value, 0u, 0u, 0u, 0u, timeTz.zone.c_str());

			opaque.value.utc_time = static_cast<ISC_TIME>(duration.count() / 100);

			return opaque;
		}

		TimeTz opaqueTimeTzToTimeTz(
			StatusWrapper* statusWrapper, const OpaqueTimeTz& opaqueTime, std::string* decodedTimeZoneName = nullptr)
		{
			const auto ticks = static_cast<std::int64_t>(opaqueTime.value.utc_time) * 100;

			unsigned hours;
			unsigned minutes;
			unsigned seconds;
			unsigned fractions;
			std::array<char, 128> timeZoneBuffer;

			client->getUtil()->decodeTimeTz(statusWrapper, &opaqueTime.value, &hours, &minutes, &seconds, &fractions,
				static_cast<unsigned>(timeZoneBuffer.size()), timeZoneBuffer.data());

			TimeTz timeTz;
			timeTz.utcTime = Time{std::chrono::microseconds{ticks}};
			timeTz.zone = timeZoneBuffer.data();

			if (decodedTimeZoneName)
				*decodedTimeZoneName = timeTz.zone;

			return timeTz;
		}

		OpaqueTimeTz stringToOpaqueTimeTz(StatusWrapper* statusWrapper, std::string_view value)
		{
			return timeTzToOpaqueTimeTz(statusWrapper, stringToTimeTz(statusWrapper, value));
		}

		std::string opaqueTimeTzToString(StatusWrapper* statusWrapper, const OpaqueTimeTz& time)
		{
			unsigned hours;
			unsigned minutes;
			unsigned seconds;
			unsigned fractions;
			std::array<char, 128> timeZoneBuffer;

			client->getUtil()->decodeTimeTz(statusWrapper, &time.value, &hours, &minutes, &seconds, &fractions,
				static_cast<unsigned>(timeZoneBuffer.size()), timeZoneBuffer.data());

			return std::format("{:02}:{:02}:{:02}.{:04} {}", hours, minutes, seconds, fractions, timeZoneBuffer.data());
		}

		TimeTz stringToTimeTz(StatusWrapper* statusWrapper, std::string_view value)
		{
			static const std::regex pattern(
				R"(^\s*([0-9]{2})\s*:\s*([0-9]{2})\s*:\s*([0-9]{2})(?:\s*\.\s*([0-9]{1,4}))?\s+([^\s]+)\s*$)");

			const std::string stringValue{value};

			std::smatch matches;
			if (!std::regex_match(stringValue, matches, pattern))
				throwConversionErrorFromString(std::string{value});

			const auto makeComponentView = [&](const std::size_t index)
			{
				return std::string_view{stringValue.data() + static_cast<std::size_t>(matches.position(index)),
					static_cast<std::size_t>(matches.length(index))};
			};

			const auto parseComponent = [&](const std::size_t index)
			{
				unsigned result;
				const auto component = makeComponentView(index);
				const auto [ptr, ec] =
					std::from_chars(component.data(), component.data() + component.size(), result, 10);

				if (ec != std::errc{} || ptr != component.data() + component.size())
					throwConversionErrorFromString(std::string{value});

				return result;
			};

			const auto hours = parseComponent(1);
			const auto minutes = parseComponent(2);
			const auto seconds = parseComponent(3);

			unsigned fractions = 0;
			if (matches[4].matched && matches.length(4) > 0)
			{
				const auto fractionComponent = makeComponentView(4);
				const auto [ptr, ec] = std::from_chars(
					fractionComponent.data(), fractionComponent.data() + fractionComponent.size(), fractions, 10);

				if (ec != std::errc{} || ptr != fractionComponent.data() + fractionComponent.size())
					throwConversionErrorFromString(std::string{value});

				for (auto remaining = 4 - static_cast<int>(fractionComponent.size()); remaining > 0; --remaining)
					fractions *= 10;
			}

			if (hours >= 24 || minutes >= 60 || seconds >= 60)
				throwInvalidTimeValue();

			const auto timeOfDay = std::chrono::hours{hours} + std::chrono::minutes{minutes} +
				std::chrono::seconds{seconds} + std::chrono::microseconds{static_cast<std::int64_t>(fractions) * 100};

			if (timeOfDay >= std::chrono::hours{24})
				throwInvalidTimeValue();

			OpaqueTimeTz encoded;
			const std::string timeZoneString{makeComponentView(5)};
			client->getUtil()->encodeTimeTz(
				statusWrapper, &encoded.value, hours, minutes, seconds, fractions, timeZoneString.c_str());

			return opaqueTimeTzToTimeTz(statusWrapper, encoded);
		}

		// FIXME: review
		OpaqueTimestamp timestampToOpaqueTimestamp(const Timestamp& timestamp)
		{
			const auto& date = timestamp.date;
			if (!date.ok())
				throwInvalidTimestampValue();

			const auto opaqueDate = dateToOpaqueDate(date);

			const auto timeOfDay = timestamp.time.to_duration();
			if (timeOfDay.count() < 0 || timeOfDay >= std::chrono::hours{24})
				throwInvalidTimestampValue();

			if (timestamp.time.is_negative())
				throwInvalidTimestampValue();

			const auto subseconds = timestamp.time.subseconds().count();
			if (subseconds % 100 != 0)
				throwInvalidTimestampValue();

			OpaqueTimestamp opaqueTimestamp;
			opaqueTimestamp.value.timestamp_date = opaqueDate.value;
			opaqueTimestamp.value.timestamp_time =
				client->getUtil()->encodeTime(static_cast<unsigned>(timestamp.time.hours().count()),
					static_cast<unsigned>(timestamp.time.minutes().count()),
					static_cast<unsigned>(timestamp.time.seconds().count()), static_cast<unsigned>(subseconds / 100));

			return opaqueTimestamp;
		}

		Timestamp opaqueTimestampToTimestamp(OpaqueTimestamp timestamp)
		{
			unsigned year;
			unsigned month;
			unsigned day;
			unsigned hours;
			unsigned minutes;
			unsigned seconds;
			unsigned subseconds;

			const auto util = client->getUtil();
			util->decodeDate(timestamp.value.timestamp_date, &year, &month, &day);
			util->decodeTime(timestamp.value.timestamp_time, &hours, &minutes, &seconds, &subseconds);

			const auto timeOfDay = std::chrono::hours{hours} + std::chrono::minutes{minutes} +
				std::chrono::seconds{seconds} + std::chrono::microseconds{static_cast<std::int64_t>(subseconds) * 100};

			const Date date{
				std::chrono::year{static_cast<int>(year)}, std::chrono::month{month}, std::chrono::day{day}};

			if (!date.ok())
				throwInvalidTimestampValue();

			return Timestamp{date, Time{timeOfDay}};
		}

		Timestamp stringToTimestamp(std::string_view value)
		{
			static const std::regex pattern(
				R"(^\s*([0-9]{4})\s*-\s*([0-9]{2})\s*-\s*([0-9]{2})\s+([0-9]{2})\s*:\s*([0-9]{2})\s*:\s*([0-9]{2})(?:\s*\.\s*([0-9]{1,4}))?\s*$)");

			const std::string stringValue{value};

			std::smatch matches;
			if (!std::regex_match(stringValue, matches, pattern))
				throwConversionErrorFromString(std::string{value});

			const auto makeComponentView = [&](const std::size_t index)
			{
				return std::string_view{stringValue.data() + static_cast<std::size_t>(matches.position(index)),
					static_cast<std::size_t>(matches.length(index))};
			};

			const auto parseComponent = [&](const std::size_t index)
			{
				unsigned result;
				const auto component = makeComponentView(index);
				const auto [ptr, ec] =
					std::from_chars(component.data(), component.data() + component.size(), result, 10);

				if (ec != std::errc{} || ptr != component.data() + component.size())
					throwConversionErrorFromString(std::string{value});

				return result;
			};

			const int year = static_cast<int>(parseComponent(1));
			const unsigned month = parseComponent(2);
			const unsigned day = parseComponent(3);
			const auto hours = parseComponent(4);
			const auto minutes = parseComponent(5);
			const auto seconds = parseComponent(6);

			unsigned fractions = 0;
			if (matches[7].matched && matches.length(7) > 0)
			{
				const auto fractionComponent = makeComponentView(7);
				const auto [ptr, ec] = std::from_chars(
					fractionComponent.data(), fractionComponent.data() + fractionComponent.size(), fractions, 10);

				if (ec != std::errc{} || ptr != fractionComponent.data() + fractionComponent.size())
					throwConversionErrorFromString(std::string{value});

				for (auto remaining = 4 - static_cast<int>(fractionComponent.size()); remaining > 0; --remaining)
					fractions *= 10;
			}

			const Date date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};

			if (!date.ok())
				throwInvalidTimestampValue();

			if (hours >= 24 || minutes >= 60 || seconds >= 60)
				throwInvalidTimestampValue();

			const auto timeOfDay = std::chrono::hours{hours} + std::chrono::minutes{minutes} +
				std::chrono::seconds{seconds} + std::chrono::microseconds{static_cast<std::int64_t>(fractions) * 100};

			if (timeOfDay >= std::chrono::hours{24})
				throwInvalidTimestampValue();

			return Timestamp{date, Time{timeOfDay}};
		}

		OpaqueTimestamp stringToOpaqueTimestamp(std::string_view value)
		{
			return timestampToOpaqueTimestamp(stringToTimestamp(value));
		}

		std::string opaqueTimestampToString(OpaqueTimestamp timestamp)
		{
			const auto converted = opaqueTimestampToTimestamp(timestamp);
			const auto subseconds = static_cast<unsigned>(converted.time.subseconds().count() / 100);

			const auto dateString = std::format("{:%Y-%m-%d}", std::chrono::local_days{converted.date});
			const auto timeString =
				std::format("{:02}:{:02}:{:02}.{:04}", static_cast<unsigned>(converted.time.hours().count()),
					static_cast<unsigned>(converted.time.minutes().count()),
					static_cast<unsigned>(converted.time.seconds().count()), subseconds);

			return std::format("{} {}", dateString, timeString);
		}

		OpaqueTimestampTz timestampTzToOpaqueTimestampTz(StatusWrapper* statusWrapper, const TimestampTz& timestampTz)
		{
			OpaqueTimestampTz opaque;

			client->getUtil()->encodeTimeStampTz(
				statusWrapper, &opaque.value, 1u, 1u, 1u, 0u, 0u, 0u, 0u, timestampTz.zone.c_str());

			const auto utcOpaque = timestampToOpaqueTimestamp(timestampTz.utcTimestamp);
			opaque.value.utc_timestamp = utcOpaque.value;

			return opaque;
		}

		TimestampTz opaqueTimestampTzToTimestampTz(StatusWrapper* statusWrapper,
			const OpaqueTimestampTz& opaqueTimestamp, std::string* decodedTimeZoneName = nullptr)
		{
			const auto ticks =
				(static_cast<std::int64_t>(opaqueTimestamp.value.utc_timestamp.timestamp_date) * TICKS_PER_DAY +
					static_cast<std::int64_t>(opaqueTimestamp.value.utc_timestamp.timestamp_time)) *
				100;

			unsigned year;
			unsigned month;
			unsigned day;
			unsigned hours;
			unsigned minutes;
			unsigned seconds;
			unsigned subseconds;
			std::array<char, 128> timeZoneBuffer;

			client->getUtil()->decodeTimeStampTz(statusWrapper, &opaqueTimestamp.value, &year, &month, &day, &hours,
				&minutes, &seconds, &subseconds, static_cast<unsigned>(timeZoneBuffer.size()), timeZoneBuffer.data());

			TimestampTz timestampTz;
			const auto utcLocalTime = BASE_EPOCH + std::chrono::microseconds{ticks};
			timestampTz.utcTimestamp = Timestamp::fromLocalTime(utcLocalTime);
			timestampTz.zone = timeZoneBuffer.data();

			if (decodedTimeZoneName)
				*decodedTimeZoneName = timestampTz.zone;

			return timestampTz;
		}

		OpaqueTimestampTz stringToOpaqueTimestampTz(StatusWrapper* statusWrapper, std::string_view value)
		{
			return timestampTzToOpaqueTimestampTz(statusWrapper, stringToTimestampTz(statusWrapper, value));
		}

		std::string opaqueTimestampTzToString(StatusWrapper* statusWrapper, const OpaqueTimestampTz& timestamp)
		{
			unsigned year;
			unsigned month;
			unsigned day;
			unsigned hours;
			unsigned minutes;
			unsigned seconds;
			unsigned subseconds;
			std::array<char, 128> timeZoneBuffer;

			client->getUtil()->decodeTimeStampTz(statusWrapper, &timestamp.value, &year, &month, &day, &hours, &minutes,
				&seconds, &subseconds, static_cast<unsigned>(timeZoneBuffer.size()), timeZoneBuffer.data());

			return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:04} {}", year, month, day, hours, minutes,
				seconds, subseconds, timeZoneBuffer.data());
		}

		TimestampTz stringToTimestampTz(StatusWrapper* statusWrapper, std::string_view value)
		{
			static const std::regex pattern(
				R"(^\s*([0-9]{4})\s*-\s*([0-9]{2})\s*-\s*([0-9]{2})\s+([0-9]{2})\s*:\s*([0-9]{2})\s*:\s*([0-9]{2})(?:\s*\.\s*([0-9]{1,4}))?\s+([^\s]+)\s*$)");

			const std::string stringValue{value};

			std::smatch matches;
			if (!std::regex_match(stringValue, matches, pattern))
				throwConversionErrorFromString(std::string{value});

			const auto makeComponentView = [&](const std::size_t index)
			{
				return std::string_view{stringValue.data() + static_cast<std::size_t>(matches.position(index)),
					static_cast<std::size_t>(matches.length(index))};
			};

			const auto parseComponent = [&](const std::size_t index)
			{
				unsigned result;
				const auto component = makeComponentView(index);
				const auto [ptr, ec] =
					std::from_chars(component.data(), component.data() + component.size(), result, 10);

				if (ec != std::errc{} || ptr != component.data() + component.size())
					throwConversionErrorFromString(std::string{value});

				return result;
			};

			const int year = static_cast<int>(parseComponent(1));
			const unsigned month = parseComponent(2);
			const unsigned day = parseComponent(3);
			const auto hours = parseComponent(4);
			const auto minutes = parseComponent(5);
			const auto seconds = parseComponent(6);

			unsigned fractions = 0;
			if (matches[7].matched && matches.length(7) > 0)
			{
				const auto fractionComponent = makeComponentView(7);
				const auto [ptr, ec] = std::from_chars(
					fractionComponent.data(), fractionComponent.data() + fractionComponent.size(), fractions, 10);

				if (ec != std::errc{} || ptr != fractionComponent.data() + fractionComponent.size())
					throwConversionErrorFromString(std::string{value});

				for (auto remaining = 4 - static_cast<int>(fractionComponent.size()); remaining > 0; --remaining)
					fractions *= 10;
			}

			const Date date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};

			if (!date.ok())
				throwInvalidTimestampValue();

			if (hours >= 24 || minutes >= 60 || seconds >= 60)
				throwInvalidTimestampValue();

			const auto timeOfDay = std::chrono::hours{hours} + std::chrono::minutes{minutes} +
				std::chrono::seconds{seconds} + std::chrono::microseconds{static_cast<std::int64_t>(fractions) * 100};

			if (timeOfDay >= std::chrono::hours{24})
				throwInvalidTimestampValue();

			const Timestamp localTimestamp{date, Time{timeOfDay}};

			const auto monthValue = static_cast<unsigned>(date.month());
			const auto dayValue = static_cast<unsigned>(date.day());

			OpaqueTimestampTz encoded;
			const std::string timeZoneString{makeComponentView(8)};
			client->getUtil()->encodeTimeStampTz(statusWrapper, &encoded.value,
				static_cast<unsigned>(static_cast<int>(date.year())), monthValue, dayValue, hours, minutes, seconds,
				fractions, timeZoneString.c_str());

			const OpaqueTimestamp utcOpaque{encoded.value.utc_timestamp};
			const auto utcTimestamp = opaqueTimestampToTimestamp(utcOpaque);

			const auto offsetDuration = localTimestamp.toLocalTime() - utcTimestamp.toLocalTime();
			if (offsetDuration % std::chrono::minutes{1} != std::chrono::microseconds::zero())
				throwInvalidTimestampValue();

			std::string resolvedTimeZoneName;
			opaqueTimestampTzToTimestampTz(statusWrapper, encoded, &resolvedTimeZoneName);

			return TimestampTz{utcTimestamp, resolvedTimeZoneName};
		}

	private:
		[[noreturn]] void throwConversionErrorFromString(const std::string& str)
		{
			const std::intptr_t STATUS_CONVERSION_ERROR_FROM_STRING[] = {
				isc_convert_error,
				reinterpret_cast<std::intptr_t>(str.c_str()),
				isc_arg_end,
			};

			throw DatabaseException(*client, STATUS_CONVERSION_ERROR_FROM_STRING);
		}

		[[noreturn]] void throwInvalidDateValue()
		{
			static constexpr std::intptr_t STATUS_INVALID_DATE_VALUE[] = {
				isc_invalid_date_val,
				isc_arg_end,
			};

			throw DatabaseException(*client, STATUS_INVALID_DATE_VALUE);
		}

		[[noreturn]] void throwInvalidTimeValue()
		{
			static constexpr std::intptr_t STATUS_INVALID_TIME_VALUE[] = {
				isc_invalid_time_val,
				isc_arg_end,
			};

			throw DatabaseException(*client, STATUS_INVALID_TIME_VALUE);
		}

		[[noreturn]] void throwInvalidTimestampValue()
		{
			static constexpr std::intptr_t STATUS_INVALID_TIMESTAMP_VALUE[] = {
				isc_invalid_timestamp_val,
				isc_arg_end,
			};

			throw DatabaseException(*client, STATUS_INVALID_TIMESTAMP_VALUE);
		}

	private:
		static constexpr auto TICKS_PER_DAY = std::int64_t{24} * 60 * 60 * 10000;
		static constexpr auto BASE_EPOCH = std::chrono::local_days{
			std::chrono::year{1858} / std::chrono::November / 17,
		};
		Client* client;
	};
}  // namespace fbcpp::impl


#endif  // FBCPP_CALENDAR_CONVERTER_H
