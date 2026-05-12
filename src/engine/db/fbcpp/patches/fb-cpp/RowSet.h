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

#ifndef FBCPP_ROWSET_H
#define FBCPP_ROWSET_H

#include "fb-api.h"
#include "Row.h"
#include "SmartPtrs.h"
#include "NumericConverter.h"
#include "CalendarConverter.h"
#include "Descriptor.h"
#include "Exception.h"
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Statement;

	///
	/// @brief A disconnected buffer of rows fetched from a Statement's result set.
	///
	/// Rows are fetched into a contiguous buffer at construction time. After
	/// construction the RowSet is independent of its source Statement and can
	/// be used, moved, or destroyed freely.
	///
	class RowSet final
	{

	public:
		///
		/// @brief Fetches up to `maxRows` rows from the current result set of
		/// `statement`.
		///
		/// The statement must have an open result set (i.e. `execute()` was
		/// called and it is a SELECT-type statement). Rows are fetched via
		/// `IResultSet::fetchNext()` directly into the internal buffer.
		///
		/// @param statement The statement with an open result set.
		/// @param maxRows Maximum number of rows to fetch.
		///
		explicit RowSet(Statement& statement, unsigned maxRows);

		RowSet(RowSet&& o) noexcept
			: client{o.client},
			  count{o.count},
			  messageLength{o.messageLength},
			  buffer{std::move(o.buffer)},
			  descriptors{std::move(o.descriptors)},
			  statusWrapper{std::move(o.statusWrapper)},
			  numericConverter{std::move(o.numericConverter)},
			  calendarConverter{std::move(o.calendarConverter)}
		{
			o.count = 0;
			o.messageLength = 0;
		}

		RowSet& operator=(RowSet&& o) noexcept
		{
			if (this != &o)
			{
				client = o.client;
				count = o.count;
				messageLength = o.messageLength;
				buffer = std::move(o.buffer);
				descriptors = std::move(o.descriptors);
				statusWrapper = std::move(o.statusWrapper);
				numericConverter = std::move(o.numericConverter);
				calendarConverter = std::move(o.calendarConverter);
				o.count = 0;
				o.messageLength = 0;
			}

			return *this;
		}

		RowSet(const RowSet&) = delete;
		RowSet& operator=(const RowSet&) = delete;

	public:
		///
		/// @brief Returns the number of rows actually fetched.
		///
		unsigned getCount() const noexcept
		{
			return count;
		}

		///
		/// @brief Returns the message length (in bytes) of each row.
		///
		unsigned getMessageLength() const noexcept
		{
			return messageLength;
		}

		///
		/// @brief Returns a Row view for typed access to the row at `index`.
		/// @param index Zero-based row index (must be < getCount()).
		///
		Row getRow(unsigned index)
		{
			assert(index < count);
			return Row{*client, descriptors, getRawRow(index)};
		}

		///
		/// @brief Returns a span over the raw data of the row at `index`.
		/// @param index Zero-based row index (must be < getCount()).
		///
		std::span<const std::byte> getRawRow(unsigned index) const
		{
			assert(index < count);
			const auto* data = buffer.data() + static_cast<std::size_t>(index) * messageLength;
			return {data, messageLength};
		}

		///
		/// @brief Returns the entire contiguous buffer containing all fetched rows.
		///
		const std::vector<std::byte>& getRawBuffer() const noexcept
		{
			return buffer;
		}

	private:
		Client* client;
		unsigned count = 0;
		unsigned messageLength = 0;
		std::vector<std::byte> buffer;
		std::vector<Descriptor> descriptors;
		impl::StatusWrapper statusWrapper;
		impl::NumericConverter numericConverter;
		impl::CalendarConverter calendarConverter;
	};
}  // namespace fbcpp

#endif  // FBCPP_ROWSET_H
