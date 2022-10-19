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

#ifndef MAIN_H
#define MAIN_H


#include "config/LocalSettings.h"
class MainFrame;

class Application: public wxApp
{
private:
    // format local settings 
    // TODO: multilingual
    LocalSettings* localSetM;
    // Open databases whose file names were given as command line parameters
    wxArrayString cmdlineParamsM;
    void openDatabasesFromParams(MainFrame* frFrame);
    // Reads the environment variables that influence FR's behaviour.
    void checkEnvironment();
    // Reads the command line params that influence FR's behaviour.
    void parseCommandLine();
    // Translates the supported macros (like $app and $user) in path
    // specifications coming from the command line or the environment.
    const wxString translatePathMacros(const wxString path) const;
protected:
    virtual const wxString getConfigurableObjectId() const;
public:
    bool OnInit();
    virtual bool OnExceptionInMainLoop();
    void OnFatalException();
    virtual void HandleEvent(wxEvtHandler* handler, wxEventFunction func,
        wxEvent& event) const;
    int OnExit();
};

DECLARE_APP(Application)

#endif // MAIN_H
