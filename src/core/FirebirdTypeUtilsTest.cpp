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

#include <wx/wx.h>
#include "core/StringUtils.h"
#include "engine/db/ITransaction.h"
#include "firebird/constants.h"

// Simple test framework mock for FlameRobin
bool check(bool condition, const wxString& message)
{
    if (!condition)
    {
        wxPrintf("FAILED: %s\n", message.c_str());
        return false;
    }
    return true;
}

int main()
{
    bool ok = true;

    ok = check(isolationLevelToString(fr::TransactionIsolationLevel::Consistency) == "Consistency", "Consistency isolation level string") && ok;
    ok = check(isolationLevelToString(fr::TransactionIsolationLevel::Concurrency) == "Concurrency", "Concurrency isolation level string") && ok;
    ok = check(isolationLevelToString(fr::TransactionIsolationLevel::ReadDirty) == "Read Dirty", "Read Dirty isolation level string") && ok;
    ok = check(isolationLevelToString(fr::TransactionIsolationLevel::ReadCommitted) == "Read Committed", "Read Committed isolation level string") && ok;
    ok = check(isolationLevelToString(fr::TransactionIsolationLevel::ReadConsistency) == "Read Consistency", "Read Consistency isolation level string") && ok;

    ok = check(cryptStateToString(0) == "Not encrypted", "Not encrypted state") && ok;
    ok = check(cryptStateToString(fb_info_crypt_encrypted) == "Encrypted", "Encrypted state") && ok;
    ok = check(cryptStateToString(fb_info_crypt_process) == "Encryption in progress", "Encryption in progress state") && ok;
    ok = check(cryptStateToString(fb_info_crypt_encrypted | fb_info_crypt_process) == "Decryption in progress", "Decryption in progress state") && ok;

    if (ok)
    {
        wxPrintf("All tests PASSED\n");
        return 0;
    }
    else
    {
        wxPrintf("Some tests FAILED\n");
        return 1;
    }
}
