/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to do so, subject to the
 * following conditions:
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

#ifndef FBCPP_BLOB_H
#define FBCPP_BLOB_H

#include "fb-api.h"
#include "SmartPtrs.h"
#include "Exception.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Attachment;
	class Transaction;

	///
	/// Represents a Firebird blob identifier.
	///
	class BlobId final
	{
	public:
		/// Returns whether this blob identifier is empty.
		bool isEmpty() const noexcept
		{
			return id.gds_quad_high == 0 && id.gds_quad_low == 0;
		}

	public:
		///
		/// Stores the raw Firebird blob identifier value.
		///
		ISC_QUAD id{0, 0};
	};

	///
	/// Blob storage options.
	///
	enum class BlobStorage : std::uint8_t
	{
		///
		/// Blob is stored in the main database file.
		///
		MAIN = isc_bpb_storage_main,

		///
		/// Blob is stored in temporary storage and will not persist beyond the transaction.
		///
		TEMPORARY = isc_bpb_storage_temp
	};

	///
	/// Blob type.
	///
	enum class BlobType : std::uint8_t
	{
		///
		/// Blob is stored and accessed as discrete segments.
		///
		SEGMENTED = isc_bpb_type_segmented,

		///
		/// Blob is treated as a continuous stream of bytes.
		///
		STREAM = isc_bpb_type_stream
	};

	///
	/// Additional options used when creating or opening blobs.
	///
	class BlobOptions final
	{
	public:
		///
		/// Retrieves the blob parameter block (BPB) used during blob operations.
		///
		const std::vector<std::uint8_t>& getBpb() const noexcept
		{
			return bpb;
		}

		///
		/// Sets the blob parameter block (BPB) using a copy of the provided value.
		///
		BlobOptions& setBpb(const std::vector<std::uint8_t>& value)
		{
			bpb = value;
			return *this;
		}

		///
		/// Sets the blob parameter block (BPB) by moving the provided value.
		///
		BlobOptions& setBpb(std::vector<std::uint8_t>&& value)
		{
			bpb = std::move(value);
			return *this;
		}

		///
		/// Retrieves the blob type to be used for blob operations.
		///
		const std::optional<BlobType> getType() const
		{
			return type;
		}

		///
		/// Sets the blob type to be used for blob operations.
		///
		BlobOptions& setType(BlobType value)
		{
			type = value;
			return *this;
		}

		///
		/// Retrieves the source blob subtype.
		///
		const std::optional<BlobType> getSourceType() const
		{
			return sourceType;
		}

		///
		/// Sets the source blob subtype.
		///
		BlobOptions& setSourceType(BlobType value)
		{
			sourceType = value;
			return *this;
		}

		///
		/// Retrieves the target blob subtype.
		///
		const std::optional<BlobType> getTargetType() const
		{
			return targetType;
		}

		///
		/// Sets the target blob subtype.
		///
		BlobOptions& setTargetType(BlobType value)
		{
			targetType = value;
			return *this;
		}

		///
		/// Retrieves the source character set identifier.
		///
		const std::optional<std::int16_t> getSourceCharSet() const
		{
			return sourceCharSet;
		}

		///
		/// Sets the source character set identifier.
		///
		BlobOptions& setSourceCharSet(std::int16_t value)
		{
			sourceCharSet = value;
			return *this;
		}

		///
		/// Retrieves the target character set identifier.
		///
		const std::optional<std::int16_t> getTargetCharSet() const
		{
			return targetCharSet;
		}

		///
		/// Sets the target character set identifier.
		///
		BlobOptions& setTargetCharSet(std::int16_t value)
		{
			targetCharSet = value;
			return *this;
		}

		///
		/// Retrieves the blob storage mode.
		///
		const std::optional<BlobStorage> getStorage() const
		{
			return storage;
		}

		///
		/// Sets the blob storage mode.
		///
		BlobOptions& setStorage(BlobStorage value)
		{
			storage = value;
			return *this;
		}

	private:
		std::vector<std::uint8_t> bpb;
		std::optional<BlobType> type;
		std::optional<BlobType> sourceType;
		std::optional<BlobType> targetType;
		std::optional<std::int16_t> sourceCharSet;
		std::optional<std::int16_t> targetCharSet;
		std::optional<BlobStorage> storage;
	};

	///
	/// Defines the origin used when repositioning a blob.
	///
	enum class BlobSeekMode : int
	{
		///
		/// Offset is relative to the beginning of the blob.
		///
		FROM_BEGIN = 0,

		///
		/// Offset is relative to the current position in the blob.
		///
		FROM_CURRENT = blb_seek_relative,

		///
		/// Offset is relative to the end of the blob.
		///
		FROM_END = blb_seek_from_tail
	};

	///
	/// Provides read and write access to Firebird blobs.
	///
	class Blob final
	{
	public:
		///
		/// Creates and opens a new blob for writing.
		///
		Blob(Attachment& attachment, Transaction& transaction, const BlobOptions& options = {});

		///
		/// Opens an existing blob for reading or writing.
		///
		Blob(Attachment& attachment, Transaction& transaction, const BlobId& blobId, const BlobOptions& options = {});

		///
		/// Transfers blob ownership from another instance.
		///
		Blob(Blob&& o) noexcept
			: attachment{o.attachment},
			  transaction{o.transaction},
			  id{o.id},
			  statusWrapper{std::move(o.statusWrapper)},
			  handle{std::move(o.handle)}
		{
		}

		///
		/// Move assignment is not supported.
		///
		Blob& operator=(Blob&&) = delete;

		///
		/// Copy construction is not supported.
		///
		Blob(const Blob&) = delete;

		///
		/// Copy assignment is not supported.
		///
		Blob& operator=(const Blob&) = delete;

		///
		/// Automatically closes the blob if still open.
		///
		~Blob() noexcept
		{
			if (isValid())
			{
				try
				{
					close();
				}
				catch (...)
				{
					// swallow
				}
			}
		}

	public:
		///
		/// Returns whether the blob handle is valid.
		///
		bool isValid() const noexcept
		{
			return handle != nullptr;
		}

		///
		/// Provides access to the current blob identifier.
		///
		const BlobId& getId() const noexcept
		{
			return id;
		}

		///
		/// Retrieves the length of the blob in bytes.
		///
		unsigned getLength();

		///
		/// Exposes the underlying Firebird blob handle.
		///
		FbRef<fb::IBlob> getHandle() noexcept
		{
			return handle;
		}

		///
		/// Reads data from the blob into the provided buffer.
		///
		unsigned read(std::span<std::byte> buffer);

		///
		/// Reads data from the blob into the provided buffer.
		///
		unsigned read(std::span<char> buffer)
		{
			return read(std::as_writable_bytes(buffer));
		}

		///
		/// Reads a single segment from the blob into the provided buffer.
		///
		unsigned readSegment(std::span<std::byte> buffer);

		///
		/// Reads a single segment from the blob into the provided buffer.
		///
		unsigned readSegment(std::span<char> buffer)
		{
			return readSegment(std::as_writable_bytes(buffer));
		}

		///
		/// Writes data from the buffer into the blob.
		///
		void write(std::span<const std::byte> buffer);

		///
		/// Writes data from the buffer into the blob.
		///
		void write(std::span<const char> buffer)
		{
			write(std::as_bytes(buffer));
		}

		///
		/// Writes a single segment from the buffer into the blob.
		///
		void writeSegment(std::span<const std::byte> buffer);

		///
		/// Writes a single segment from the buffer into the blob.
		///
		void writeSegment(std::span<const char> buffer)
		{
			writeSegment(std::as_bytes(buffer));
		}

		///
		/// Repositions the blob read/write cursor.
		///
		int seek(BlobSeekMode mode, int offset);

		///
		/// Cancels any changes performed on the blob and releases the handle.
		///
		void cancel();

		///
		/// Closes the blob and finalizes any pending changes.
		///
		void close();

	private:
		std::vector<std::uint8_t> prepareBpb(const BlobOptions& options);

	private:
		Attachment& attachment;
		Transaction& transaction;
		BlobId id;
		impl::StatusWrapper statusWrapper;
		FbRef<fb::IBlob> handle;
	};
}  // namespace fbcpp


#endif  // FBCPP_BLOB_H
