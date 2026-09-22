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

#include "gui/AboutBoxPlugins.h"

#include <firebird/Interface.h>

// Firebird's plugin manager takes the list of plugin names to enumerate as a
// plain string and unconditionally does the equivalent of strlen() on it, so
// passing a null pointer for "use the configured default" crashes inside
// fbclient.  The configured list for each plugin type therefore has to be read
// from firebird.conf first, exactly like Firebird itself does internally.
//
// Only the types that own a configuration key can be listed this way; external
// engines, database encryption and replication plugins are named at the point
// of use (in DDL or per-database settings) and have no global list.
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
                Firebird::IConfigManager* cm = master->getConfigManager();
                if (pm && pm->cloopVTable && cm && cm->cloopVTable)
                {
                    auto* pmVtable = static_cast<Firebird::IPluginManager::VTable*>(pm->cloopVTable);
                    auto* cmVtable = static_cast<Firebird::IConfigManager::VTable*>(cm->cloopVTable);

                    Firebird::IFirebirdConf* conf = cmVtable->getFirebirdConf(cm);
                    if (conf && conf->cloopVTable)
                    {
                        auto* confVtable = static_cast<Firebird::IFirebirdConf::VTable*>(conf->cloopVTable);

                        size_t offset = 0;
                        buffer[0] = '\0';

                        for (size_t i = 0; i < sizeof(pluginTypeConfigs)/sizeof(pluginTypeConfigs[0]); ++i)
                        {
                            const PluginTypeConfig& ptc = pluginTypeConfigs[i];
                            const char* names = confVtable->asString(conf,
                                confVtable->getKey(conf, ptc.configKey));
                            if (!names || !*names)
                                continue;

                            Firebird::IPluginSet* pluginSet = pmVtable->getPlugins(pm, status,
                                ptc.type, names, conf);
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
                        success = true;
                    }
                    if (conf)
                        conf->release();
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

wxString getFirebirdLoadedPlugins()
{
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
                Firebird::IConfigManager* cm = master->getConfigManager();
                if (pm && pm->cloopVTable && cm && cm->cloopVTable)
                {
                    auto* pmVtable = static_cast<Firebird::IPluginManager::VTable*>(pm->cloopVTable);
                    auto* cmVtable = static_cast<Firebird::IConfigManager::VTable*>(cm->cloopVTable);

                    Firebird::IFirebirdConf* conf = cmVtable->getFirebirdConf(cm);
                    if (conf && conf->cloopVTable)
                    {
                        auto* confVtable = static_cast<Firebird::IFirebirdConf::VTable*>(conf->cloopVTable);

                        for (const PluginTypeConfig& ptc : pluginTypeConfigs)
                        {
                            const char* names = confVtable->asString(conf,
                                confVtable->getKey(conf, ptc.configKey));
                            if (!names || !*names)
                                continue;

                            Firebird::IPluginSet* pluginSet = pmVtable->getPlugins(pm, status,
                                ptc.type, names, conf);
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

