/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2006. Milan Babuskov.

  All Rights Reserved.

  $Id:  $

  Contributor(s):
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/AdvancedMessageDialog.h"
#include "Isaac.h"
#include "MasterPassword.h"
//-----------------------------------------------------------------------------
wxString encryptPassword(const wxString& password, const wxString& context)
{
    wxString mpw = MasterPassword::getMasterPassword();
    if (mpw.IsEmpty())
        return wxEmptyString;
    Isaac isc(mpw+context);
    return isc.getCipher(password);
}
//-----------------------------------------------------------------------------
wxString decryptPassword(const wxString& cipher, const wxString& context)
{
    wxString mpw = MasterPassword::getMasterPassword();
    if (mpw.IsEmpty())
        return wxEmptyString;
    Isaac isc(mpw+context);
    return isc.deCipher(cipher);
}
//-----------------------------------------------------------------------------
MasterPassword::MasterPassword()
{
}
//-----------------------------------------------------------------------------
MasterPassword& MasterPassword::getInstance()
{
    static MasterPassword mps;
    return mps;
}
//-----------------------------------------------------------------------------
wxString MasterPassword::getMasterPassword()
{
    wxString& mp = getInstance().mpw;

    AdvancedMessageBox(
        _("You should now enter the Master Password which\nis used to encrypt all passwords.\n\nIf you haven't used this feature before, then please\nenter the Master Password which will be used in future.\n\nYou can change the master password by\nselecting the appropriate option from View menu.\n\nPlease read the manual for more information."),
        _("Master password required"),
        wxOK, 0, 0, wxT("DIALOG_MasterPasswordNotice"));

    if (mp.IsEmpty())
    {
        mp = wxGetPasswordFromUser(
            _("Please enter the master password"),
            _("Enter master password"));
    }
    return mp;
}
//-----------------------------------------------------------------------------
void MasterPassword::setMasterPassword(const wxString& str)
{
    wxString& mp = getInstance().mpw;
    mp = str;
}
//-----------------------------------------------------------------------------
