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

#ifdef wxUSE_ABOUTDLG
    #include <wx/aboutdlg.h>
#endif

#include <ibase.h>

#ifndef FB_API_VER
    #define FB_API_VER 0
#endif

#ifndef FBCPP_VERSION
    #define FBCPP_VERSION "unknown"
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
    #define FR_CPU_ARCH "x86-64"
#elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
    #define FR_CPU_ARCH "x86"
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
    #define FR_CPU_ARCH "arm64"
#elif defined(__arm__) || defined(_M_ARM)
    #define FR_CPU_ARCH "arm"
#elif defined(__powerpc__) || defined(__ppc__)
    #define FR_CPU_ARCH "ppc"
#elif defined(__mips__)
    #define FR_CPU_ARCH "mips"
#elif defined(__riscv)
    #define FR_CPU_ARCH "riscv"
#endif

#include "frversion.h"
#include "gui/AboutBox.h"
#include "gui/AboutBoxPlugins.h"
#include <firebird/Interface.h>

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
static wxString getFbClientPath()
{
    HMODULE hMod = ::GetModuleHandleW(L"fbclient.dll");
    if (!hMod)
        hMod = ::GetModuleHandleW(L"fbclient");
    if (hMod)
    {
        wchar_t path[32768];
        if (::GetModuleFileNameW(hMod, path, 32768))
        {
            return wxString(path);
        }
    }
    return wxEmptyString;
}
#else
#include <dlfcn.h>
static wxString getFbClientPath()
{
    Dl_info info;
    if (dladdr((void*)&Firebird::fb_get_master_interface, &info) && info.dli_fname)
    {
        return wxString::FromUTF8(info.dli_fname);
    }
    return wxEmptyString;
}
#endif


void showAboutBox(wxWindow* parent)
{
    wxString libs;
    libs.Printf(_("Firebird C++ API version %d.%d\n"
                  "fb-cpp library version %s\n"
                  "wxWidgets library version %d.%d.%d"),
        FB_API_VER / 10,
        FB_API_VER % 10,
        FBCPP_VERSION,
        wxMAJOR_VERSION,
        wxMINOR_VERSION,
        wxRELEASE_NUMBER
    );

    wxString clientPath = getFbClientPath();
    wxString configuredPlugins = getFirebirdConfiguredPlugins();
    wxString pluginDirectory = getFirebirdPluginDirectory();
    wxString pluginModules = getFirebirdPluginModules();

    if (!clientPath.empty())
    {
        libs += "\n" + wxString::Format(_("Firebird client loaded from: %s"), clientPath);
    }
    if (!configuredPlugins.empty())
    {
        libs += "\n" + wxString::Format(_("Configured plugins: %s"), configuredPlugins);
    }
    if (!pluginDirectory.empty())
    {
        libs += "\n" + wxString::Format(_("Plugin directory: %s"), pluginDirectory);
    }
    if (!pluginModules.empty())
    {
        libs += "\n" + wxString::Format(_("Installed plugin modules: %s"), pluginModules);
    }

    wxString ver;
#if defined FR_GIT_HASH
    wxString githash(FR_GIT_HASH);
    ver.Printf("%d.%d.%d (git hash %s)",
        FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_RLS, githash.c_str());
#else
    ver.Printf("%d.%d.%d",
        FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_RLS);
#endif
#if wxUSE_UNICODE
    ver += " Unicode";
#endif
#if defined(FR_CPU_ARCH)
    ver += " (";
    ver += FR_CPU_ARCH;
    ver += ")";
#endif

#if defined wxUSE_ABOUTDLG && (defined __WXMAC__ || defined __WXGTK__)

    wxUnusedVar(parent);

    wxAboutDialogInfo info;

    info.SetName("FlameRobin");

    info.SetCopyright(_("Copyright (c) 2004-2023 FlameRobin Development Team"));

    info.SetVersion(ver);

    wxString msg(_("Database Administration Tool for Firebird RDBMS"));
    msg += "\n\n";
    msg += libs;
    info.SetDescription(msg);

    // the following would prohibit the native dialog on Mac OS X
#if defined __WXGTK__
    info.SetWebSite("http://flamerobin.org");
#endif

    wxAboutBox(info);

#else

    wxString msg("FlameRobin " + ver);

#if defined(_WIN64) && !defined(FR_CPU_ARCH)
    msg += " (x64)";
#endif

    msg += "\n";
    msg += _("Database administration tool for Firebird RDBMS");
    msg += "\n\n";
    msg += libs;
    msg += "\n\n";
    msg += _("Copyright (c) 2004-2022  FlameRobin Development Team");
    msg += "\n";
    msg += _("http://www.flamerobin.org");

    wxMessageBox(msg, _("About FlameRobin"), wxOK | wxICON_INFORMATION, parent);

#endif
}

