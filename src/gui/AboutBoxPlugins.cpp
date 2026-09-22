/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>

#include "gui/AboutBoxPlugins.h"

#include <firebird/Interface.h>

// The plugin types with a configuration key of their own, and that key.  The
// other types - external engines, database encryption, replication - are named
// at the point of use and have no global list.
struct PluginTypeConfig
{
    unsigned type;
    const char* configKey;
};

static const PluginTypeConfig pluginTypeConfigs[] = {
    { Firebird::IPluginManager::TYPE_PROVIDER,             "Providers"       },
    { Firebird::IPluginManager::TYPE_AUTH_CLIENT,          "AuthClient"      },
    { Firebird::IPluginManager::TYPE_AUTH_SERVER,          "AuthServer"      },
    { Firebird::IPluginManager::TYPE_AUTH_USER_MANAGEMENT, "UserManager"     },
    { Firebird::IPluginManager::TYPE_TRACE,                "TracePlugin"     },
    { Firebird::IPluginManager::TYPE_WIRE_CRYPT,           "WireCryptPlugin" },
    { Firebird::IPluginManager::TYPE_KEY_HOLDER,           "KeyHolderPlugin" }
};

namespace
{

Firebird::IConfigManager* getConfigManager()
{
    Firebird::IMaster* master = Firebird::fb_get_master_interface();
    if (!master)
        return nullptr;

    Firebird::IConfigManager* cm = master->getConfigManager();
    return (cm && cm->cloopVTable) ? cm : nullptr;
}

void appendUnique(wxArrayString& names, const wxString& name)
{
    if (!name.empty() && names.Index(name) == wxNOT_FOUND)
        names.Add(name);
}

} // namespace

wxString getFirebirdConfiguredPlugins()
{
    wxArrayString names;
    try
    {
        Firebird::IConfigManager* cm = getConfigManager();
        if (!cm)
            return wxEmptyString;

        auto* cmVtable = static_cast<Firebird::IConfigManager::VTable*>(cm->cloopVTable);
        Firebird::IFirebirdConf* conf = cmVtable->getFirebirdConf(cm);
        if (conf && conf->cloopVTable)
        {
            auto* confVtable = static_cast<Firebird::IFirebirdConf::VTable*>(conf->cloopVTable);
            for (const PluginTypeConfig& ptc : pluginTypeConfigs)
            {
                const char* list = confVtable->asString(conf,
                    confVtable->getKey(conf, ptc.configKey));
                if (!list || !*list)
                    continue;

                wxStringTokenizer tokens(wxString::FromUTF8(list), ",;");
                while (tokens.HasMoreTokens())
                    appendUnique(names, tokens.GetNextToken().Trim(true).Trim(false));
            }
            conf->release();
        }
    }
    catch (...)
    {
    }

    wxString result;
    for (const wxString& name : names)
    {
        if (!result.empty())
            result += ", ";
        result += name;
    }
    return result;
}

wxString getFirebirdPluginDirectory()
{
    try
    {
        Firebird::IConfigManager* cm = getConfigManager();
        if (!cm)
            return wxEmptyString;

        auto* cmVtable = static_cast<Firebird::IConfigManager::VTable*>(cm->cloopVTable);
        const char* dir = cmVtable->getDirectory(cm,
            Firebird::IConfigManager::DIR_PLUGINS);
        if (dir && *dir)
            return wxString::FromUTF8(dir);
    }
    catch (...)
    {
    }
    return wxEmptyString;
}

wxString getFirebirdPluginModules()
{
    const wxString directory = getFirebirdPluginDirectory();
    if (directory.empty() || !wxDir::Exists(directory))
        return wxEmptyString;

#if defined(_WIN32) || defined(WIN32)
    const wxString filter = "*.dll";
#elif defined(__WXMAC__)
    const wxString filter = "*.dylib";
#else
    const wxString filter = "*.so";
#endif

    wxArrayString modules;
    wxString filename;
    wxDir dir(directory);
    bool found = dir.IsOpened() && dir.GetFirst(&filename, filter, wxDIR_FILES);
    while (found)
    {
        // libEngine13.so -> Engine13, ChaCha.dll -> ChaCha
        wxString name = wxFileName(filename).GetName();
#if !defined(_WIN32) && !defined(WIN32)
        if (name.StartsWith("lib"))
            name = name.Mid(3);
#endif
        appendUnique(modules, name);
        found = dir.GetNext(&filename);
    }

    modules.Sort();

    wxString result;
    for (const wxString& module : modules)
    {
        if (!result.empty())
            result += ", ";
        result += module;
    }
    return result;
}
