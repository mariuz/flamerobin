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

  The Initial Developer of the Original Code is Milan Babuskov

  Portions created by the original developer
  are Copyright (C) 2006. Milan Babuskov

  All Rights Reserved.

  $Id:  $

  Contributor(s):
*/

#ifndef FR_MASTERPASSWORD_H
#define FR_MASTERPASSWORD_H
//-----------------------------------------------------------------------------
wxString encryptPassword(const wxString& password, const wxString& context);
wxString decryptPassword(const wxString& cipher, const wxString& context);
//-----------------------------------------------------------------------------
class MasterPassword
{
private:
    static MasterPassword& getInstance();
    wxString mpw;
    MasterPassword();

public:
    static wxString getMasterPassword();

    // before doing this you may want to decrypt passwords for all
    // databases and change them as well
    static void setMasterPassword(const wxString& str);
};
//-----------------------------------------------------------------------------
#endif
