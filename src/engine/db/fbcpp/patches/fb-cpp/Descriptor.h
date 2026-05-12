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

#ifndef FBCPP_DESCRIPTOR_H
#define FBCPP_DESCRIPTOR_H

#include "fb-api.h"
#include <string>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Descriptor original type.
	///
	enum class DescriptorOriginalType : unsigned
	{
		///
		/// Null type indicator.
		///
		NULL_TYPE = SQL_NULL,

		///
		/// Fixed-length text.
		///
		TEXT = SQL_TEXT,

		///
		/// Variable-length text.
		///
		VARYING = SQL_VARYING,

		///
		/// 16-bit signed integer.
		///
		SHORT = SQL_SHORT,

		///
		/// 32-bit signed integer.
		///
		LONG = SQL_LONG,

		///
		/// Single-precision floating point.
		///
		FLOAT = SQL_FLOAT,

		///
		/// Double-precision floating point.
		///
		DOUBLE = SQL_DOUBLE,

		///
		/// Timestamp without time zone.
		///
		TIMESTAMP = SQL_TIMESTAMP,

		///
		/// Binary large object.
		///
		BLOB = SQL_BLOB,

		///
		/// Time of day without time zone.
		///
		TIME = SQL_TYPE_TIME,

		///
		/// Calendar date.
		///
		DATE = SQL_TYPE_DATE,

		///
		/// 64-bit signed integer.
		///
		INT64 = SQL_INT64,

		///
		/// Timestamp with time zone.
		///
		TIMESTAMP_TZ = SQL_TIMESTAMP_TZ,

		///
		/// Extended timestamp with time zone.
		///
		TIMESTAMP_TZ_EX = SQL_TIMESTAMP_TZ_EX,

		///
		/// Time of day with time zone.
		///
		TIME_TZ = SQL_TIME_TZ,

		///
		/// Extended time of day with time zone.
		///
		TIME_TZ_EX = SQL_TIME_TZ_EX,

		///
		/// 128-bit signed integer.
		///
		INT128 = SQL_INT128,

		///
		/// 16-digit decimal floating point.
		///
		DEC16 = SQL_DEC16,

		///
		/// 34-digit decimal floating point.
		///
		DEC34 = SQL_DEC34,

		///
		/// Boolean value.
		///
		BOOLEAN = SQL_BOOLEAN,
	};

	///
	/// Descriptor adjusted type.
	///
	enum class DescriptorAdjustedType : unsigned
	{
		///
		/// Null type indicator.
		///
		NULL_TYPE = SQL_NULL,

		///
		/// String type (variable-length).
		///
		STRING = SQL_VARYING,

		///
		/// 16-bit signed integer.
		///
		INT16 = SQL_SHORT,

		///
		/// 32-bit signed integer.
		///
		INT32 = SQL_LONG,

		///
		/// Single-precision floating point.
		///
		FLOAT = SQL_FLOAT,

		///
		/// Double-precision floating point.
		///
		DOUBLE = SQL_DOUBLE,

		///
		/// Timestamp without time zone.
		///
		TIMESTAMP = SQL_TIMESTAMP,

		///
		/// Binary large object.
		///
		BLOB = SQL_BLOB,

		///
		/// Time of day without time zone.
		///
		TIME = SQL_TYPE_TIME,

		///
		/// Calendar date.
		///
		DATE = SQL_TYPE_DATE,

		///
		/// 64-bit signed integer.
		///
		INT64 = SQL_INT64,

		///
		/// Timestamp with time zone.
		///
		TIMESTAMP_TZ = SQL_TIMESTAMP_TZ,

		///
		/// Extended timestamp with time zone.
		///
		TIMESTAMP_TZ_EX = SQL_TIMESTAMP_TZ_EX,

		///
		/// Time of day with time zone.
		///
		TIME_TZ = SQL_TIME_TZ,

		///
		/// Extended time of day with time zone.
		///
		TIME_TZ_EX = SQL_TIME_TZ_EX,

		///
		/// 128-bit signed integer.
		///
		INT128 = SQL_INT128,

		///
		/// 16-digit decimal floating point.
		///
		DECFLOAT16 = SQL_DEC16,

		///
		/// 34-digit decimal floating point.
		///
		DECFLOAT34 = SQL_DEC34,

		///
		/// Boolean value.
		///
		BOOLEAN = SQL_BOOLEAN,
	};

	///
	/// Describes a parameter or column.
	///
	struct Descriptor final
	{
		///
		/// Original SQL type as reported by Firebird.
		///
		DescriptorOriginalType originalType;

		///
		/// Adjusted type after normalization for easier handling.
		///
		DescriptorAdjustedType adjustedType;

		///
		/// Decimal scale for numeric types; zero for non-numeric types.
		///
		int scale;

		///
		/// Length in bytes of the column or parameter data.
		///
		unsigned length;

		///
		/// Byte offset of this field within the message buffer.
		///
		unsigned offset;

		///
		/// Byte offset of the null indicator within the message buffer.
		///
		unsigned nullOffset;

		///
		/// Indicates whether the column or parameter can contain null values.
		///
		bool isNullable;

		///
		/// Column or parameter name.
		///
		std::string name;

		///
		/// Table or relation this column belongs to (empty for expressions).
		///
		std::string relation;

		///
		/// Column alias as it appears in the query's SELECT list.
		///
		std::string alias;

		///
		/// Owner of the relation (empty for expressions).
		///
		std::string owner;

		///
		/// Character set ID for string and BLOB columns.
		///
		unsigned charSetId;

		///
		/// Sub-type (BLOB sub-type or numeric sub-type).
		///
		int subType;
	};
}  // namespace fbcpp


#endif  // FBCPP_DESCRIPTOR_H
