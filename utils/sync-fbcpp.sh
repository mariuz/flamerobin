#!/bin/bash
# sync-fbcpp.sh - Synchronize internal fb-cpp patches with upstream submodule

set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
SUBMODULE_DIR="$REPO_ROOT/src/fb-cpp"
PATCH_FILE="$REPO_ROOT/ports/fb-cpp/fb-cpp-flamerobin.patch"
TEMP_DIR=$(mktemp -d)
PATCH_SRC_DIR="$TEMP_DIR/patched"

echo "Checking submodule state..."
cd "$SUBMODULE_DIR"
SUBMODULE_REV=$(git rev-parse --short HEAD)

echo "Preparing patched source in temporary directory..."
mkdir -p "$PATCH_SRC_DIR"
rsync -av --delete \
    --exclude="CMakeLists.txt" \
    --exclude=".github/" \
    --exclude=".vscode/" \
    --exclude="agents/" \
    --exclude="cmake/" \
    --exclude="doc/" \
    --exclude="vcpkg/" \
    "$SUBMODULE_DIR/src/fb-cpp/" "$PATCH_SRC_DIR/"

echo "Re-applying FlameRobin patches to temporary directory..."

# 0. Enable inheritance and extensions (remove final and change private to protected)
for f in Attachment.h Blob.h Statement.h Transaction.h; do
    perl -i -pe 's/class ([A-Za-z]+) final/class $1/g' "$PATCH_SRC_DIR/$f"
    perl -i -pe 's/private:/protected:/g' "$PATCH_SRC_DIR/$f"
done

# 1. Transaction.h - Add Client constructor and start()
perl -i -pe 's/#include <span>/#include <cstdint>\n#include <span>/g' "$PATCH_SRC_DIR/Transaction.h"
perl -i -0777 -pe 's/class Transaction\n\t\{/class Transaction\n\t\{\n\tpublic:\n\t\texplicit Transaction(Client& client);/g' "$PATCH_SRC_DIR/Transaction.h"
perl -i -0777 -pe 's/void rollbackRetaining\(\);\n\n\tprotected:/void rollbackRetaining();\n\n\t\tvoid start(Attachment& attachment, const TransactionOptions& options = {});\n\n\tprotected:/g' "$PATCH_SRC_DIR/Transaction.h"

# 2. Transaction.cpp - Implementations
perl -i -0777 -pe 's/using namespace fbcpp::impl;\n\n\nstatic/using namespace fbcpp::impl;\n\n\nTransaction::Transaction(Client& client)\n\t: client{client},\n\t  handle{nullptr},\n\t  state{TransactionState::ROLLED_BACK}\n{\n}\n\n\nstatic/g' "$PATCH_SRC_DIR/Transaction.cpp"
perl -i -0777 -pe 's/void Transaction::rollbackRetaining\(\)\n\{\n\tassert\(isValid\(\)\);\n\tassert\(state == TransactionState::ACTIVE\);\n\n\tStatusWrapper statusWrapper\{client\};\n\n\thandle->rollbackRetaining\(&statusWrapper\);\n\}/void Transaction::rollbackRetaining()\n{\n\tassert(isValid());\n\tassert(state == TransactionState::ACTIVE);\n\n\tStatusWrapper statusWrapper{client};\n\n\thandle->rollbackRetaining(&statusWrapper);\n}\n\nvoid Transaction::start(Attachment& attachment, const TransactionOptions& options)\n{\n\tassert(!isValid());\n\n\tconst auto master = client.getMaster();\n\tStatusWrapper statusWrapper{client};\n\n\tauto tpbBuilder = buildTpb(master, statusWrapper, options);\n\tconst auto tpbBuffer = tpbBuilder->getBuffer(&statusWrapper);\n\tconst auto tpbBufferLen = tpbBuffer->getBufferLength(&statusWrapper);\n\n\thandle.reset(attachment.getHandle()->startTransaction(&statusWrapper, tpbBufferLen, tpbBuffer));\n\tstate = TransactionState::ACTIVE;\n}/g' "$PATCH_SRC_DIR/Transaction.cpp"

# Export buildTpb for extensions
perl -i -pe 's/static FbUniquePtr<fb::IXpbBuilder> buildTpb/FbUniquePtr<fb::IXpbBuilder> Transaction::buildTpb/g' "$PATCH_SRC_DIR/Transaction.cpp"
perl -i -0777 -pe 's/void start\(Attachment& attachment, const TransactionOptions& options = \{\}\);\n\n\tprotected:/void start(Attachment& attachment, const TransactionOptions& options = {});\n\n\t\tstatic FbUniquePtr<fb::IXpbBuilder> buildTpb(fb::IMaster* master, impl::StatusWrapper& statusWrapper, const TransactionOptions& options);\n\n\tprotected:/g' "$PATCH_SRC_DIR/Transaction.h"

# 3. Statement.h - Add closeCursor(), fix truncation check, and add Attachment constructor
perl -i -0777 -pe 's/void free\(\);\n\n\t\t\/\/\/\n\t\t\/\/\/\ @brief Retrieves the textual legacy plan/void free();\n\n\t\tvoid closeCursor();\n\n\t\t\/\/\/\n\t\t\/\/\/\ @brief Retrieves the textual legacy plan/g' "$PATCH_SRC_DIR/Statement.h"
perl -i -pe 's/if \(value.length\(\) > descriptor.length\)/if (value.length() > descriptor.length - sizeof(std::uint16_t))/g' "$PATCH_SRC_DIR/Statement.h"
perl -i -0777 -pe 's/class Statement\n\t\{/class Statement\n\t\{\n\tpublic:\n\t\texplicit Statement(Attachment& attachment);/g' "$PATCH_SRC_DIR/Statement.h"

# 4. Statement.cpp - Implementations, prepare() decouple, and charset/length fix
perl -i -0777 -pe 's/using namespace fbcpp::impl;\n\n\nStatement::Statement/using namespace fbcpp::impl;\n\n\nStatement::Statement(Attachment& attachment)\n\t: attachment{&attachment},\n\t  statusWrapper{attachment.getClient()},\n\t  calendarConverter{attachment.getClient()},\n\t  numericConverter{attachment.getClient()}\n{\n}\n\n\nStatement::Statement/g' "$PATCH_SRC_DIR/Statement.cpp"
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

echo "Generating new patch file..."
cd "$TEMP_DIR"
git init
mkdir -p src/fb-cpp
cp -r "$SUBMODULE_DIR/src/fb-cpp/"* src/fb-cpp/
git add .
git commit -m "base"
cp -r "$PATCH_SRC_DIR/"* src/fb-cpp/
git add .
git diff --cached > "$PATCH_FILE"

echo "Cleaning up..."
rm -rf "$TEMP_DIR"

echo "Done. Patch file updated from submodule revision $SUBMODULE_REV."
echo "Please verify the changes and commit."
