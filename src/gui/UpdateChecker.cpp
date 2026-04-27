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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include <wx/txtstrm.h>

#include "frversion.h"
#include "gui/UpdateChecker.h"

void UpdateChecker::check(wxWindow* parent, bool quiet)
{
    wxHTTP http;
    http.SetHeader("User-Agent", "FlameRobin");
    http.SetTimeout(10); // 10 seconds

    // We should probably use a stable URL for version info
    // For now, let's use a placeholder that points to flamerobin.org
    // In a real scenario, this could be a GitHub Gist or a file on the website
    wxString versionUrl = "http://www.flamerobin.org/version.txt";
    
    if (!quiet)
        wxBeginBusyCursor();

    if (http.Connect("www.flamerobin.org"))
    {
        wxInputStream* stream = http.GetInputStream("/version.txt");
        if (http.GetResponse() == 200 && stream)
        {
            wxString content;
            wxStringOutputStream out_stream(&content);
            stream->Read(out_stream);
            delete stream;

            if (!quiet)
                wxEndBusyCursor();

            // Expected format: MAJOR.MINOR.RLS
            // e.g. 0.9.3
            long major = 0, minor = 0, rls = 0;
            wxString versionStr = content.BeforeFirst('\n').Trim();
            
            bool parsed = false;
            wxString rest;
            if (versionStr.BeforeFirst('.').ToLong(&major))
            {
                rest = versionStr.AfterFirst('.');
                if (rest.BeforeFirst('.').ToLong(&minor))
                {
                    rest = rest.AfterFirst('.');
                    if (rest.ToLong(&rls))
                    {
                        parsed = true;
                    }
                }
            }

            if (parsed)
            {
                bool newVersion = false;
                if (major > FR_VERSION_MAJOR)
                    newVersion = true;
                else if (major == FR_VERSION_MAJOR && minor > FR_VERSION_MINOR)
                    newVersion = true;
                else if (major == FR_VERSION_MAJOR && minor == FR_VERSION_MINOR && rls > FR_VERSION_RLS)
                    newVersion = true;

                if (newVersion)
                {
                    int res = wxMessageBox(
                        wxString::Format(_("A new version of FlameRobin is available: %ld.%ld.%ld\nYour current version is: %d.%d.%d\n\nWould you like to visit the download page?"),
                            major, minor, rls, FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_RLS),
                        _("Update Available"),
                        wxYES_NO | wxICON_INFORMATION,
                        parent
                    );
                    if (res == wxYES)
                    {
                        wxLaunchDefaultBrowser("http://www.flamerobin.org/downloads.html");
                    }
                }
                else if (!quiet)
                {
                    wxMessageBox(_("Your version of FlameRobin is up to date."), _("No Updates"), wxOK | wxICON_INFORMATION, parent);
                }
            }
            else if (!quiet)
            {
                wxMessageBox(_("Could not parse version information from server."), _("Error"), wxOK | wxICON_ERROR, parent);
            }
        }
        else
        {
            if (!quiet)
            {
                wxEndBusyCursor();
                wxMessageBox(_("Could not retrieve version information from server."), _("Error"), wxOK | wxICON_ERROR, parent);
            }
            delete stream;
        }
    }
    else
    {
        if (!quiet)
        {
            wxEndBusyCursor();
            wxMessageBox(_("Could not connect to update server."), _("Error"), wxOK | wxICON_ERROR, parent);
        }
    }
}
