/*
 * MIT License
 *
 * Copyright (c) 2026 F.D.Castel
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

#ifndef FBCPP_ROW_H
#define FBCPP_ROW_H

#include "config.h"
#include "fb-api.h"
#include "types.h"
#include "Blob.h"
#include "NumericConverter.h"
#include "CalendarConverter.h"
#include "Descriptor.h"
#include "Exception.h"
#include "StructBinding.h"
#include "VariantTypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

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
	/// @brief A lightweight, non-owning view of a single row's data with typed accessors.
	///
	/// Row provides typed access to column values in a message buffer, using
	/// descriptors to interpret the raw bytes. It is produced by RowSet for any
	/// fetched row.
	///
	class Row final
	{
	public:
		///
		/// @brief Constructs a Row view over the given message buffer.
		///
		/// @param client Client used to build conversion helpers.
		/// @param descriptors Column descriptors.
		/// @param message Span over the raw row data.
		///
		Row(Client& client, const std::vector<Descriptor>& descriptors, std::span<const std::byte> message)
			: client{&client},
			  descriptors{&descriptors},
			  message{message},
			  statusWrapper{client},
			  numericConverter{client},
			  calendarConverter{client}
		{
		}

	public:
		///
		/// @name Result reading
		/// @{

		///
		/// @brief Reports whether the row has a null at the given column.
		///
		bool isNull(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);
			return *reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE;
		}

		///
		/// @brief Reads a boolean column.
		///
		std::optional<bool> getBool(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BOOLEAN:
					return message[descriptor.offset] != std::byte{0};

				default:
					throwInvalidType("bool", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a 16-bit signed integer column.
		///
		std::optional<std::int16_t> getInt16(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<std::int16_t>(index, scale, "std::int16_t");
		}

		///
		/// @brief Reads a scaled 16-bit signed integer column.
		///
		std::optional<ScaledInt16> getScaledInt16(unsigned index)
		{
			std::optional<int> scale;
			const auto value = getNumber<std::int16_t>(index, scale, "ScaledInt16");
			return value.has_value() ? std::optional{ScaledInt16{value.value(), scale.value()}} : std::nullopt;
		}

		///
		/// @brief Reads a 32-bit signed integer column.
		///
		std::optional<std::int32_t> getInt32(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<std::int32_t>(index, scale, "std::int32_t");
		}

		///
		/// @brief Reads a scaled 32-bit signed integer column.
		///
		std::optional<ScaledInt32> getScaledInt32(unsigned index)
		{
			std::optional<int> scale;
			const auto value = getNumber<std::int32_t>(index, scale, "ScaledInt32");
			return value.has_value() ? std::optional{ScaledInt32{value.value(), scale.value()}} : std::nullopt;
		}

		///
		/// @brief Reads a 64-bit signed integer column.
		///
		std::optional<std::int64_t> getInt64(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<std::int64_t>(index, scale, "std::int64_t");
		}

		///
		/// @brief Reads a scaled 64-bit signed integer column.
		///
		std::optional<ScaledInt64> getScaledInt64(unsigned index)
		{
			std::optional<int> scale;
			const auto value = getNumber<std::int64_t>(index, scale, "ScaledInt64");
			return value.has_value() ? std::optional{ScaledInt64{value.value(), scale.value()}} : std::nullopt;
		}

		///
		/// @brief Reads a Firebird scaled 128-bit integer column.
		///
		std::optional<ScaledOpaqueInt128> getScaledOpaqueInt128(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT128:
					return ScaledOpaqueInt128{
						OpaqueInt128{*reinterpret_cast<const OpaqueInt128*>(&message[descriptor.offset])},
						descriptor.scale};

				default:
					throwInvalidType("ScaledOpaqueInt128", descriptor.adjustedType);
			}
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost 128-bit integer column.
		///
		std::optional<BoostInt128> getBoostInt128(unsigned index)
		{
			std::optional<int> scale{0};
			const auto value = getNumber<BoostInt128>(index, scale, "BoostInt128");
			return value.has_value() ? std::optional{value.value()} : std::nullopt;
		}

		///
		/// @brief Reads a scaled Boost 128-bit integer column.
		///
		std::optional<ScaledBoostInt128> getScaledBoostInt128(unsigned index)
		{
			std::optional<int> scale;
			const auto value = getNumber<BoostInt128>(index, scale, "ScaledBoostInt128");
			return value.has_value() ? std::optional{ScaledBoostInt128{value.value(), scale.value()}} : std::nullopt;
		}
#endif

		///
		/// @brief Reads a single precision floating-point column.
		///
		std::optional<float> getFloat(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<float>(index, scale, "float");
		}

		///
		/// @brief Reads a double precision floating-point column.
		///
		std::optional<double> getDouble(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<double>(index, scale, "double");
		}

		///
		/// @brief Reads a Firebird 16-digit decimal floating-point column.
		///
		std::optional<OpaqueDecFloat16> getOpaqueDecFloat16(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DECFLOAT16:
					return OpaqueDecFloat16{*reinterpret_cast<const OpaqueDecFloat16*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueDecFloat16", descriptor.adjustedType);
			}
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost-based 16-digit decimal floating-point column.
		///
		std::optional<BoostDecFloat16> getBoostDecFloat16(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<BoostDecFloat16>(index, scale, "BoostDecFloat16");
		}
#endif

		///
		/// @brief Reads a Firebird 34-digit decimal floating-point column.
		///
		std::optional<OpaqueDecFloat34> getOpaqueDecFloat34(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DECFLOAT34:
					return OpaqueDecFloat34{*reinterpret_cast<const OpaqueDecFloat34*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueDecFloat34", descriptor.adjustedType);
			}
		}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
		///
		/// @brief Reads a Boost-based 34-digit decimal floating-point column.
		///
		std::optional<BoostDecFloat34> getBoostDecFloat34(unsigned index)
		{
			std::optional<int> scale{0};
			return getNumber<BoostDecFloat34>(index, scale, "BoostDecFloat34");
		}
#endif

		///
		/// @brief Reads a date column.
		///
		std::optional<Date> getDate(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DATE:
					return calendarConverter.opaqueDateToDate(
						*reinterpret_cast<const OpaqueDate*>(&message[descriptor.offset]));

				default:
					throwInvalidType("Date", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw date column in Firebird's representation.
		///
		std::optional<OpaqueDate> getOpaqueDate(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::DATE:
					return OpaqueDate{*reinterpret_cast<const OpaqueDate*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueDate", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a time-of-day column without timezone.
		///
		std::optional<Time> getTime(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME:
					return calendarConverter.opaqueTimeToTime(
						*reinterpret_cast<const OpaqueTime*>(&message[descriptor.offset]));

				default:
					throwInvalidType("Time", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw time-of-day column in Firebird's representation.
		///
		std::optional<OpaqueTime> getOpaqueTime(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME:
					return OpaqueTime{*reinterpret_cast<const OpaqueTime*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueTime", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a timestamp column without timezone.
		///
		std::optional<Timestamp> getTimestamp(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP:
					return calendarConverter.opaqueTimestampToTimestamp(
						*reinterpret_cast<const OpaqueTimestamp*>(&message[descriptor.offset]));

				default:
					throwInvalidType("Timestamp", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw timestamp column in Firebird's representation.
		///
		std::optional<OpaqueTimestamp> getOpaqueTimestamp(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP:
					return OpaqueTimestamp{*reinterpret_cast<const OpaqueTimestamp*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueTimestamp", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a time-of-day column with timezone.
		///
		std::optional<TimeTz> getTimeTz(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME_TZ:
					return calendarConverter.opaqueTimeTzToTimeTz(
						&statusWrapper, *reinterpret_cast<const OpaqueTimeTz*>(&message[descriptor.offset]));

				default:
					throwInvalidType("TimeTz", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw time-of-day column with timezone in Firebird's representation.
		///
		std::optional<OpaqueTimeTz> getOpaqueTimeTz(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIME_TZ:
					return OpaqueTimeTz{*reinterpret_cast<const OpaqueTimeTz*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueTimeTz", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a timestamp-with-time-zone column.
		///
		std::optional<TimestampTz> getTimestampTz(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP_TZ:
					return calendarConverter.opaqueTimestampTzToTimestampTz(
						&statusWrapper, *reinterpret_cast<const OpaqueTimestampTz*>(&message[descriptor.offset]));

				default:
					throwInvalidType("TimestampTz", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a raw timestamp-with-time-zone column in Firebird's representation.
		///
		std::optional<OpaqueTimestampTz> getOpaqueTimestampTz(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::TIMESTAMP_TZ:
					return OpaqueTimestampTz{*reinterpret_cast<const OpaqueTimestampTz*>(&message[descriptor.offset])};

				default:
					throwInvalidType("OpaqueTimestampTz", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a blob identifier column.
		///
		std::optional<BlobId> getBlobId(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BLOB:
				{
					BlobId value;
					value.id = *reinterpret_cast<const ISC_QUAD*>(&message[descriptor.offset]);
					return value;
				}

				default:
					throwInvalidType("BlobId", descriptor.adjustedType);
			}
		}

		///
		/// @brief Reads a textual column, applying number-to-string conversions when needed.
		///
		std::optional<std::string> getString(unsigned index)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			const auto data = &message[descriptor.offset];

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BOOLEAN:
					return (message[descriptor.offset] != std::byte{0}) ? std::string{"true"} : std::string{"false"};

				case DescriptorAdjustedType::INT16:
					return numericConverter.numberToString(
						ScaledInt16{*reinterpret_cast<const std::int16_t*>(data), descriptor.scale});

				case DescriptorAdjustedType::INT32:
					return numericConverter.numberToString(
						ScaledInt32{*reinterpret_cast<const std::int32_t*>(data), descriptor.scale});

				case DescriptorAdjustedType::INT64:
					return numericConverter.numberToString(
						ScaledInt64{*reinterpret_cast<const std::int64_t*>(data), descriptor.scale});

				case DescriptorAdjustedType::INT128:
					return numericConverter.opaqueInt128ToString(
						&statusWrapper, *reinterpret_cast<const OpaqueInt128*>(data), descriptor.scale);

				case DescriptorAdjustedType::FLOAT:
					return numericConverter.numberToString(*reinterpret_cast<const float*>(data));

				case DescriptorAdjustedType::DOUBLE:
					return numericConverter.numberToString(*reinterpret_cast<const double*>(data));

				case DescriptorAdjustedType::DATE:
					return calendarConverter.opaqueDateToString(*reinterpret_cast<const OpaqueDate*>(data));

				case DescriptorAdjustedType::TIME:
					return calendarConverter.opaqueTimeToString(*reinterpret_cast<const OpaqueTime*>(data));

				case DescriptorAdjustedType::TIMESTAMP:
					return calendarConverter.opaqueTimestampToString(*reinterpret_cast<const OpaqueTimestamp*>(data));

				case DescriptorAdjustedType::TIME_TZ:
					return calendarConverter.opaqueTimeTzToString(
						&statusWrapper, *reinterpret_cast<const OpaqueTimeTz*>(data));

				case DescriptorAdjustedType::TIMESTAMP_TZ:
					return calendarConverter.opaqueTimestampTzToString(
						&statusWrapper, *reinterpret_cast<const OpaqueTimestampTz*>(data));

				case DescriptorAdjustedType::DECFLOAT16:
					return numericConverter.opaqueDecFloat16ToString(
						&statusWrapper, *reinterpret_cast<const OpaqueDecFloat16*>(data));

				case DescriptorAdjustedType::DECFLOAT34:
					return numericConverter.opaqueDecFloat34ToString(
						&statusWrapper, *reinterpret_cast<const OpaqueDecFloat34*>(data));

				case DescriptorAdjustedType::STRING:
					return std::string{reinterpret_cast<const char*>(data + sizeof(std::uint16_t)),
						*reinterpret_cast<const std::uint16_t*>(data)};

				default:
					throwInvalidType("std::string", descriptor.adjustedType);
			}
		}

		///
		/// @}
		///

		///
		/// @brief Retrieves a column using the most appropriate typed accessor specialization.
		///
		template <typename T>
		T get(unsigned index);

		///
		/// @brief Retrieves all output columns into a user-defined aggregate struct.
		///
		template <Aggregate T>
		T get()
		{
			using namespace impl::reflection;

			constexpr std::size_t N = fieldCountV<T>;

			if (N != descriptors->size())
			{
				throw FbCppException("Struct field count (" + std::to_string(N) +
					") does not match output column count (" + std::to_string(descriptors->size()) + ")");
			}

			return getStruct<T>(std::make_index_sequence<N>{});
		}

		///
		/// @brief Retrieves all output columns into a tuple-like type.
		///
		template <TupleLike T>
		T get()
		{
			using namespace impl::reflection;

			constexpr std::size_t N = std::tuple_size_v<T>;

			if (N != descriptors->size())
			{
				throw FbCppException("Tuple element count (" + std::to_string(N) +
					") does not match output column count (" + std::to_string(descriptors->size()) + ")");
			}

			return getTuple<T>(std::make_index_sequence<N>{});
		}

		///
		/// @brief Retrieves a column value as a user-defined variant type.
		///
		template <VariantLike V>
		V get(unsigned index)
		{
			using namespace impl::reflection;

			static_assert(variantAlternativesSupportedV<V>,
				"Variant contains unsupported types. All variant alternatives must be types supported by fb-cpp "
				"(e.g., std::int32_t, std::string, Date, ScaledOpaqueInt128, etc.). Check VariantTypeTraits.h for the "
				"complete list of supported types.");

			const auto& descriptor = getDescriptor(index);

			if (isNull(index))
			{
				if constexpr (variantContainsV<std::monostate, V>)
					return V{std::monostate{}};
				else
				{
					throw FbCppException(
						"NULL value encountered but variant does not contain std::monostate at index " +
						std::to_string(index));
				}
			}

			return getVariantValue<V>(index, descriptor);
		}

	private:
		const Descriptor& getDescriptor(unsigned index)
		{
			if (index >= descriptors->size())
				throw std::out_of_range("index out of range");

			return (*descriptors)[index];
		}

		template <typename T, std::size_t... Is>
		T getStruct(std::index_sequence<Is...>)
		{
			using namespace impl::reflection;

			return T{getStructField<FieldType<T, Is>>(static_cast<unsigned>(Is))...};
		}

		template <typename F>
		auto getStructField(unsigned index)
		{
			using namespace impl::reflection;

			if constexpr (isOptionalV<F>)
				return get<F>(index);
			else if constexpr (isVariantV<F>)
				return get<F>(index);
			else
			{
				auto opt = get<std::optional<F>>(index);

				if (!opt.has_value())
				{
					throw FbCppException(
						"Null value encountered for non-optional field at index " + std::to_string(index));
				}

				return std::move(opt.value());
			}
		}

		template <typename T, std::size_t... Is>
		T getTuple(std::index_sequence<Is...>)
		{
			using namespace impl::reflection;

			return T{getStructField<std::tuple_element_t<Is, T>>(static_cast<unsigned>(Is))...};
		}

		template <typename V>
		V getVariantValue(unsigned index, const Descriptor& descriptor)
		{
			using namespace impl::reflection;

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::BOOLEAN:
					if constexpr (variantContainsV<bool, V>)
						return V{get<std::optional<bool>>(index).value()};
					break;

				case DescriptorAdjustedType::INT16:
					if (descriptor.scale != 0)
					{
						if constexpr (variantContainsV<ScaledInt16, V>)
							return V{get<std::optional<ScaledInt16>>(index).value()};
						if constexpr (variantContainsV<ScaledInt32, V>)
							return V{get<std::optional<ScaledInt32>>(index).value()};
						if constexpr (variantContainsV<ScaledInt64, V>)
							return V{get<std::optional<ScaledInt64>>(index).value()};
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
						if constexpr (variantContainsV<ScaledBoostInt128, V>)
							return V{get<std::optional<ScaledBoostInt128>>(index).value()};
#endif
					}
					if constexpr (variantContainsV<std::int16_t, V>)
						return V{get<std::optional<std::int16_t>>(index).value()};
					break;

				case DescriptorAdjustedType::INT32:
					if (descriptor.scale != 0)
					{
						if constexpr (variantContainsV<ScaledInt32, V>)
							return V{get<std::optional<ScaledInt32>>(index).value()};
						if constexpr (variantContainsV<ScaledInt64, V>)
							return V{get<std::optional<ScaledInt64>>(index).value()};
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
						if constexpr (variantContainsV<ScaledBoostInt128, V>)
							return V{get<std::optional<ScaledBoostInt128>>(index).value()};
#endif
					}
					if constexpr (variantContainsV<std::int32_t, V>)
						return V{get<std::optional<std::int32_t>>(index).value()};
					break;

				case DescriptorAdjustedType::INT64:
					if (descriptor.scale != 0)
					{
						if constexpr (variantContainsV<ScaledInt64, V>)
							return V{get<std::optional<ScaledInt64>>(index).value()};
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
						if constexpr (variantContainsV<ScaledBoostInt128, V>)
							return V{get<std::optional<ScaledBoostInt128>>(index).value()};
#endif
					}
					if constexpr (variantContainsV<std::int64_t, V>)
						return V{get<std::optional<std::int64_t>>(index).value()};
					break;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
					if constexpr (variantContainsV<ScaledOpaqueInt128, V>)
						return V{get<std::optional<ScaledOpaqueInt128>>(index).value()};
					else if (descriptor.scale != 0)
					{
						if constexpr (variantContainsV<ScaledBoostInt128, V>)
							return V{get<std::optional<ScaledBoostInt128>>(index).value()};
					}
					else if constexpr (variantContainsV<BoostInt128, V>)
						return V{get<std::optional<BoostInt128>>(index).value()};
					break;
#endif

				case DescriptorAdjustedType::FLOAT:
					if constexpr (variantContainsV<float, V>)
						return V{get<std::optional<float>>(index).value()};
					break;

				case DescriptorAdjustedType::DOUBLE:
					if constexpr (variantContainsV<double, V>)
						return V{get<std::optional<double>>(index).value()};
					break;

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::DECFLOAT16:
					if constexpr (variantContainsV<OpaqueDecFloat16, V>)
						return V{get<std::optional<OpaqueDecFloat16>>(index).value()};
					else if constexpr (variantContainsV<BoostDecFloat16, V>)
						return V{get<std::optional<BoostDecFloat16>>(index).value()};
					break;

				case DescriptorAdjustedType::DECFLOAT34:
					if constexpr (variantContainsV<OpaqueDecFloat34, V>)
						return V{get<std::optional<OpaqueDecFloat34>>(index).value()};
					else if constexpr (variantContainsV<BoostDecFloat34, V>)
						return V{get<std::optional<BoostDecFloat34>>(index).value()};
					break;
#endif

				case DescriptorAdjustedType::STRING:
					if constexpr (variantContainsV<std::string, V>)
						return V{get<std::optional<std::string>>(index).value()};
					break;

				case DescriptorAdjustedType::DATE:
					if constexpr (variantContainsV<OpaqueDate, V>)
						return V{get<std::optional<OpaqueDate>>(index).value()};
					else if constexpr (variantContainsV<Date, V>)
						return V{get<std::optional<Date>>(index).value()};
					break;

				case DescriptorAdjustedType::TIME:
					if constexpr (variantContainsV<OpaqueTime, V>)
						return V{get<std::optional<OpaqueTime>>(index).value()};
					else if constexpr (variantContainsV<Time, V>)
						return V{get<std::optional<Time>>(index).value()};
					break;

				case DescriptorAdjustedType::TIMESTAMP:
					if constexpr (variantContainsV<OpaqueTimestamp, V>)
						return V{get<std::optional<OpaqueTimestamp>>(index).value()};
					else if constexpr (variantContainsV<Timestamp, V>)
						return V{get<std::optional<Timestamp>>(index).value()};
					break;

				case DescriptorAdjustedType::TIME_TZ:
					if constexpr (variantContainsV<OpaqueTimeTz, V>)
						return V{get<std::optional<OpaqueTimeTz>>(index).value()};
					else if constexpr (variantContainsV<TimeTz, V>)
						return V{get<std::optional<TimeTz>>(index).value()};
					break;

				case DescriptorAdjustedType::TIMESTAMP_TZ:
					if constexpr (variantContainsV<OpaqueTimestampTz, V>)
						return V{get<std::optional<OpaqueTimestampTz>>(index).value()};
					else if constexpr (variantContainsV<TimestampTz, V>)
						return V{get<std::optional<TimestampTz>>(index).value()};
					break;

				case DescriptorAdjustedType::BLOB:
					if constexpr (variantContainsV<BlobId, V>)
						return V{get<std::optional<BlobId>>(index).value()};
					break;

				default:
					break;
			}

			return tryVariantAlternatives<V, 0>(index, descriptor);
		}

		template <typename V, std::size_t I = 0>
		V tryVariantAlternatives(unsigned index, [[maybe_unused]] const Descriptor& descriptor)
		{
			using namespace impl::reflection;

			if constexpr (I >= std::variant_size_v<V>)
			{
				throw FbCppException(
					"Cannot convert SQL type to any variant alternative at index " + std::to_string(index));
			}
			else
			{
				using Alt = std::variant_alternative_t<I, V>;

				if constexpr (std::is_same_v<Alt, std::monostate>)
					return tryVariantAlternatives<V, I + 1>(index, descriptor);
				else if constexpr (isOpaqueTypeV<Alt>)
					return tryVariantAlternatives<V, I + 1>(index, descriptor);
				else
				{
					auto opt = get<std::optional<Alt>>(index);
					return V{std::move(opt.value())};
				}
			}
		}

		// FIXME: floating to integral
		template <typename T>
		std::optional<T> getNumber(unsigned index, std::optional<int>& scale, const char* typeName)
		{
			const auto& descriptor = getDescriptor(index);

			if (*reinterpret_cast<const std::int16_t*>(&message[descriptor.nullOffset]) != FB_FALSE)
				return std::nullopt;

			auto data = &message[descriptor.offset];
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
			std::optional<BoostInt128> boostInt128;
			std::optional<BoostDecFloat16> boostDecFloat16;
			std::optional<BoostDecFloat34> boostDecFloat34;
#endif

			// FIXME: Use IUtil
			switch (descriptor.adjustedType)
			{
#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
					boostInt128.emplace(
						numericConverter.opaqueInt128ToBoostInt128(*reinterpret_cast<const OpaqueInt128*>(data)));
					data = reinterpret_cast<const std::byte*>(&boostInt128.value());
					break;

				case DescriptorAdjustedType::DECFLOAT16:
					boostDecFloat16.emplace(numericConverter.opaqueDecFloat16ToBoostDecFloat16(
						&statusWrapper, *reinterpret_cast<const OpaqueDecFloat16*>(data)));
					data = reinterpret_cast<const std::byte*>(&boostDecFloat16.value());
					break;

				case DescriptorAdjustedType::DECFLOAT34:
					boostDecFloat34.emplace(numericConverter.opaqueDecFloat34ToBoostDecFloat34(
						&statusWrapper, *reinterpret_cast<const OpaqueDecFloat34*>(data)));
					data = reinterpret_cast<const std::byte*>(&boostDecFloat34.value());
					break;
#endif

				default:
					break;
			}

			return convertNumber<T>(descriptor, data, scale, typeName);
		}

		[[noreturn]] static void throwInvalidType(const char* actualType, DescriptorAdjustedType descriptorType)
		{
			throw FbCppException("Invalid type: actual type " + std::string(actualType) + ", descriptor type " +
				std::to_string(static_cast<unsigned>(descriptorType)));
		}

		template <typename T>
		T convertNumber(
			const Descriptor& descriptor, const std::byte* data, std::optional<int>& toScale, const char* toTypeName)
		{
			if (!toScale.has_value())
			{
				switch (descriptor.adjustedType)
				{
					case DescriptorAdjustedType::DECFLOAT16:
					case DescriptorAdjustedType::DECFLOAT34:
					case DescriptorAdjustedType::FLOAT:
					case DescriptorAdjustedType::DOUBLE:
						throwInvalidType(toTypeName, descriptor.adjustedType);

					default:
						break;
				}

				toScale = descriptor.scale;
			}

			switch (descriptor.adjustedType)
			{
				case DescriptorAdjustedType::INT16:
					return numericConverter.numberToNumber<T>(
						ScaledInt16{*reinterpret_cast<const std::int16_t*>(data), descriptor.scale}, toScale.value());

				case DescriptorAdjustedType::INT32:
					return numericConverter.numberToNumber<T>(
						ScaledInt32{*reinterpret_cast<const std::int32_t*>(data), descriptor.scale}, toScale.value());

				case DescriptorAdjustedType::INT64:
					return numericConverter.numberToNumber<T>(
						ScaledInt64{*reinterpret_cast<const std::int64_t*>(data), descriptor.scale}, toScale.value());

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
				case DescriptorAdjustedType::INT128:
					return numericConverter.numberToNumber<T>(
						ScaledBoostInt128{*reinterpret_cast<const BoostInt128*>(data), descriptor.scale},
						toScale.value());

				case DescriptorAdjustedType::DECFLOAT16:
					return numericConverter.numberToNumber<T>(
						*reinterpret_cast<const BoostDecFloat16*>(data), toScale.value());

				case DescriptorAdjustedType::DECFLOAT34:
					return numericConverter.numberToNumber<T>(
						*reinterpret_cast<const BoostDecFloat34*>(data), toScale.value());
#endif

				case DescriptorAdjustedType::FLOAT:
					return numericConverter.numberToNumber<T>(*reinterpret_cast<const float*>(data), toScale.value());

				case DescriptorAdjustedType::DOUBLE:
					return numericConverter.numberToNumber<T>(*reinterpret_cast<const double*>(data), toScale.value());

				default:
					throwInvalidType(toTypeName, descriptor.adjustedType);
			}
		}

	private:
		Client* client;
		const std::vector<Descriptor>* descriptors;
		std::span<const std::byte> message;
		impl::StatusWrapper statusWrapper;
		impl::NumericConverter numericConverter;
		impl::CalendarConverter calendarConverter;
	};

	///
	/// @name Row convenience template specializations
	/// @{
	///

	template <>
	inline std::optional<bool> Row::get<std::optional<bool>>(unsigned index)
	{
		return getBool(index);
	}

	template <>
	inline std::optional<BlobId> Row::get<std::optional<BlobId>>(unsigned index)
	{
		return getBlobId(index);
	}

	template <>
	inline std::optional<std::int16_t> Row::get<std::optional<std::int16_t>>(unsigned index)
	{
		return getInt16(index);
	}

	template <>
	inline std::optional<ScaledInt16> Row::get<std::optional<ScaledInt16>>(unsigned index)
	{
		return getScaledInt16(index);
	}

	template <>
	inline std::optional<std::int32_t> Row::get<std::optional<std::int32_t>>(unsigned index)
	{
		return getInt32(index);
	}

	template <>
	inline std::optional<ScaledInt32> Row::get<std::optional<ScaledInt32>>(unsigned index)
	{
		return getScaledInt32(index);
	}

	template <>
	inline std::optional<std::int64_t> Row::get<std::optional<std::int64_t>>(unsigned index)
	{
		return getInt64(index);
	}

	template <>
	inline std::optional<ScaledInt64> Row::get<std::optional<ScaledInt64>>(unsigned index)
	{
		return getScaledInt64(index);
	}

	template <>
	inline std::optional<ScaledOpaqueInt128> Row::get<std::optional<ScaledOpaqueInt128>>(unsigned index)
	{
		return getScaledOpaqueInt128(index);
	}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	inline std::optional<BoostInt128> Row::get<std::optional<BoostInt128>>(unsigned index)
	{
		return getBoostInt128(index);
	}

	template <>
	inline std::optional<ScaledBoostInt128> Row::get<std::optional<ScaledBoostInt128>>(unsigned index)
	{
		return getScaledBoostInt128(index);
	}
#endif

	template <>
	inline std::optional<float> Row::get<std::optional<float>>(unsigned index)
	{
		return getFloat(index);
	}

	template <>
	inline std::optional<double> Row::get<std::optional<double>>(unsigned index)
	{
		return getDouble(index);
	}

	template <>
	inline std::optional<OpaqueDecFloat16> Row::get<std::optional<OpaqueDecFloat16>>(unsigned index)
	{
		return getOpaqueDecFloat16(index);
	}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	inline std::optional<BoostDecFloat16> Row::get<std::optional<BoostDecFloat16>>(unsigned index)
	{
		return getBoostDecFloat16(index);
	}
#endif

	template <>
	inline std::optional<OpaqueDecFloat34> Row::get<std::optional<OpaqueDecFloat34>>(unsigned index)
	{
		return getOpaqueDecFloat34(index);
	}

#if FB_CPP_USE_BOOST_MULTIPRECISION != 0
	template <>
	inline std::optional<BoostDecFloat34> Row::get<std::optional<BoostDecFloat34>>(unsigned index)
	{
		return getBoostDecFloat34(index);
	}
#endif

	template <>
	inline std::optional<Date> Row::get<std::optional<Date>>(unsigned index)
	{
		return getDate(index);
	}

	template <>
	inline std::optional<OpaqueDate> Row::get<std::optional<OpaqueDate>>(unsigned index)
	{
		return getOpaqueDate(index);
	}

	template <>
	inline std::optional<Time> Row::get<std::optional<Time>>(unsigned index)
	{
		return getTime(index);
	}

	template <>
	inline std::optional<OpaqueTime> Row::get<std::optional<OpaqueTime>>(unsigned index)
	{
		return getOpaqueTime(index);
	}

	template <>
	inline std::optional<OpaqueTimestamp> Row::get<std::optional<OpaqueTimestamp>>(unsigned index)
	{
		return getOpaqueTimestamp(index);
	}

	template <>
	inline std::optional<Timestamp> Row::get<std::optional<Timestamp>>(unsigned index)
	{
		return getTimestamp(index);
	}

	template <>
	inline std::optional<TimeTz> Row::get<std::optional<TimeTz>>(unsigned index)
	{
		return getTimeTz(index);
	}

	template <>
	inline std::optional<OpaqueTimeTz> Row::get<std::optional<OpaqueTimeTz>>(unsigned index)
	{
		return getOpaqueTimeTz(index);
	}

	template <>
	inline std::optional<TimestampTz> Row::get<std::optional<TimestampTz>>(unsigned index)
	{
		return getTimestampTz(index);
	}

	template <>
	inline std::optional<OpaqueTimestampTz> Row::get<std::optional<OpaqueTimestampTz>>(unsigned index)
	{
		return getOpaqueTimestampTz(index);
	}

	template <>
	inline std::optional<std::string> Row::get<std::optional<std::string>>(unsigned index)
	{
		return getString(index);
	}

	///
	/// @}
	///
}  // namespace fbcpp


#endif  // FBCPP_ROW_H
