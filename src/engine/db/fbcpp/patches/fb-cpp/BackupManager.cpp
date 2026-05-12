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

#include "BackupManager.h"
#include "Client.h"

using namespace fbcpp;
using namespace fbcpp::impl;

void BackupManager::backup(const BackupOptions& options)
{
	StatusWrapper statusWrapper{getClient()};
	auto builder =
		fbUnique(getClient().getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_START, nullptr, 0));
	builder->insertTag(&statusWrapper, isc_action_svc_backup);
	builder->insertString(&statusWrapper, isc_spb_dbname, options.getDatabase().c_str());

	for (const auto& backupFile : options.getBackupFiles())
	{
		builder->insertString(&statusWrapper, isc_spb_bkp_file, backupFile.path.c_str());

		if (backupFile.length)
			addSpbInt(builder.get(), &statusWrapper, isc_spb_bkp_length, *backupFile.length, "Backup file length");
	}

	if (options.getVerboseOutput())
		builder->insertTag(&statusWrapper, isc_spb_verbose);

	if (const auto parallelWorkers = options.getParallelWorkers())
		builder->insertInt(&statusWrapper, isc_spb_bkp_parallel_workers, static_cast<int>(*parallelWorkers));

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	startAction(std::vector<std::uint8_t>(buffer, buffer + length));
	waitForCompletion(options.getVerboseOutput());
}

void BackupManager::restore(const RestoreOptions& options)
{
	StatusWrapper statusWrapper{getClient()};
	auto builder =
		fbUnique(getClient().getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_START, nullptr, 0));
	builder->insertTag(&statusWrapper, isc_action_svc_restore);

	for (const auto& databaseFile : options.getDatabaseFiles())
	{
		builder->insertString(&statusWrapper, isc_spb_dbname, databaseFile.path.c_str());

		if (databaseFile.length)
			addSpbInt(builder.get(), &statusWrapper, isc_spb_res_length, *databaseFile.length, "Database file length");
	}

	for (const auto& backupFile : options.getBackupFiles())
		builder->insertString(&statusWrapper, isc_spb_bkp_file, backupFile.c_str());

	builder->insertInt(
		&statusWrapper, isc_spb_options, options.getReplace() ? isc_spb_res_replace : isc_spb_res_create);

	if (options.getVerboseOutput())
		builder->insertTag(&statusWrapper, isc_spb_verbose);

	if (const auto parallelWorkers = options.getParallelWorkers())
		builder->insertInt(&statusWrapper, isc_spb_res_parallel_workers, static_cast<int>(*parallelWorkers));

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	startAction(std::vector<std::uint8_t>(buffer, buffer + length));
	waitForCompletion(options.getVerboseOutput(), true);
}
