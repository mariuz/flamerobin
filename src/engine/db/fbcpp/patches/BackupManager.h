/*
 * MIT License
 *
 * Copyright (c) 2026 Adriano dos Santos Fernandes
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

#ifndef FBCPP_BACKUP_MANAGER_H
#define FBCPP_BACKUP_MANAGER_H

#include "ServiceManager.h"
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Represents options used to run a backup operation through the service manager.
	///
	class BackupOptions final
	{
	public:
		struct BackupFileSpec final
		{
			std::string path;
			std::optional<std::uint64_t> length;
		};

	public:
		///
		/// Returns the database path to be backed up.
		///
		const std::string& getDatabase() const
		{
			return database;
		}

		///
		/// Sets the database path to be backed up.
		///
		BackupOptions& setDatabase(const std::string& value)
		{
			database = value;
			return *this;
		}

		///
		/// Returns the configured backup file specifications.
		///
		const std::vector<BackupFileSpec>& getBackupFiles() const
		{
			return backupFiles;
		}

		///
		/// Appends a backup file path.
		///
		BackupOptions& addBackupFile(const std::string& value)
		{
			backupFiles.push_back({value, std::nullopt});
			return *this;
		}

		///
		/// Appends a backup file path with a split length.
		///
		BackupOptions& addBackupFile(const std::string& value, std::uint64_t length)
		{
			if (!backupFiles.empty() && !backupFiles.back().length)
				throw std::invalid_argument{"Cannot add a backup file with length after a backup file without length"};

			backupFiles.push_back({value, length});
			return *this;
		}

		///
		/// Replaces the backup file paths with a single path.
		///
		BackupOptions& setBackupFile(const std::string& value)
		{
			backupFiles = {{value, std::nullopt}};
			return *this;
		}

		///
		/// Replaces the backup file paths with a single path and split length.
		///
		BackupOptions& setBackupFile(const std::string& value, std::uint64_t length)
		{
			backupFiles = {{value, length}};
			return *this;
		}

		///
		/// Returns the verbose output callback.
		///
		const ServiceManager::VerboseOutput& getVerboseOutput() const
		{
			return verboseOutput;
		}

		///
		/// Sets the verbose output callback.
		///
		BackupOptions& setVerboseOutput(ServiceManager::VerboseOutput value)
		{
			verboseOutput = std::move(value);
			return *this;
		}

		///
		/// Returns the requested number of parallel workers.
		///
		const std::optional<std::uint32_t>& getParallelWorkers() const
		{
			return parallelWorkers;
		}

		///
		/// Sets the requested number of parallel workers.
		///
		BackupOptions& setParallelWorkers(std::uint32_t value)
		{
			parallelWorkers = value;
			return *this;
		}

	private:
		std::string database;
		std::vector<BackupFileSpec> backupFiles;
		ServiceManager::VerboseOutput verboseOutput;
		std::optional<std::uint32_t> parallelWorkers;
	};

	///
	/// Represents options used to run a restore operation through the service manager.
	///
	class RestoreOptions final
	{
	public:
		struct DatabaseFileSpec final
		{
			std::string path;
			std::optional<std::uint64_t> length;
		};

	public:
		///
		/// Returns the configured database file specifications.
		///
		const std::vector<DatabaseFileSpec>& getDatabaseFiles() const
		{
			return databaseFiles;
		}

		///
		/// Sets the database path to be restored.
		///
		RestoreOptions& setDatabase(const std::string& value)
		{
			databaseFiles = {{value, std::nullopt}};
			return *this;
		}

		///
		/// Sets the database path to be restored with a split length.
		///
		RestoreOptions& setDatabase(const std::string& value, std::uint64_t length)
		{
			databaseFiles = {{value, length}};
			return *this;
		}

		///
		/// Appends a database file path.
		///
		RestoreOptions& addDatabaseFile(const std::string& value)
		{
			databaseFiles.push_back({value, std::nullopt});
			return *this;
		}

		///
		/// Appends a database file path with a split length.
		///
		RestoreOptions& addDatabaseFile(const std::string& value, std::uint64_t length)
		{
			if (!databaseFiles.empty() && !databaseFiles.back().length)
			{
				throw std::invalid_argument{
					"Cannot add a database file with length after a database file without length"};
			}

			databaseFiles.push_back({value, length});
			return *this;
		}

		///
		/// Returns the backup file paths.
		///
		const std::vector<std::string>& getBackupFiles() const
		{
			return backupFiles;
		}

		///
		/// Appends a backup file path.
		///
		RestoreOptions& addBackupFile(const std::string& value)
		{
			backupFiles.emplace_back(value);
			return *this;
		}

		///
		/// Replaces the backup file paths with a single path.
		///
		RestoreOptions& setBackupFile(const std::string& value)
		{
			backupFiles = {value};
			return *this;
		}

		///
		/// Returns whether the target database should be replaced.
		///
		bool getReplace() const
		{
			return replace;
		}

		///
		/// Sets whether the target database should be replaced.
		///
		RestoreOptions& setReplace(bool value)
		{
			replace = value;
			return *this;
		}

		///
		/// Returns the verbose output callback.
		///
		const ServiceManager::VerboseOutput& getVerboseOutput() const
		{
			return verboseOutput;
		}

		///
		/// Sets the verbose output callback.
		///
		RestoreOptions& setVerboseOutput(ServiceManager::VerboseOutput value)
		{
			verboseOutput = std::move(value);
			return *this;
		}

		///
		/// Returns the requested number of parallel workers.
		///
		const std::optional<std::uint32_t>& getParallelWorkers() const
		{
			return parallelWorkers;
		}

		///
		/// Sets the requested number of parallel workers.
		///
		RestoreOptions& setParallelWorkers(std::uint32_t value)
		{
			parallelWorkers = value;
			return *this;
		}

	private:
		std::vector<DatabaseFileSpec> databaseFiles;
		std::vector<std::string> backupFiles;
		bool replace = false;
		ServiceManager::VerboseOutput verboseOutput;
		std::optional<std::uint32_t> parallelWorkers;
	};

	///
	/// Executes backup and restore operations through the Firebird service manager.
	///
	class BackupManager final : public ServiceManager
	{
	public:
		using ServiceManager::ServiceManager;

	public:
		///
		/// Runs a backup operation using the provided options.
		///
		void backup(const BackupOptions& options);

		///
		/// Runs a restore operation using the provided options.
		///
		void restore(const RestoreOptions& options);
	};
}  // namespace fbcpp


#endif  // FBCPP_BACKUP_MANAGER_H
