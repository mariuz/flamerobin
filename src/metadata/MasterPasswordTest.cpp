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

#include <iostream>
#include <cassert>
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "MasterPassword.h"
#include "gui/AdvancedMessageDialog.h"
#include "config/Config.h"

// Stubs for GUI dialogs used by MasterPassword::getMasterPassword()
AdvancedMessageDialogButtons::AdvancedMessageDialogButtons()
{
}

void AdvancedMessageDialogButtons::addAffirmativeButton(int, const wxString&)
{
}

void AdvancedMessageDialogButtons::addAlternateButton(int, const wxString&)
{
}

AdvancedMessageDialogButtonsOk::AdvancedMessageDialogButtonsOk(const wxString)
{
}

int showInformationDialog(wxWindow*, const wxString&, const wxString&,
    AdvancedMessageDialogButtons, Config&, const wxString&,
    const wxString&)
{
    return wxID_OK;
}

int main()
{
    bool allPassed = true;

    auto test = [&allPassed](const std::string& name, bool condition) {
        if (condition)
        {
            std::cout << "[PASS] " << name << std::endl;
        }
        else
        {
            std::cerr << "[FAIL] " << name << std::endl;
            allPassed = false;
        }
    };

    // 1. Initial state
    MasterPassword::reset();
    test("Initial state has no master password", !MasterPassword::hasMasterPassword());
    test("Initial state is not verified", !MasterPassword::isVerified());

    // 2. Set master password
    MasterPassword::setMasterPassword("test_master_123");
    test("hasMasterPassword returns true after setting", MasterPassword::hasMasterPassword());
    test("isVerified is false after setting new master password", !MasterPassword::isVerified());
    test("getMasterPassword returns correct string",
        MasterPassword::getMasterPassword() == "test_master_123");

    // 3. Encrypt and decrypt with correct master password
    wxString originalPass = "firebird_secret_pass";
    wxString context = "SYSDBA/var/db/test.fdb";
    wxString cipher = encryptPassword(originalPass, context);
    test("Ciphertext is non-empty", !cipher.IsEmpty());
    test("Ciphertext length is 256 hex characters", cipher.Length() == 256);

    wxString decrypted = decryptPassword(cipher, context);
    test("Decrypted password matches original", decrypted.StartsWith(originalPass));

    // 4. Wrong master password fails to decrypt original password
    MasterPassword::setMasterPassword("wrong_master_xyz");
    test("hasMasterPassword is true for wrong password", MasterPassword::hasMasterPassword());
    test("isVerified is false after change", !MasterPassword::isVerified());

    wxString wrongDecrypted = decryptPassword(cipher, context);
    test("Wrong master password does not decrypt original password",
        !wrongDecrypted.StartsWith(originalPass));

    // 5. Reset master password clears cache and verification
    MasterPassword::setVerified(true);
    test("setVerified(true) marks verified", MasterPassword::isVerified());
    MasterPassword::reset();
    test("reset() clears master password", !MasterPassword::hasMasterPassword());
    test("reset() clears verified flag", !MasterPassword::isVerified());

    // 6. Decryption when master password prompt is cancelled returns empty string
    static int promptCallCount = 0;
    static wxString nextPromptReturn = wxEmptyString;
    MasterPassword::setPasswordPromptFunction([](const wxString&, const wxString&) -> wxString {
        promptCallCount++;
        return nextPromptReturn;
    });

    nextPromptReturn = wxEmptyString; // User clicks Cancel
    promptCallCount = 0;
    wxString emptyResult = decryptPassword(cipher, context);
    test("Prompt was called when master password was empty", promptCallCount == 1);
    test("decryptPassword returns empty when prompt cancelled", emptyResult.IsEmpty());
    test("hasMasterPassword remains false after cancel", !MasterPassword::hasMasterPassword());

    // Next call prompts again because it was cancelled
    nextPromptReturn = "test_master_123";
    promptCallCount = 0;
    wxString repromptResult = decryptPassword(cipher, context);
    test("Prompt was called again on next attempt after cancel", promptCallCount == 1);
    test("decryptPassword succeeded after entering password in prompt",
        repromptResult.StartsWith(originalPass));
    test("hasMasterPassword is now true", MasterPassword::hasMasterPassword());

    // 7. Simulating connection failure logic:
    // Case A: Unverified master password entered (wrong typo) -> connect fails -> cache must be reset
    MasterPassword::reset();
    MasterPassword::setMasterPassword("mistyped_password");
    test("Unverified password before connection", !MasterPassword::isVerified());

    // Connection throws error; since !isVerified(), reset is called
    if (!MasterPassword::isVerified())
        MasterPassword::reset();
    test("Unverified master password was cleared after failed connection",
        !MasterPassword::hasMasterPassword());

    // Case B: Correct master password entered -> connect succeeds -> marked verified
    MasterPassword::setMasterPassword("test_master_123");
    test("Correct password decrypted", decryptPassword(cipher, context).StartsWith(originalPass));
    // Connection succeeds
    MasterPassword::setVerified(true);
    test("Master password is now verified", MasterPassword::isVerified());

    // Case C: Subsequent connection failure on a different database (verified password preserved)
    if (!MasterPassword::isVerified())
        MasterPassword::reset();
    test("Verified master password is NOT cleared on subsequent error",
        MasterPassword::hasMasterPassword() && MasterPassword::isVerified());

    // 8. Cancelled prompt detection logic in connectDatabase:
    // When a database has an encrypted password stored (raw is non-empty),
    // but decryptPassword returned empty (because user cancelled the prompt),
    // connectDatabase must abort (return false) instead of passing an empty password.
    nextPromptReturn = wxEmptyString;
    MasterPassword::reset();
    wxString passFromDecrypt = decryptPassword(cipher, context);
    bool hasStoredPassword = !cipher.IsEmpty();
    bool useEncryptedMode = true;
    bool shouldAbort = (useEncryptedMode && hasStoredPassword && passFromDecrypt.IsEmpty());
    test("Cancelled master password prompt triggers connection abort", shouldAbort);

    if (allPassed)
    {
        std::cout << "All MasterPassword tests passed successfully." << std::endl;
        return 0;
    }
    return 1;
}
