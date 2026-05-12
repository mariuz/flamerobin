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

# 3. Statement.h - Add closeCursor()
perl -i -0777 -pe 's/void free\(\);\n\n\t\t\/\/\/\n\t\t\/\/\/\ @brief Retrieves the textual legacy plan/void free();\n\n\t\tvoid closeCursor();\n\n\t\t\/\/\/\n\t\t\/\/\/\ @brief Retrieves the textual legacy plan/g' "$PATCH_SRC_DIR/Statement.h"

# 4. Statement.cpp - Implementations and prepare() decouple
perl -i -0777 -pe 's/statementHandle.reset\(attachment.getHandle\(\)->prepare\(&statusWrapper, transaction.getHandle\(\).get\(\),/statementHandle.reset(attachment.getHandle()->prepare(&statusWrapper, nullptr,/g' "$PATCH_SRC_DIR/Statement.cpp"
perl -i -0777 -pe 's/void Statement::free\(\)\n\{\n\tassert\(isValid\(\)\);\n\n\tif \(resultSetHandle\)\n\t\{\n\t\tresultSetHandle->close\(&statusWrapper\);\n\t\tresultSetHandle.reset\(\);\n\t\}\n\n\tstatementHandle->free\(&statusWrapper\);\n\tstatementHandle.reset\(\);\n\}/void Statement::free()\n{\n\tassert(isValid());\n\n\tcloseCursor();\n\n\tstatementHandle->free(&statusWrapper);\n\tstatementHandle.reset();\n}\n\nvoid Statement::closeCursor()\n{\n\tassert(isValid());\n\n\tif (resultSetHandle)\n\t{\n\t\tresultSetHandle->close(&statusWrapper);\n\t\tresultSetHandle.reset();\n\t}\n}/g' "$PATCH_SRC_DIR/Statement.cpp"

# 5. BackupManager.h - Aggregate initialization fix
perl -i -pe 's/\.emplace_back\(value, ([^)]+)\)/.push_back({value, $1})/g' "$PATCH_SRC_DIR/BackupManager.h"

echo "Done. Internal fb-cpp source updated from submodule revision $SUBMODULE_REV."
echo "Please verify the changes and commit."
