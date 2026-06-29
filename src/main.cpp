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

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/cmdline.h>
#include <wx/settings.h>
#include <wx/sysopt.h>
#include <wx/utils.h>

#include <exception>

#include <ibpp.h>

#include "config/Config.h"
#include "config/LocaleManager.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/db/DatabaseFactory.h"
#include "gui/FRStyleManager.h"
#include "gui/MainFrame.h"
#include "main.h"
#include "mcp/McpServer.h"

//----------------------------------------------------------------------
// CRT Debug Heap (MSVC Debug builds)
// Activated by building with -DENABLE_CRT_LEAK_CHECK=ON.
// _CRTDBG_MAP_ALLOC must be defined BEFORE any CRT header to redirect
// malloc/free to their debug variants, enabling source-location reporting.
// See: https://learn.microsoft.com/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library
//----------------------------------------------------------------------
#if defined(FR_CRT_LEAK_CHECK) && defined(_MSC_VER) && defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
    #include <cstdlib>
    #include <crtdbg.h>
    #define FR_CRT_LEAK_CHECK_ACTIVE 1
#endif

#ifdef FR_USE_CRASHPAD
#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#endif

IMPLEMENT_APP(Application)

#ifdef FR_USE_CRASHPAD
bool startCrashpad()
{
    wxFileName handlerPath(wxStandardPaths::Get().GetExecutablePath());
    handlerPath.SetFullName("crashpad_handler.exe");
    base::FilePath handler(handlerPath.GetFullPath().ToStdWstring());

    wxFileName dbPath(wxStandardPaths::Get().GetUserLocalDataDir(), "");
    dbPath.AppendDir("crashes");
    if (!wxDirExists(dbPath.GetPath()))
        wxMkdir(dbPath.GetPath());
    base::FilePath database(dbPath.GetPath().ToStdWstring());

    wxFileName metricsPath(wxStandardPaths::Get().GetUserLocalDataDir(), "");
    metricsPath.AppendDir("crashes_metrics");
    if (!wxDirExists(metricsPath.GetPath()))
        wxMkdir(metricsPath.GetPath());
    base::FilePath metrics(metricsPath.GetPath().ToStdWstring());

    std::string url = "";

    std::map<std::string, std::string> annotations;
    annotations["format"] = "minidump";
    annotations["prod"] = "FlameRobin";
    annotations["ver"] = "26.6.21";

    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit");

    crashpad::CrashpadClient client;
    bool status = client.StartHandler(
        handler,
        database,
        metrics,
        url,
        annotations,
        arguments,
        true, // restartable
        false // asynchronous_start
    );

    if (status)
    {
        std::unique_ptr<crashpad::CrashReportDatabase> db =
            crashpad::CrashReportDatabase::Initialize(database);
        if (db && db->GetSettings())
        {
            db->GetSettings()->SetUploadsEnabled(false);
        }
    }
    return status;
}
#endif

void parachute()
{
#ifdef FR_USE_CRASHPAD
    CONTEXT context;
    RtlCaptureContext(&context);
    crashpad::CrashpadClient::DumpWithoutCrash(context);
#endif

    wxString msg = ::wxGetTranslation(
        "A fatal error has occurred. If you know how to\n"
        "reproduce the problem, please submit the bug report at:\n"
        "https://github.com/mariuz/flamerobin/issues/new\n\n"
        "The program can try to keep running so that you\n"
        "can save your data. Do you wish to try?\n");

    fprintf(stderr, "\n*** FATAL ERROR ***\n%s\n********************\n",
        (const char*)msg.mb_str());

    if (wxYES == ::wxMessageBox(msg,
        _("Fatal error"), wxYES_NO | wxICON_ERROR))
    {
        int result = ::wxGetApp().OnRun();
        exit(result);
    }
    exit(1);
}

bool Application::OnExceptionInMainLoop()
{
    return true;
}

void Application::OnFatalException()
{
    parachute();
}

bool Application::OnInit()
{
#ifdef FR_USE_CRASHPAD
    startCrashpad();
#endif

#if wxUSE_ON_FATAL_EXCEPTION
    ::wxHandleFatalExceptions();
#endif

    std::set_terminate(parachute);

//----------------------------------------------------------------------
// CRT Debug Heap: arm the allocator debug flags.
//   _CRTDBG_ALLOC_MEM_DF  – turns on debug heap block tracking
//   _CRTDBG_LEAK_CHECK_DF – calls _CrtDumpMemoryLeaks() automatically
//                            when the CRT shuts down (belt-and-braces;
//                            we also call it explicitly in OnExit).
// Output appears in the Visual Studio "Output" window while debugging.
//----------------------------------------------------------------------
#ifdef FR_CRT_LEAK_CHECK_ACTIVE
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    // Route all CRT debug output to the VS Output window AND stderr
    _CrtSetReportMode(_CRT_WARN,  _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN,  _CRTDBG_FILE_STDERR);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    wxLogDebug("[FlameRobin] CRT Debug Heap activated - memory leak check ON");
#endif
    checkEnvironment();
    LocaleManager::get().initFromConfig();
    parseCommandLine();

    int backend = config().get("databaseBackend", static_cast<int>(fr::DatabaseBackend::FbCpp));
    if (backend == static_cast<int>(fr::DatabaseBackend::IBPP))
        fr::DatabaseFactory::setDefaultBackend(fr::DatabaseBackend::IBPP);
    else
        fr::DatabaseFactory::setDefaultBackend(fr::DatabaseBackend::FbCpp);

#if wxCHECK_VERSION(3, 3, 0)
    int theme = config().get(FRStyleManager::_DARKMODE_KEY, (int)FRStyleManager::ThemeSystem);
    if (theme == FRStyleManager::ThemeLight)
        SetAppearance(wxApp::Appearance::Light);
    else if (theme == FRStyleManager::ThemeDark)
        SetAppearance(wxApp::Appearance::Dark);
    else if (theme == FRStyleManager::ThemeSystem)
        SetAppearance(wxApp::Appearance::System);

#ifdef __WXMSW__
    MSWEnableDarkMode();
#endif
#endif

#if defined(__WXOSX_COCOA__)
    std::locale::global(std::locale(""));
#endif

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
        wxMessageBox(e.what(), _("Error initializing IBPP library."), wxOK | wxICON_ERROR);
        return false;
    }

    if (mcpModeM)
    {
        fr::McpServer::run();
        exit(0);
    }

    wxImage::AddHandler(new wxPNGHandler);

    wxSystemOptions::SetOption("mac.listctrl.always_use_generic", true);

    MainFrame* main_frame = new MainFrame(0, -1, "");
    SetTopWindow(main_frame);
    main_frame->Show();

    openDatabasesFromParams(main_frame);
    return true;
}

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
        wxMessageBox(e.what(), _("Unhandled Error in FlameRobin"),
            wxOK | wxICON_ERROR, wxGetTopLevelParent(wxGetActiveWindow()));
    }
}

int Application::OnExit()
{
//----------------------------------------------------------------------
// CRT Debug Heap: dump all still-reachable allocations to the Output
// window and stderr.  Each leaked block prints:
//   {filename}({line}): {bytes} bytes at 0x{address}
// (source-location info is only available when _CRTDBG_MAP_ALLOC was
//  defined before the first CRT header – which we do above).
// See: https://learn.microsoft.com/cpp/c-runtime-library/reference/crtdumpmemoryleaks
//----------------------------------------------------------------------
#ifdef FR_CRT_LEAK_CHECK_ACTIVE
    if (_CrtDumpMemoryLeaks())
        wxLogDebug("[FlameRobin] CRT leak check: memory leaks detected (see Output / stderr)");
    else
        wxLogDebug("[FlameRobin] CRT leak check: no memory leaks detected");
#endif
    return 0;
}

void Application::checkEnvironment()
{
    wxString envVar;
    if (wxGetEnv("FR_HOME", &envVar))
        config().setHomePath(translatePathMacros(envVar));
    if (wxGetEnv("FR_USER_HOME", &envVar))
        config().setUserHomePath(translatePathMacros(envVar));
}

void Application::parseCommandLine()
{
    wxCmdLineParser parser(wxGetApp().argc, wxGetApp().argv);
    parser.AddOption("h", "home", _("Set FlameRobin's home path"));
    parser.AddOption("uh", "user-home",
        _("Set FlameRobin's user home path"));
    parser.AddSwitch("m", "mcp", _("Run FlameRobin as an MCP server"));
    // open databases given as command line parameters
    parser.AddParam(_("File name of database to open"), wxCMD_LINE_VAL_STRING,
        wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);

    mcpModeM = false;
    if (parser.Parse() == 0)
    {
        wxString paramValue;
        if (parser.Found("home", &paramValue))
            config().setHomePath(translatePathMacros(paramValue));

        if (parser.Found("user-home", &paramValue))
            config().setUserHomePath(translatePathMacros(paramValue));

        mcpModeM = parser.Found("mcp");

        for (size_t i = 0; i < parser.GetParamCount(); i++)
            cmdlineParamsM.Add(parser.GetParam(i));
    }
}

const wxString Application::translatePathMacros(const wxString path) const
{
    if (path == "$app")
        return config().getLocalDataDir();
    else if (path == "$user")
        return config().getUserLocalDataDir();
    else
        return path;
}

const wxString Application::getConfigurableObjectId() const
{
    return "";
}

void Application::openDatabasesFromParams(MainFrame* frFrame)
{
    if (cmdlineParamsM.GetCount())
    {
        for (size_t i = 0; i < cmdlineParamsM.GetCount(); i++)
            frFrame->openUnregisteredDatabase(cmdlineParamsM[i]);
        cmdlineParamsM.Clear();
    }
}
