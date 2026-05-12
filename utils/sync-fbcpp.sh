#!/bin/bash
# sync-fbcpp.sh - Synchronize internal fb-cpp patches with upstream submodule

set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
SUBMODULE_DIR="$REPO_ROOT/src/fb-cpp"
PATCH_SRC_DIR="$REPO_ROOT/src/engine/db/fbcpp/patches/fb-cpp"

echo "Checking submodule state..."
cd "$SUBMODULE_DIR"
SUBMODULE_REV=$(git rev-parse --short HEAD)

echo "Copying clean source from submodule to patches directory..."
# We exclude CMake related files that we manage at the project level
rsync -av --delete \
    --exclude="CMakeLists.txt" \
    --exclude=".github/" \
    --exclude=".vscode/" \
    --exclude="agents/" \
    --exclude="cmake/" \
    --exclude="doc/" \
    --exclude="vcpkg/" \
    "$SUBMODULE_DIR/src/fb-cpp/" "$PATCH_SRC_DIR/"

echo "Re-applying FlameRobin patches..."

# 1. Transaction.h - Add Client constructor and start()
perl -i -0777 -pe 's/class Transaction final\n\t\{/class Transaction final\n\t\{\n\tpublic:\n\t\texplicit Transaction(Client& client);/g' "$PATCH_SRC_DIR/Transaction.h"
perl -i -0777 -pe 's/void rollbackRetaining\(\);\n\n\tprivate:/void rollbackRetaining();\n\n\t\tvoid start(Attachment& attachment, const TransactionOptions& options = {});\n\n\tprivate:/g' "$PATCH_SRC_DIR/Transaction.h"

# 2. Transaction.cpp - Implementations
perl -i -0777 -pe 's/using namespace fbcpp::impl;\n\n\nstatic/using namespace fbcpp::impl;\n\n\nTransaction::Transaction(Client& client)\n\t: client{client},\n\t  handle{nullptr},\n\t  state{TransactionState::ROLLED_BACK}\n{\n}\n\n\nstatic/g' "$PATCH_SRC_DIR/Transaction.cpp"
perl -i -0777 -pe 's/void Transaction::rollbackRetaining\(\)\n\{\n\tassert\(isValid\(\)\);\n\tassert\(state == TransactionState::ACTIVE\);\n\n\tStatusWrapper statusWrapper\{client\};\n\n\thandle->rollbackRetaining\(&statusWrapper\);\n\}/void Transaction::rollbackRetaining()\n{\n\tassert(isValid());\n\tassert(state == TransactionState::ACTIVE);\n\n\tStatusWrapper statusWrapper{client};\n\n\thandle->rollbackRetaining(&statusWrapper);\n}\n\nvoid Transaction::start(Attachment& attachment, const TransactionOptions& options)\n{\n\tassert(!isValid());\n\n\tconst auto master = client.getMaster();\n\tStatusWrapper statusWrapper{client};\n\n\tauto tpbBuilder = buildTpb(master, statusWrapper, options);\n\tconst auto tpbBuffer = tpbBuilder->getBuffer(&statusWrapper);\n\tconst auto tpbBufferLen = tpbBuilder->getBufferLength(&statusWrapper);\n\n\thandle.reset(attachment.getHandle()->startTransaction(&statusWrapper, tpbBufferLen, tpbBuffer));\n\tstate = TransactionState::ACTIVE;\n}/g' "$PATCH_SRC_DIR/Transaction.cpp"

# 3. Statement.h - Add closeCursor() and fix truncation check
perl -i -0777 -pe 's/void free\(\);\n\n\t\t\/\/\/\n\t\t\/\/\/\ @brief Retrieves the textual legacy plan/void free();\n\n\t\tvoid closeCursor();\n\n\t\t\/\/\/\n\t\t\/\/\/\ @brief Retrieves the textual legacy plan/g' "$PATCH_SRC_DIR/Statement.h"
perl -i -pe 's/if \(value.length\(\) > descriptor.length\)/if (value.length() > descriptor.length - sizeof(std::uint16_t))/g' "$PATCH_SRC_DIR/Statement.h"

# 4. Statement.cpp - Implementations, prepare() decouple, and charset/length fix
perl -i -0777 -pe 's/statementHandle.reset\(attachment.getHandle\(\)->prepare\(&statusWrapper, transaction.getHandle\(\).get\(\),/statementHandle.reset(attachment.getHandle()->prepare(&statusWrapper, nullptr,/g' "$PATCH_SRC_DIR/Statement.cpp"
perl -i -0777 -pe 's/void Statement::free\(\)\n\{\n\tassert\(isValid\(\)\);\n\n\tif \(resultSetHandle\)\n\t\{\n\t\tresultSetHandle->close\(&statusWrapper\);\n\t\tresultSetHandle.reset\(\);\n\t\}\n\n\tstatementHandle->free\(&statusWrapper\);\n\tstatementHandle.reset\(\);\n\}/void Statement::free()\n{\n\tassert(isValid());\n\n\tcloseCursor();\n\n\tstatementHandle->free(&statusWrapper);\n\tstatementHandle.reset();\n}\n\nvoid Statement::closeCursor()\n{\n\tassert(isValid());\n\n\tif (resultSetHandle)\n\t{\n\t\tresultSetHandle->close(&statusWrapper);\n\t\tresultSetHandle.reset();\n\t}\n}/g' "$PATCH_SRC_DIR/Statement.cpp"

# Add charset 127 and length + 2 fix for TEXT to VARYING conversion
perl -i -0777 -pe 's/builder->setType\(&statusWrapper, index, SQL_VARYING\);\n\t\t\t\t\tdescriptor.adjustedType = DescriptorAdjustedType::STRING;/builder->setType(&statusWrapper, index, SQL_VARYING);\n\t\t\t\t\tbuilder->setLength(&statusWrapper, index, descriptor.length + 2);\n\t\t\t\t\tbuilder->setCharSet(&statusWrapper, index, 127); \/\/ CS_dynamic\n\t\t\t\t\tdescriptor.adjustedType = DescriptorAdjustedType::STRING;\n\t\t\t\t\tdescriptor.length += 2;\n\t\t\t\t\tdescriptor.charSetId = 127;/g' "$PATCH_SRC_DIR/Statement.cpp"

# Add charset 127 fix for existing VARYING
perl -i -0777 -pe 's/case DescriptorOriginalType::VARYING:\n\t\t\t\t\tbreak;/case DescriptorOriginalType::VARYING:\n\t\t\t\t\tif (!builder)\n\t\t\t\t\t\tbuilder.reset(metadata->getBuilder(&statusWrapper));\n\n\t\t\t\t\tbuilder->setCharSet(&statusWrapper, index, 127); \/\/ CS_dynamic\n\t\t\t\t\tdescriptor.charSetId = 127;\n\t\t\t\t\tbreak;/g' "$PATCH_SRC_DIR/Statement.cpp"

# Ensure length is re-read in second loop
perl -i -0777 -pe 's/descriptor.nullOffset = metadata->getNullOffset\(&statusWrapper, index\);/descriptor.nullOffset = metadata->getNullOffset(&statusWrapper, index);\n\t\t\t\tdescriptor.length = metadata->getLength(&statusWrapper, index);/g' "$PATCH_SRC_DIR/Statement.cpp"

# 5. BackupManager.h - Aggregate initialization fix
perl -i -pe 's/\.emplace_back\(value, ([^)]+)\)/.push_back({value, $1})/g' "$PATCH_SRC_DIR/BackupManager.h"

echo "Done. Internal fb-cpp source updated from submodule revision $SUBMODULE_REV."
echo "Please verify the changes and commit."
