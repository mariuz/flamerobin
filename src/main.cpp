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
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/cmdline.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <vector>
#include <string>

#include <ibpp.h>

#include "config/Config.h"
#include "gui/MainFrame.h"
#include "main.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
IMPLEMENT_APP(FRApp)
//-----------------------------------------------------------------------------
void FRApp::OnFatalException()
{
    if (wxYES == ::wxMessageBox(::wxGetTranslation(
        wxT("A fatal error has occured. If you know how to\n")
        wxT("reproduce the problem, please submit the bug report at:\n")
        wxT("http://sourceforge.net/tracker/?group_id=124340&atid=699234\n\n")
        wxT("The program can try to keep running so that you\n")
        wxT("can save your data. Do you wish to try?\n")),
        _("Fatal error"), wxYES_NO | wxICON_ERROR))
    {
        MainLoop();
    }
}
//-----------------------------------------------------------------------------
bool FRApp::OnInit()
{
#if wxUSE_ON_FATAL_EXCEPTION
    ::wxHandleFatalExceptions();
#endif

    checkEnvironment();
    parseCommandLine();

    // initialize IBPP library - if it fails: exit
    try
    {
        if (! IBPP::CheckVersion(IBPP::Version))
        {
            wxMessageBox(_("Wrong IBPP version."), _("Error."), wxOK | wxICON_ERROR);
            return false;
        }
    }
    catch (IBPP::Exception &e)
    {
        wxMessageBox(std2wx(e.ErrorMessage()), _("Error initalizing IBPP library."), wxOK | wxICON_ERROR);
        return false;
    }

    wxImage::AddHandler(new wxPNGHandler);

    MainFrame* main_frame = new MainFrame(0, -1, wxT(""));
    SetTopWindow(main_frame);
    main_frame->Show();

    return true;
}
//-----------------------------------------------------------------------------
void FRApp::checkEnvironment()
{
    wxString envVar;
    if (wxGetEnv(wxT("FR_HOME"), &envVar))
        config().setHomePath(translatePathMacros(wx2std(envVar)));
    if (wxGetEnv(wxT("FR_USER_HOME"), &envVar))
        config().setUserHomePath(translatePathMacros(wx2std(envVar)));
}
//-----------------------------------------------------------------------------
void FRApp::parseCommandLine()
{
    wxCmdLineParser parser(wxGetApp().argc, wxGetApp().argv);
    parser.AddOption(wxT("h"), wxT("home"));
    parser.AddOption(wxT("uh"), wxT("user-home"));
    if (parser.Parse() == 0)
    {
        wxString paramValue;
        if (parser.Found(wxT("home"), &paramValue))
            config().setHomePath(translatePathMacros(wx2std(paramValue)));

        if (parser.Found(wxT("user-home"), &paramValue))
            config().setUserHomePath(translatePathMacros(wx2std(paramValue)));
    }
}
//-----------------------------------------------------------------------------
const string FRApp::translatePathMacros(const string path) const
{
    if (path == "$app")
        return wx2std(wxStandardPaths::Get().GetLocalDataDir());
    else if (path == "$user")
        return wx2std(wxStandardPaths::Get().GetUserLocalDataDir());
    else
        return path;
}
//-----------------------------------------------------------------------------
