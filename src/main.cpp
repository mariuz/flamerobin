/*
  Copyright (c) 2004-2012 The FlameRobin Development Team

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


  $Id$

*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/cmdline.h>
#include <wx/sysopt.h>
#include <wx/utils.h>

#include <exception>

#include <ibpp.h>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/MainFrame.h"
#include "main.h"
//-----------------------------------------------------------------------------
IMPLEMENT_APP(Application)
//-----------------------------------------------------------------------------
void parachute()
{
    if (wxYES == ::wxMessageBox(::wxGetTranslation(
        wxT("A fatal error has occured. If you know how to\n")
        wxT("reproduce the problem, please submit the bug report at:\n")
        wxT("http://www.flamerobin.org/bugs.php\n\n")
        wxT("The program can try to keep running so that you\n")
        wxT("can save your data. Do you wish to try?\n")),
        _("Fatal error"), wxYES_NO | wxICON_ERROR))
    {
        int result = ::wxGetApp().OnRun();
        exit(result);
    }
    exit(1);
}
//-----------------------------------------------------------------------------
bool Application::OnExceptionInMainLoop()
{
    return true;
}
//-----------------------------------------------------------------------------
void Application::OnFatalException()
{
    parachute();
}
//-----------------------------------------------------------------------------
bool Application::OnInit()
{
#if wxUSE_ON_FATAL_EXCEPTION
    ::wxHandleFatalExceptions();
#endif

    std::set_terminate(parachute);
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

    /* we might be needing this
#ifdef __WXMSW__
    if (wxTheApp->GetComCtl32Version() >= 600 && ::wxDisplayDepth() >= 32)
        wxSystemOptions::SetOption(wxT("msw.remap"), 2);
    else
        wxSystemOptions::SetOption(wxT("msw.remap"), 0);
#endif
    */
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), true);

    MainFrame* main_frame = new MainFrame(0, -1, wxT(""));
    SetTopWindow(main_frame);
    main_frame->Show();

    openDatabasesFromParams(main_frame);
    return true;
}
//-----------------------------------------------------------------------------
void Application::HandleEvent(wxEvtHandler* handler, wxEventFunction func,
    wxEvent& event) const
{
    try
    {
        wxAppConsole::HandleEvent(handler, func, event);
    }
    catch (const FRAbort&)
    {
        // Do nothing. FRAbort is a silent exception.
    }
    catch (const std::exception& e)
    {
        wxMessageBox(std2wx(e.what()), _("Unhandled Error in FlameRobin"),
            wxOK | wxICON_ERROR, wxGetTopLevelParent(wxGetActiveWindow()));
    }
}
//-----------------------------------------------------------------------------
void Application::checkEnvironment()
{
    wxString envVar;
    if (wxGetEnv(wxT("FR_HOME"), &envVar))
        config().setHomePath(translatePathMacros(envVar));
    if (wxGetEnv(wxT("FR_USER_HOME"), &envVar))
        config().setUserHomePath(translatePathMacros(envVar));
}
//-----------------------------------------------------------------------------
void Application::parseCommandLine()
{
    wxCmdLineParser parser(wxGetApp().argc, wxGetApp().argv);
    parser.AddOption(wxT("h"), wxT("home"), _("Set FlameRobin's home path"));
    parser.AddOption(wxT("uh"), wxT("user-home"),
        _("Set FlameRobin's user home path"));
    // open databases given as command line parameters
    parser.AddParam(_("File name of database to open"), wxCMD_LINE_VAL_STRING,
        wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);

    if (parser.Parse() == 0)
    {
        wxString paramValue;
        if (parser.Found(wxT("home"), &paramValue))
            config().setHomePath(translatePathMacros(paramValue));

        if (parser.Found(wxT("user-home"), &paramValue))
            config().setUserHomePath(translatePathMacros(paramValue));

        for (size_t i = 0; i < parser.GetParamCount(); i++)
            cmdlineParamsM.Add(parser.GetParam(i));
    }
}
//-----------------------------------------------------------------------------
const wxString Application::translatePathMacros(const wxString path) const
{
    if (path == wxT("$app"))
        return config().getLocalDataDir();
    else if (path == wxT("$user"))
        return config().getUserLocalDataDir();
    else
        return path;
}
//-----------------------------------------------------------------------------
const wxString Application::getConfigurableObjectId() const
{
    return wxT("");
}
//-----------------------------------------------------------------------------
void Application::openDatabasesFromParams(MainFrame* frFrame)
{
    if (cmdlineParamsM.GetCount())
    {
        for (size_t i = 0; i < cmdlineParamsM.GetCount(); i++)
            frFrame->openUnregisteredDatabase(cmdlineParamsM[i]);
        cmdlineParamsM.Clear();
    }
}
//-----------------------------------------------------------------------------
