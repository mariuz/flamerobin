/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "engine/db/fbcpp/FbCppExtensions.h"
#include <fb-cpp/Attachment.h>
#include <fb-cpp/Client.h>
#include <fb-cpp/Row.h>
#include <fb-cpp/Exception.h>
#include <firebird/Interface.h>

namespace fbcpp
{

StatementExt::StatementExt(Attachment& attachment, Transaction& transaction, std::string_view sql,
    const StatementOptions& options)
    : Statement(attachment)
{
    unsigned flags = fb::IStatement::PREPARE_PREFETCH_METADATA;

    if (options.getPrefetchLegacyPlan())
        flags |= fb::IStatement::PREPARE_PREFETCH_LEGACY_PLAN;

    if (options.getPrefetchPlan())
        flags |= fb::IStatement::PREPARE_PREFETCH_DETAILED_PLAN;

    // Use nullptr for transaction during prepare to decouple from specific transaction.
    statementHandle.reset(attachment.getHandle()->prepare(&statusWrapper, nullptr,
        static_cast<unsigned>(sql.length()), sql.data(), options.getDialect(), flags));

    if (options.getCursorName().has_value())
        statementHandle->setCursorName(&statusWrapper, options.getCursorName()->c_str());

    if (options.getCursorType() == CursorType::SCROLLABLE)
        cursorFlags = fb::IStatement::CURSOR_TYPE_SCROLLABLE;

    type = static_cast<StatementType>(statementHandle->getType(&statusWrapper));

    auto processMetadata = [&](FbRef<fb::IMessageMetadata>& metadata, std::vector<Descriptor>& descriptors,
                                     std::vector<std::byte>& message)
    {
        if (!metadata)
            return;

        message.resize(metadata->getMessageLength(&statusWrapper));

        FbRef<fb::IMetadataBuilder> builder;

        const auto count = metadata->getCount(&statusWrapper);
        descriptors.reserve(count);

        for (unsigned index = 0u; index < count; ++index)
        {
            Descriptor descriptor{
                .originalType = static_cast<DescriptorOriginalType>(metadata->getType(&statusWrapper, index)),
                .adjustedType = static_cast<DescriptorAdjustedType>(metadata->getType(&statusWrapper, index)),
                .scale = metadata->getScale(&statusWrapper, index),
                .length = metadata->getLength(&statusWrapper, index),
                .offset = 0,
                .nullOffset = 0,
                .isNullable = static_cast<bool>(metadata->isNullable(&statusWrapper, index)),
                .name = metadata->getField(&statusWrapper, index),
                .relation = metadata->getRelation(&statusWrapper, index),
                .alias = metadata->getAlias(&statusWrapper, index),
                .owner = metadata->getOwner(&statusWrapper, index),
                .charSetId = metadata->getCharSet(&statusWrapper, index),
                .subType = metadata->getSubType(&statusWrapper, index),
            };

            switch (descriptor.originalType)
            {
                case DescriptorOriginalType::TEXT:
                    if (!builder)
                        builder.reset(metadata->getBuilder(&statusWrapper));

                    builder->setType(&statusWrapper, index, static_cast<unsigned>(DescriptorOriginalType::VARYING));
                    builder->setLength(&statusWrapper, index, descriptor.length + 2);
                    builder->setCharSet(&statusWrapper, index, 127); // CS_dynamic

                    descriptor.adjustedType = DescriptorAdjustedType::STRING;
                    descriptor.length += 2;
                    descriptor.charSetId = 127;
                    break;

                case DescriptorOriginalType::VARYING:
                    if (!builder)
                        builder.reset(metadata->getBuilder(&statusWrapper));

                    builder->setCharSet(&statusWrapper, index, 127); // CS_dynamic
                    descriptor.adjustedType = DescriptorAdjustedType::STRING;
                    descriptor.charSetId = 127;
                    break;

                case DescriptorOriginalType::SHORT:
                case DescriptorOriginalType::LONG:
                case DescriptorOriginalType::INT64:
                    if (descriptor.scale != 0)
                        descriptor.adjustedType = DescriptorAdjustedType::INT64;
                    else
                    {
                        switch (descriptor.originalType)
                        {
                            case DescriptorOriginalType::SHORT:
                                descriptor.adjustedType = DescriptorAdjustedType::INT16;
                                break;
                            case DescriptorOriginalType::LONG:
                                descriptor.adjustedType = DescriptorAdjustedType::INT32;
                                break;
                            case DescriptorOriginalType::INT64:
                                descriptor.adjustedType = DescriptorAdjustedType::INT64;
                                break;
                            default:
                                break;
                        }
                    }
                    break;

                case DescriptorOriginalType::FLOAT:
                    descriptor.adjustedType = DescriptorAdjustedType::FLOAT;
                    break;

                case DescriptorOriginalType::DOUBLE:
                    descriptor.adjustedType = DescriptorAdjustedType::DOUBLE;
                    break;

                case DescriptorOriginalType::TIMESTAMP:
                    descriptor.adjustedType = DescriptorAdjustedType::TIMESTAMP;
                    break;

                case DescriptorOriginalType::DATE:
                    descriptor.adjustedType = DescriptorAdjustedType::DATE;
                    break;

                case DescriptorOriginalType::TIME:
                    descriptor.adjustedType = DescriptorAdjustedType::TIME;
                    break;

                case DescriptorOriginalType::TIMESTAMP_TZ:
                case DescriptorOriginalType::TIMESTAMP_TZ_EX:
                    descriptor.adjustedType = DescriptorAdjustedType::TIMESTAMP_TZ;
                    break;

                case DescriptorOriginalType::TIME_TZ:
                case DescriptorOriginalType::TIME_TZ_EX:
                    descriptor.adjustedType = DescriptorAdjustedType::TIME_TZ;
                    break;

                case DescriptorOriginalType::INT128:
                    descriptor.adjustedType = DescriptorAdjustedType::INT128;
                    break;

                case DescriptorOriginalType::DEC16:
                    descriptor.adjustedType = DescriptorAdjustedType::DECFLOAT16;
                    break;

                case DescriptorOriginalType::DEC34:
                    descriptor.adjustedType = DescriptorAdjustedType::DECFLOAT34;
                    break;

                case DescriptorOriginalType::BOOLEAN:
                    descriptor.adjustedType = DescriptorAdjustedType::BOOLEAN;
                    break;

                case DescriptorOriginalType::BLOB:
                    descriptor.adjustedType = DescriptorAdjustedType::BLOB;
                    break;

                default:
                    break;
            }

            descriptors.push_back(std::move(descriptor));
        }

        if (builder)
        {
            metadata.reset(builder->getMetadata(&statusWrapper));
            message.resize(metadata->getMessageLength(&statusWrapper));
        }

        for (unsigned index = 0u; index < count; ++index)
        {
            descriptors[index].offset = metadata->getOffset(&statusWrapper, index);
            descriptors[index].nullOffset = metadata->getNullOffset(&statusWrapper, index);
            descriptors[index].length = metadata->getLength(&statusWrapper, index);
        }
    };

    inMetadata.reset(statementHandle->getInputMetadata(&statusWrapper));
    processMetadata(inMetadata, inDescriptors, inMessage);

    outMetadata.reset(statementHandle->getOutputMetadata(&statusWrapper));
    processMetadata(outMetadata, outDescriptors, outMessage);

    outRow = std::make_unique<Row>(attachment.getClient(), outDescriptors, std::span{outMessage});
}

void StatementExt::closeCursor()
{
    assert(isValid());

    if (resultSetHandle)
    {
        resultSetHandle->close(&statusWrapper);
        resultSetHandle.reset();
    }
}

TransactionExt::TransactionExt(Client& client)
    : Transaction(client)
{
}

void TransactionExt::start(Attachment& attachment, const TransactionOptions& options)
{
    assert(!isValid());

    const auto master = client.getMaster();
    impl::StatusWrapper statusWrapper{client};

    auto tpbBuilder = Transaction::buildTpb(master, statusWrapper, options);
    const auto tpbBuffer = tpbBuilder->getBuffer(&statusWrapper);
    const auto tpbBufferLen = tpbBuilder->getBufferLength(&statusWrapper);

    handle.reset(attachment.getHandle()->startTransaction(&statusWrapper, tpbBufferLen, tpbBuffer));
    state = TransactionState::ACTIVE;
}

} // namespace fbcpp
