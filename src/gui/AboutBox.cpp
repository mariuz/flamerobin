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
#include <ibpp.h>

#ifndef FB_API_VER
    #define FB_API_VER 0
#endif

#ifndef FBCPP_VERSION
    #define FBCPP_VERSION "unknown"
#endif

#include "frversion.h"
#include "gui/AboutBox.h"
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

extern "C" bool fbcpp_is_client_initialized();

#if defined(_WIN32) || defined(WIN32)
#include <excpt.h>
#include <stdio.h>
#include <string.h>

static bool queryPluginsSEH(char* buffer, size_t bufferSize)
{
    bool success = false;
    __try
    {
        Firebird::IMaster* master = Firebird::fb_get_master_interface();
        if (master)
        {
            Firebird::IStatus* status = master->getStatus();
            if (status)
            {
                Firebird::IPluginManager* pm = master->getPluginManager();
                if (pm && pm->cloopVTable)
                {
                    auto* pmVtable = static_cast<Firebird::IPluginManager::VTable*>(pm->cloopVTable);

                    Firebird::IConfigManager* cm = master->getConfigManager();
                    Firebird::IFirebirdConf* conf = nullptr;
                    if (cm && cm->cloopVTable)
                    {
                        auto* cmVtable = static_cast<Firebird::IConfigManager::VTable*>(cm->cloopVTable);
                        conf = cmVtable->getFirebirdConf(cm);
                    }

                    unsigned types[] = {
                        Firebird::IPluginManager::TYPE_PROVIDER,
                        Firebird::IPluginManager::TYPE_AUTH_SERVER,
                        Firebird::IPluginManager::TYPE_AUTH_CLIENT,
                        Firebird::IPluginManager::TYPE_AUTH_USER_MANAGEMENT,
                        Firebird::IPluginManager::TYPE_EXTERNAL_ENGINE,
                        Firebird::IPluginManager::TYPE_TRACE,
                        Firebird::IPluginManager::TYPE_WIRE_CRYPT,
                        Firebird::IPluginManager::TYPE_DB_CRYPT,
                        Firebird::IPluginManager::TYPE_KEY_HOLDER,
                        Firebird::IPluginManager::TYPE_REPLICATOR
                    };

                    size_t offset = 0;
                    buffer[0] = '\0';

                    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); ++i)
                    {
                        unsigned type = types[i];
                        Firebird::IPluginSet* pluginSet = pmVtable->getPlugins(pm, status, type, nullptr, conf);
                        if (pluginSet)
                        {
                            if (pluginSet->cloopVTable)
                            {
                                auto* psVtable = static_cast<Firebird::IPluginSet::VTable*>(pluginSet->cloopVTable);
                                while (true)
                                {
                                    const char* name = psVtable->getName(pluginSet);
                                    if (!name)
                                        break;
                                    const char* moduleName = psVtable->getModuleName(pluginSet);

                                    char temp[512];
                                    if (moduleName && *moduleName)
                                    {
                                        _snprintf(temp, sizeof(temp), "%s (%s)", name, moduleName);
                                    }
                                    else
                                    {
                                        _snprintf(temp, sizeof(temp), "%s", name);
                                    }
                                    temp[sizeof(temp) - 1] = '\0';

                                    size_t tempLen = strlen(temp);
                                    if (offset + tempLen + 3 < bufferSize)
                                    {
                                        if (offset > 0)
                                        {
                                            strcat(buffer, ", ");
                                            offset += 2;
                                        }
                                        strcat(buffer, temp);
                                        offset += tempLen;
                                    }
                                    psVtable->next(pluginSet, status);
                                }
                            }
                            pluginSet->release();
                        }
                    }
                    if (conf)
                        conf->release();
                    success = true;
                }
                status->dispose();
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        success = false;
    }
    return success;
}
#endif

static wxString getLoadedPlugins()
{
    if (!fbcpp_is_client_initialized())
        return wxEmptyString;

    wxString pluginsList;
#if defined(_WIN32) || defined(WIN32)
    char buffer[4096];
    if (queryPluginsSEH(buffer, sizeof(buffer)))
    {
        pluginsList = wxString::FromUTF8(buffer);
    }
#else
    try
    {
        Firebird::IMaster* master = Firebird::fb_get_master_interface();
        if (master)
        {
            Firebird::IStatus* status = master->getStatus();
            if (status)
            {
                Firebird::IPluginManager* pm = master->getPluginManager();
                if (pm && pm->cloopVTable)
                {
                    auto* pmVtable = static_cast<Firebird::IPluginManager::VTable*>(pm->cloopVTable);

                    Firebird::IConfigManager* cm = master->getConfigManager();
                    Firebird::IFirebirdConf* conf = nullptr;
                    if (cm && cm->cloopVTable)
                    {
                        auto* cmVtable = static_cast<Firebird::IConfigManager::VTable*>(cm->cloopVTable);
                        conf = cmVtable->getFirebirdConf(cm);
                    }

                    unsigned types[] = {
                        Firebird::IPluginManager::TYPE_PROVIDER,
                        Firebird::IPluginManager::TYPE_AUTH_SERVER,
                        Firebird::IPluginManager::TYPE_AUTH_CLIENT,
                        Firebird::IPluginManager::TYPE_AUTH_USER_MANAGEMENT,
                        Firebird::IPluginManager::TYPE_EXTERNAL_ENGINE,
                        Firebird::IPluginManager::TYPE_TRACE,
                        Firebird::IPluginManager::TYPE_WIRE_CRYPT,
                        Firebird::IPluginManager::TYPE_DB_CRYPT,
                        Firebird::IPluginManager::TYPE_KEY_HOLDER,
                        Firebird::IPluginManager::TYPE_REPLICATOR
                    };

                    for (unsigned type : types)
                    {
                        Firebird::IPluginSet* pluginSet = pmVtable->getPlugins(pm, status, type, nullptr, conf);
                        if (pluginSet)
                        {
                            if (pluginSet->cloopVTable)
                            {
                                auto* psVtable = static_cast<Firebird::IPluginSet::VTable*>(pluginSet->cloopVTable);
                                while (true)
                                {
                                    const char* name = psVtable->getName(pluginSet);
                                    if (!name)
                                        break;
                                    const char* moduleName = psVtable->getModuleName(pluginSet);
                                    if (!pluginsList.empty())
                                        pluginsList += ", ";
                                    pluginsList += wxString::FromUTF8(name);
                                    if (moduleName && *moduleName)
                                    {
                                        pluginsList += " (" + wxString::FromUTF8(moduleName) + ")";
                                    }
                                    psVtable->next(pluginSet, status);
                                }
                            }
                            pluginSet->release();
                        }
                    }
                    if (conf)
                        conf->release();
                }
                status->dispose();
            }
        }
    }
    catch (...)
    {
    }
#endif
    return pluginsList;
}

void showAboutBox(wxWindow* parent)
{
    wxString libs;
    libs.Printf(_("This tool uses IBPP library version %d.%d.%d.%d\n"
                  "Firebird C++ API version %d.%d\n"
                  "fb-cpp library version %s\n"
                  "wxWidgets library version %d.%d.%d"),
        (IBPP::Version & 0xFF000000) >> 24,
        (IBPP::Version & 0x00FF0000) >> 16,
        (IBPP::Version & 0x0000FF00) >> 8,
        (IBPP::Version & 0x000000FF),
        FB_API_VER / 10,
        FB_API_VER % 10,
        FBCPP_VERSION,
        wxMAJOR_VERSION,
        wxMINOR_VERSION,
        wxRELEASE_NUMBER
    );

    wxString clientPath = getFbClientPath();
    wxString loadedPlugins = getLoadedPlugins();

    if (!clientPath.empty())
    {
        libs += "\n" + wxString::Format(_("Firebird client loaded from: %s"), clientPath);
    }
    if (!loadedPlugins.empty())
    {
        libs += "\n" + wxString::Format(_("Loaded plugins: %s"), loadedPlugins);
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

#if defined(_WIN64)
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

