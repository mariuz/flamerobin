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

#ifndef FBCPP_EXCEPTION_H
#define FBCPP_EXCEPTION_H

#include "fb-api.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Client;
}

namespace fbcpp::impl
{
	class StatusWrapper : public fb::IStatusImpl<StatusWrapper, StatusWrapper>
	{
	public:
		explicit StatusWrapper(Client& client, IStatus* status = nullptr)
			: client{&client},
			  status{status}
		{
		}

		StatusWrapper(StatusWrapper&& o) noexcept
			: client{o.client},
			  status{o.status},
			  statusOwner{o.statusOwner},
			  dirty{o.dirty}
		{
			o.status = nullptr;
			o.statusOwner = false;
			o.dirty = false;
		}

		StatusWrapper& operator=(StatusWrapper&& o) noexcept
		{
			if (this != &o)
			{
				if (statusOwner && status)
					status->dispose();

				client = o.client;
				status = o.status;
				statusOwner = o.statusOwner;
				dirty = o.dirty;

				o.status = nullptr;
				o.statusOwner = false;
				o.dirty = false;
			}

			return *this;
		}

		StatusWrapper(const StatusWrapper&) = delete;
		StatusWrapper& operator=(const StatusWrapper&) = delete;

		~StatusWrapper()
		{
			if (statusOwner && status)
				status->dispose();
		}

	public:
		static void checkException(StatusWrapper* status);

		static void catchException(IStatus* status) noexcept;

		static void clearException(StatusWrapper* status) noexcept
		{
			status->clearException();
		}

		void clearException()
		{
			if (dirty)
			{
				dirty = false;
				getStatus()->init();
			}
		}

		bool isDirty() const noexcept
		{
			return dirty;
		}

		bool hasData() const noexcept
		{
			return getState() & IStatus::STATE_ERRORS;
		}

		bool isEmpty() const noexcept
		{
			return !hasData();
		}

		static void setVersionError(
			IStatus* status, const char* interfaceName, uintptr_t currentVersion, uintptr_t expectedVersion) noexcept
		{
			// clang-format off
			const intptr_t codes[] = {
				isc_arg_gds, isc_interface_version_too_old,
				isc_arg_number, (intptr_t) expectedVersion,
				isc_arg_number, (intptr_t) currentVersion,
				isc_arg_string, (intptr_t) interfaceName,
				isc_arg_end,
			};
			// clang-format on

			status->setErrors(codes);
		}

	public:
		void dispose() noexcept override
		{
			// Disposes only the delegated status. Let the user destroy this instance.
			if (status)
				status->dispose();

			status = nullptr;
			statusOwner = false;
		}

		void init() noexcept override
		{
			clearException();
		}

		unsigned getState() const noexcept override
		{
			return dirty ? getStatus()->getState() : 0;
		}

		void setErrors2(unsigned length, const intptr_t* value) noexcept override
		{
			dirty = true;
			getStatus()->setErrors2(length, value);
		}

		void setWarnings2(unsigned length, const intptr_t* value) noexcept override
		{
			dirty = true;
			getStatus()->setWarnings2(length, value);
		}

		void setErrors(const intptr_t* value) noexcept override
		{
			dirty = true;
			getStatus()->setErrors(value);
		}

		void setWarnings(const intptr_t* value) noexcept override
		{
			dirty = true;
			getStatus()->setWarnings(value);
		}

		const intptr_t* getErrors() const noexcept override
		{
			return dirty ? getStatus()->getErrors() : cleanStatus();
		}

		const intptr_t* getWarnings() const noexcept override
		{
			return dirty ? getStatus()->getWarnings() : cleanStatus();
		}

		IStatus* clone() const noexcept override
		{
			return getStatus()->clone();
		}

	protected:
		Client* client;
		mutable IStatus* status;
		mutable bool statusOwner = false;
		bool dirty = false;

		IStatus* getStatus() const;

		static const intptr_t* cleanStatus() noexcept
		{
			static intptr_t clean[3] = {1, 0, 0};
			return clean;
		}
	};
}  // namespace fbcpp::impl


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Base exception class for all fb-cpp exceptions.
	///
	class FbCppException : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;

		///
		/// Constructs an FbCppException with the specified error message.
		///
		explicit FbCppException(const std::string& message)
			: std::runtime_error{message}
		{
		}
	};

	///
	/// Exception thrown when a Firebird database operation fails.
	///
	class DatabaseException final : public FbCppException
	{
	public:
		///
		/// Constructs a DatabaseException from a Firebird status vector.
		///
		explicit DatabaseException(Client& client, const std::intptr_t* statusVector)
			: FbCppException{buildMessage(client, statusVector)},
			  sqlState{extractSqlState(statusVector)}
		{
			copyErrorVector(statusVector);
		}

		DatabaseException(const DatabaseException& other)
			: FbCppException{static_cast<const FbCppException&>(other)},
			  errorVector{other.errorVector},
			  errorStrings{other.errorStrings},
			  sqlState{other.sqlState}
		{
			fixupStringPointers();
		}

		DatabaseException(DatabaseException&&) = default;

		DatabaseException& operator=(const DatabaseException&) = delete;
		DatabaseException& operator=(DatabaseException&&) = delete;

		///
		/// Returns the Firebird error vector.
		/// The vector is terminated by isc_arg_end.
		///
		const std::vector<std::intptr_t>& getErrors() const noexcept
		{
			return errorVector;
		}

		///
		/// Returns the primary ISC error code (first isc_arg_gds value), or 0 if none.
		///
		std::intptr_t getErrorCode() const noexcept
		{
			if (errorVector.size() >= 2 && errorVector[0] == isc_arg_gds)
				return errorVector[1];
			return 0;
		}

		///
		/// Returns the SQL state string (e.g. "42000") if present in the original status vector,
		/// or empty otherwise.
		///
		const std::string& getSqlState() const noexcept
		{
			return sqlState;
		}

	private:
		static std::string buildMessage(Client& client, const std::intptr_t* statusVector);
		static std::string extractSqlState(const std::intptr_t* statusVector);

		void copyErrorVector(const std::intptr_t* statusVector);
		void fixupStringPointers();

	private:
		std::vector<std::intptr_t> errorVector;
		std::vector<std::string> errorStrings;
		std::string sqlState;
	};
}  // namespace fbcpp


#endif  // FBCPP_EXCEPTION_H
