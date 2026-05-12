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

#include "Batch.h"
#include "Attachment.h"
#include "Client.h"
#include "Statement.h"
#include "Transaction.h"

using namespace fbcpp;
using namespace fbcpp::impl;


// --- BatchCompletionState ---

BatchCompletionState::BatchCompletionState(Client& client, FbUniquePtr<fb::IBatchCompletionState> handle) noexcept
	: client{&client},
	  statusWrapper{client},
	  handle{std::move(handle)}
{
}

BatchCompletionState::BatchCompletionState(BatchCompletionState&& o) noexcept
	: client{o.client},
	  statusWrapper{std::move(o.statusWrapper)},
	  handle{std::move(o.handle)}
{
}

unsigned BatchCompletionState::getSize()
{
	return handle->getSize(&statusWrapper);
}

int BatchCompletionState::getState(unsigned pos)
{
	return handle->getState(&statusWrapper, pos);
}

std::optional<unsigned> BatchCompletionState::findError(unsigned pos)
{
	const auto result = handle->findError(&statusWrapper, pos);

	if (result == fb::IBatchCompletionState::NO_MORE_ERRORS)
		return std::nullopt;

	return result;
}

std::vector<std::intptr_t> BatchCompletionState::getStatus(unsigned pos)
{
	auto tempStatus = client->newStatus();

	handle->getStatus(&statusWrapper, tempStatus.get(), pos);

	std::vector<std::intptr_t> result;
	const auto* errors = tempStatus->getErrors();

	if (errors)
	{
		const auto* p = errors;

		while (*p != isc_arg_end)
		{
			result.push_back(*p++);
			result.push_back(*p++);
		}

		result.push_back(isc_arg_end);
	}

	return result;
}


// --- Batch ---

Batch::Batch(Statement& statement, Transaction& transaction, const BatchOptions& options)
	: client{&statement.getAttachment().getClient()},
	  transaction{&transaction},
	  statement{&statement},
	  statusWrapper{*client}
{
	assert(statement.isValid());
	assert(transaction.isValid());

	const auto parBlock = buildParametersBlock(options);

	handle.reset(statement.getStatementHandle()->createBatch(
		&statusWrapper, statement.getInputMetadata().get(), static_cast<unsigned>(parBlock.size()), parBlock.data()));
}

Batch::Batch(Attachment& attachment, Transaction& transaction, std::string_view sql, unsigned dialect,
	const BatchOptions& options)
	: client{&attachment.getClient()},
	  transaction{&transaction},
	  statusWrapper{*client}
{
	assert(attachment.isValid());
	assert(transaction.isValid());

	const auto parBlock = buildParametersBlock(options);

	handle.reset(attachment.getHandle()->createBatch(&statusWrapper, transaction.getHandle().get(),
		static_cast<unsigned>(sql.length()), sql.data(), dialect, nullptr, static_cast<unsigned>(parBlock.size()),
		parBlock.data()));
}

Batch::Batch(Batch&& o) noexcept
	: client{o.client},
	  transaction{o.transaction},
	  statement{o.statement},
	  statusWrapper{std::move(o.statusWrapper)},
	  handle{std::move(o.handle)}
{
}


// --- Adding messages ---

void Batch::add(unsigned count, const void* inBuffer)
{
	assert(isValid());
	handle->add(&statusWrapper, count, inBuffer);
}

void Batch::addMessage()
{
	assert(isValid());
	assert(statement);
	handle->add(&statusWrapper, 1, statement->getInputMessage().data());
}


// --- Blob support ---

BlobId Batch::addBlob(std::span<const std::byte> data, const BlobOptions& bpb)
{
	assert(isValid());

	const auto preparedBpb = prepareBpb(bpb);

	BlobId blobId;
	handle->addBlob(&statusWrapper, static_cast<unsigned>(data.size()), data.data(), &blobId.id,
		static_cast<unsigned>(preparedBpb.size()), preparedBpb.data());

	return blobId;
}

void Batch::appendBlobData(std::span<const std::byte> data)
{
	assert(isValid());
	handle->appendBlobData(&statusWrapper, static_cast<unsigned>(data.size()), data.data());
}

void Batch::addBlobStream(std::span<const std::byte> data)
{
	assert(isValid());
	handle->addBlobStream(&statusWrapper, static_cast<unsigned>(data.size()), data.data());
}

BlobId Batch::registerBlob(const BlobId& existingBlob)
{
	assert(isValid());

	BlobId batchId;
	handle->registerBlob(&statusWrapper, &existingBlob.id, &batchId.id);

	return batchId;
}

void Batch::setDefaultBpb(const BlobOptions& bpb)
{
	assert(isValid());

	const auto preparedBpb = prepareBpb(bpb);
	handle->setDefaultBpb(&statusWrapper, static_cast<unsigned>(preparedBpb.size()), preparedBpb.data());
}

unsigned Batch::getBlobAlignment()
{
	assert(isValid());
	return handle->getBlobAlignment(&statusWrapper);
}


// --- Execution ---

BatchCompletionState Batch::execute()
{
	assert(isValid());

	auto completionState = fbUnique(handle->execute(&statusWrapper, transaction->getHandle().get()));

	return BatchCompletionState{*client, std::move(completionState)};
}

void Batch::cancel()
{
	assert(isValid());
	handle->cancel(&statusWrapper);
	handle.reset();
}

void Batch::close()
{
	assert(isValid());
	handle->close(&statusWrapper);
	handle.reset();
}

FbRef<fb::IMessageMetadata> Batch::getInputMetadata()
{
	assert(isValid());

	FbRef<fb::IMessageMetadata> metadata;
	metadata.reset(handle->getMetadata(&statusWrapper));

	return metadata;
}

const std::vector<Descriptor>& Batch::getInputDescriptors()
{
	assert(isValid());

	if (inputDescriptors.empty())
		buildInputDescriptors();

	return inputDescriptors;
}


// --- Internal helpers ---

std::vector<std::uint8_t> Batch::buildParametersBlock(const BatchOptions& options)
{
	auto builder = fbUnique(client->getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::BATCH, nullptr, 0));

	if (options.getMultiError())
		builder->insertInt(&statusWrapper, fb::IBatch::TAG_MULTIERROR, 1);

	if (options.getRecordCounts())
		builder->insertInt(&statusWrapper, fb::IBatch::TAG_RECORD_COUNTS, 1);

	if (const auto bufferSize = options.getBufferBytesSize(); bufferSize.has_value())
		builder->insertInt(&statusWrapper, fb::IBatch::TAG_BUFFER_BYTES_SIZE, static_cast<int>(bufferSize.value()));

	if (options.getBlobPolicy() != BlobPolicy::NONE)
		builder->insertInt(&statusWrapper, fb::IBatch::TAG_BLOB_POLICY, static_cast<int>(options.getBlobPolicy()));

	if (options.getDetailedErrors() != 64)
	{
		builder->insertInt(
			&statusWrapper, fb::IBatch::TAG_DETAILED_ERRORS, static_cast<int>(options.getDetailedErrors()));
	}

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	return {buffer, buffer + length};
}

std::vector<std::uint8_t> Batch::prepareBpb(const BlobOptions& bpb)
{
	auto builder = fbUnique(client->getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::BPB,
		reinterpret_cast<const std::uint8_t*>(bpb.getBpb().data()), static_cast<unsigned>(bpb.getBpb().size())));

	if (const auto type = bpb.getType(); type.has_value())
		builder->insertInt(&statusWrapper, isc_bpb_type, static_cast<int>(type.value()));

	if (const auto storage = bpb.getStorage(); storage.has_value())
		builder->insertInt(&statusWrapper, isc_bpb_storage, static_cast<int>(storage.value()));

	const auto buffer = builder->getBuffer(&statusWrapper);
	const auto length = builder->getBufferLength(&statusWrapper);

	return {buffer, buffer + length};
}

void Batch::buildInputDescriptors()
{
	auto metadata = getInputMetadata();
	const auto count = metadata->getCount(&statusWrapper);

	inputDescriptors.reserve(count);

	for (unsigned index = 0u; index < count; ++index)
	{
		inputDescriptors.push_back(Descriptor{
			.originalType = static_cast<DescriptorOriginalType>(metadata->getType(&statusWrapper, index)),
			.adjustedType = static_cast<DescriptorAdjustedType>(metadata->getType(&statusWrapper, index)),
			.scale = metadata->getScale(&statusWrapper, index),
			.length = metadata->getLength(&statusWrapper, index),
			.offset = metadata->getOffset(&statusWrapper, index),
			.nullOffset = metadata->getNullOffset(&statusWrapper, index),
			.isNullable = static_cast<bool>(metadata->isNullable(&statusWrapper, index)),
			.name = metadata->getField(&statusWrapper, index),
			.relation = metadata->getRelation(&statusWrapper, index),
			.alias = metadata->getAlias(&statusWrapper, index),
			.owner = metadata->getOwner(&statusWrapper, index),
			.charSetId = metadata->getCharSet(&statusWrapper, index),
			.subType = metadata->getSubType(&statusWrapper, index),
		});
	}
}
