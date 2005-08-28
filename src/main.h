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

  The Initial Developer of the Original Code is Nando Dessena.

  Portions created by the original developer
  are Copyright (C) 2004 Nando Dessena.

  All Rights Reserved.

  Contributor(s):
*/

#ifndef MAIN_H
#define MAIN_H

//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
class FRApp: public wxApp {
public:
    bool OnInit();
    void OnFatalException();
private:
    // Reads the environment variables that influence FR's behaviour.
    void checkEnvironment();
    // Reads the command line params that influence FR's behaviour.
    void parseCommandLine();
    // Translates the supported macros (like $app and $user) in path
    // specifications coming from the command line or the environment.
    const string translatePathMacros(const string path) const;
};
//-----------------------------------------------------------------------------
DECLARE_APP(FRApp)
//-----------------------------------------------------------------------------
#endif // MAIN_H
