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

#ifndef FBCPP_TRANSACTION_H
#define FBCPP_TRANSACTION_H

#include "fb-api.h"
#include "SmartPtrs.h"
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Attachment;

	///
	/// Transaction isolation level.
	///
	enum class TransactionIsolationLevel
	{
		///
		/// Ensures full transaction consistency at the expense of concurrency.
		///
		CONSISTENCY,

		///
		/// Allows reading of committed changes from other transactions.
		///
		READ_COMMITTED,

		///
		/// Provides a stable snapshot of the database at transaction start time.
		///
		SNAPSHOT
	};

	///
	/// Transaction read committed mode.
	///
	enum class TransactionReadCommittedMode
	{
		///
		/// Does not allow reading of record versions; waits for or errors on uncommitted changes.
		///
		NO_RECORD_VERSION,

		///
		/// Allows reading of the latest committed version of a record.
		///
		RECORD_VERSION
	};

	///
	/// Transaction access mode.
	///
	enum class TransactionAccessMode
	{
		///
		/// Transaction can only read data; write operations are not permitted.
		///
		READ_ONLY,

		///
		/// Transaction can read and write data.
		///
		READ_WRITE
	};

	///
	/// Transaction wait mode.
	///
	enum class TransactionWaitMode
	{
		///
		/// Transaction returns an error immediately if a lock conflict occurs.
		///
		NO_WAIT,

		///
		/// Transaction waits until a conflicting lock is released.
		///
		WAIT
	};

	///
	/// Represents options used when creating a Transaction object.
	///
	class TransactionOptions final
	{
	public:
		///
		/// Returns the TPB (Transaction Parameter Block) which will be used to start
		/// the transaction.
		///
		const std::vector<std::uint8_t>& getTpb() const
		{
			return tpb;
		}

		///
		/// Sets the TPB (Transaction Parameter Block) which will be used to start the
		/// transaction.
		///
		TransactionOptions& setTpb(const std::vector<std::uint8_t>& value)
		{
			tpb = value;
			return *this;
		}

		///
		/// Sets the TPB (Transaction Parameter Block) which will be used to start the
		/// transaction.
		///
		TransactionOptions& setTpb(std::vector<std::uint8_t>&& value)
		{
			tpb = std::move(value);
			return *this;
		}

		///
		/// Returns the transaction isolation level.
		///
		const std::optional<TransactionIsolationLevel> getIsolationLevel() const
		{
			return isolationLevel;
		}

		///
		/// Sets the transaction isolation level.
		///
		TransactionOptions& setIsolationLevel(TransactionIsolationLevel value)
		{
			isolationLevel = value;
			return *this;
		}

		///
		/// Returns the read committed mode.
		///
		const std::optional<TransactionReadCommittedMode> getReadCommittedMode() const
		{
			return readCommittedMode;
		}

		///
		/// Sets the read committed mode.
		///
		TransactionOptions& setReadCommittedMode(TransactionReadCommittedMode value)
		{
			readCommittedMode = value;
			return *this;
		}

		///
		/// Returns the transaction access mode.
		///
		const std::optional<TransactionAccessMode> getAccessMode() const
		{
			return accessMode;
		}

		///
		/// Sets the transaction access mode.
		///
		TransactionOptions& setAccessMode(TransactionAccessMode value)
		{
			accessMode = value;
			return *this;
		}

		///
		/// Returns the transaction wait mode.
		///
		const std::optional<TransactionWaitMode> getWaitMode() const
		{
			return waitMode;
		}

		///
		/// Sets the transaction wait mode.
		///
		TransactionOptions& setWaitMode(TransactionWaitMode value)
		{
			waitMode = value;
			return *this;
		}

		///
		/// Returns whether the transaction will not automatically undo changes in
		/// case of a deadlock or update conflict.
		///
		bool getNoAutoUndo() const
		{
			return noAutoUndo;
		}

		///
		/// Sets whether the transaction will not automatically undo changes in case
		/// of a deadlock or update conflict.
		///
		TransactionOptions& setNoAutoUndo(bool value)
		{
			noAutoUndo = value;
			return *this;
		}

		///
		/// Returns whether the transaction will ignore limbo transactions.
		///
		bool getIgnoreLimbo() const
		{
			return ignoreLimbo;
		}

		///
		/// Sets whether the transaction will ignore limbo transactions.
		///
		TransactionOptions& setIgnoreLimbo(bool value)
		{
			ignoreLimbo = value;
			return *this;
		}

		///
		/// Returns whether the transaction will restart requests.
		///
		bool getRestartRequests() const
		{
			return restartRequests;
		}

		///
		/// Sets whether the transaction will restart requests.
		///
		TransactionOptions& setRestartRequests(bool value)
		{
			restartRequests = value;
			return *this;
		}

		///
		/// Returns whether the transaction will be automatically committed.
		///
		bool getAutoCommit() const
		{
			return autoCommit;
		}

		///
		/// Sets whether the transaction will be automatically committed.
		///
		TransactionOptions& setAutoCommit(bool value)
		{
			autoCommit = value;
			return *this;
		}

	private:
		std::vector<std::uint8_t> tpb;
		std::optional<TransactionIsolationLevel> isolationLevel;
		std::optional<TransactionReadCommittedMode> readCommittedMode;
		std::optional<TransactionAccessMode> accessMode;
		std::optional<TransactionWaitMode> waitMode;
		bool noAutoUndo = false;
		bool ignoreLimbo = false;
		bool restartRequests = false;
		bool autoCommit = false;
	};

	class Client;

	///
	/// Transaction state for tracking two-phase commit lifecycle.
	///
	enum class TransactionState
	{
		///
		/// Transaction is active and can execute statements.
		///
		ACTIVE,

		///
		/// Transaction has been prepared (2PC phase 1).
		///
		PREPARED,

		///
		/// Transaction has been committed.
		///
		COMMITTED,

		///
		/// Transaction has been rolled back.
		///
		ROLLED_BACK
	};

	///
	/// Represents a transaction in one or more Firebird databases.
	///
	/// Single-database transactions are created using the Attachment constructor.
	/// Multi-database transactions are created using the vector of Attachments constructor
	/// and support two-phase commit (2PC) protocol via the prepare() method.
	///
	/// For 2PC:
	/// 1. Create multi-database transaction with multiple Attachments
	/// 2. Execute statements across databases
	/// 3. Call prepare() to enter prepared state
	/// 4. Call commit() or rollback() to complete
	///
	/// Important: Prepared transactions MUST be explicitly committed or rolled back.
	/// The destructor will NOT automatically rollback prepared transactions.
	///
	/// The Transaction must exist and remain valid while there are other objects
	/// using it, such as Statement. If a Transaction object is destroyed before
	/// being committed or rolled back (and not prepared), it will be automatically
	/// rolled back.
	///
	class Transaction final
	{
	public:
		///
		/// Constructs a Transaction object associated with the specified Client,
		/// but without an active Firebird transaction handle.
		///
		explicit Transaction(Client& client);

		///
		/// Constructs a Transaction object that starts a transaction in the specified
		/// Attachment using the specified options.
		///
		explicit Transaction(Attachment& attachment, const TransactionOptions& options = {});

		///
		/// Constructs a Transaction object that starts a transaction specified by a
		/// `SET TRANSACTION` command.
		///
		explicit Transaction(Attachment& attachment, std::string_view setTransactionCmd);

		///
		/// Constructs a Transaction object that starts a multi-database transaction
		/// across the specified Attachments using the specified options.
		///
		/// All attachments must use the same Client. The same TransactionOptions
		/// (TPB) will be applied to all databases.
		///
		/// This constructor enables two-phase commit (2PC) support via the prepare() method.
		///
		explicit Transaction(
			std::span<std::reference_wrapper<Attachment>> attachments, const TransactionOptions& options = {});

		///
		/// Move constructor.
		/// A moved Transaction object becomes invalid.
		///
		Transaction(Transaction&& o) noexcept
			: client{o.client},
			  handle{std::move(o.handle)},
			  state{o.state},
			  isMultiDatabase{o.isMultiDatabase}
		{
			o.state = TransactionState::ROLLED_BACK;
		}

		Transaction& operator=(Transaction&&) = delete;

		Transaction(const Transaction&) = delete;
		Transaction& operator=(const Transaction&) = delete;

		///
		/// Rolls back the transaction if it is still active.
		///
		/// Prepared transactions are NOT automatically rolled back and must be
		/// explicitly committed or rolled back before destruction.
		///
		~Transaction() noexcept
		{
			if (isValid())
			{
				assert(state != TransactionState::PREPARED &&
					"Prepared transaction must be explicitly committed or rolled back");

				try
				{
					if (state == TransactionState::ACTIVE)
						rollback();
				}
				catch (...)
				{
					// swallow
				}
			}
		}

	public:
		///
		/// Returns whether the Transaction object is valid.
		///
		bool isValid() noexcept
		{
			return handle != nullptr;
		}

		///
		/// Returns the internal Firebird ITransaction handle.
		///
		FbRef<fb::ITransaction> getHandle() noexcept
		{
			return handle;
		}

		///
		/// Returns the current transaction state.
		///
		TransactionState getState() const noexcept
		{
			return state;
		}

		///
		/// Prepares the transaction for two-phase commit (2PC phase 1).
		///
		/// After prepare() is called, the transaction must be explicitly committed or rolled back.
		/// The destructor will NOT automatically rollback prepared transactions.
		///
		void prepare();

		///
		/// Prepares the transaction for two-phase commit with a text message identifier.
		///
		/// The message can be used to identify the transaction during recovery operations.
		///
		void prepare(std::string_view message);

		///
		/// Prepares the transaction for two-phase commit with a binary message identifier.
		///
		/// The message can be used to identify the transaction during recovery operations.
		///
		void prepare(std::span<const std::uint8_t> message);

		///
		/// Commits the transaction.
		///
		/// Can be called from ACTIVE or PREPARED state.
		///
		void commit();

		///
		/// Commits the transaction while maintains it active.
		///
		/// Cannot be called on a prepared transaction.
		///
		void commitRetaining();

		///
		/// Rolls back the transaction.
		///
		/// Can be called from ACTIVE or PREPARED state.
		///
		void rollback();

		///
		/// Rolls back the transaction while maintains it active.
		///
		/// Cannot be called on a prepared transaction.
		///
		void rollbackRetaining();

		///
		/// Starts the transaction in the specified Attachment using the specified options.
		///
		void start(Attachment& attachment, const TransactionOptions& options = {});

	private:
		Client& client;
		FbRef<fb::ITransaction> handle;
		TransactionState state = TransactionState::ACTIVE;
		const bool isMultiDatabase = false;
	};
}  // namespace fbcpp


#endif  // FBCPP_TRANSACTION_H
