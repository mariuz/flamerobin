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

#include <wx/webrequest.h>
#include <wx/tokenzr.h>

#include "frversion.h"
#include "gui/UpdateChecker.h"

void UpdateChecker::check(wxWindow* parent, bool quiet)
{
    if (!quiet)
        wxBeginBusyCursor();

    wxWebSession& session = wxWebSession::GetDefault();
    wxWebRequest request = session.CreateRequest(parent, "https://api.github.com/repos/mariuz/flamerobin/releases/latest");
    request.SetHeader("User-Agent", "FlameRobin");

    parent->Bind(wxEVT_WEBREQUEST_STATE, [parent, quiet](wxWebRequestEvent& evt) {
        if (evt.GetState() == wxWebRequest::State_Completed)
        {
            if (!quiet)
                wxEndBusyCursor();

            wxString content = evt.GetResponse().AsString();
            // Simple JSON parsing for "tag_name": "vX.Y.Z"
            int pos = content.Find("\"tag_name\":");
            if (pos != wxNOT_FOUND)
            {
                wxString tail = content.Mid(pos + 11);
                int startRel = tail.Find("\"");
                if (startRel != wxNOT_FOUND)
                {
                    int start = pos + 11 + startRel;
                    wxString afterQuote = content.Mid(start + 1);
                    int endRel = afterQuote.Find("\"");
                    if (endRel != wxNOT_FOUND)
                    {
                        int end = start + 1 + endRel;
                        wxString tag = content.SubString(start + 1, end - 1);
                        if (tag.StartsWith("v"))
                            tag = tag.Mid(1);

                        wxStringTokenizer tkz(tag, ".");
                        long major = 0, minor = 0, rls = 0;
                        if (tkz.HasMoreTokens() && tkz.GetNextToken().ToLong(&major) &&
                            tkz.HasMoreTokens() && tkz.GetNextToken().ToLong(&minor) &&
                            tkz.HasMoreTokens() && tkz.GetNextToken().ToLong(&rls))
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
                                    wxString::Format(_("A new version of FlameRobin is available: %ld.%ld.%ld\nYour current version is: %d.%d.%d\n\nWould you like to visit the release page?"),
                                        major, minor, rls, FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_RLS),
                                    _("Update Available"),
                                    wxYES_NO | wxICON_INFORMATION,
                                    parent
                                );
                                if (res == wxYES)
                                {
                                    wxLaunchDefaultBrowser("https://github.com/mariuz/flamerobin/releases/latest");
                                }
                            }
                            else if (!quiet)
                            {
                                wxMessageBox(_("Your version of FlameRobin is up to date."), _("No Updates"), wxOK | wxICON_INFORMATION, parent);
                            }
                            return;
                        }
                    }
                }
            }
            if (!quiet)
                wxMessageBox(_("Could not parse version information from GitHub."), _("Error"), wxOK | wxICON_ERROR, parent);
        }
        else if (evt.GetState() == wxWebRequest::State_Failed)
        {
            if (!quiet)
            {
                wxEndBusyCursor();
                wxMessageBox(wxString::Format(_("Could not retrieve version information from GitHub:\n%s"), evt.GetErrorDescription()), _("Error"), wxOK | wxICON_ERROR, parent);
            }
        }
    });

    request.Start();
}
