/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "config/Config.h"
#include "gui/AdvancedMessageDialog.h"
#include "Isaac.h"
#include "MasterPassword.h"

wxString encryptPassword(const wxString& password, const wxString& context)
{
    wxString mpw = MasterPassword::getMasterPassword();
    if (mpw.IsEmpty())
        return wxEmptyString;
    Isaac isc(mpw+context);
    return isc.getCipher(password);
}

wxString decryptPassword(const wxString& cipher, const wxString& context)
{
    wxString mpw = MasterPassword::getMasterPassword();
    if (mpw.IsEmpty())
        return wxEmptyString;
    Isaac isc(mpw+context);
    return isc.deCipher(cipher);
}

MasterPassword::MasterPassword()
{
}

MasterPassword& MasterPassword::getInstance()
{
    static MasterPassword mps;
    return mps;
}

wxString MasterPassword::getMasterPassword()
{
    wxString& mp = getInstance().mpw;
    if (mp.IsEmpty())
    {
        wxString msg(_("If you are already using FlameRobin's encrypted passwords, please enter the master password now, it will be used for the entire session."));
        msg += "\n\n";
        msg += _("If you are using FlameRobin's encrypted passwords for the first time, please enter your master password now.");
        msg += "\n\n";
        msg += _("Please consult the manual for more information about the master password feature.");
        showInformationDialog(0, _("Master Password is required."), msg,
            AdvancedMessageDialogButtonsOk(), config(), "DIALOG_MasterPasswordNotice",
            _("Do not show this information again"));
        mp = wxGetPasswordFromUser(
            _("Please enter the master password"),
            _("Enter master password"));
    }
    return mp;
}

void MasterPassword::setMasterPassword(const wxString& str)
{
    wxString& mp = getInstance().mpw;
    mp = str;
}

