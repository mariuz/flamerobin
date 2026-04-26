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

#include <wx/settings.h>

#include "core/StringUtils.h"
#include "frutils.h"
#include "gui/HtmlHeaderMetadataItemVisitor.h"
#include "metadata/metadataitem.h"
#include "HtmlTemplateProcessor.h"


class ProcessableObject;

HtmlTemplateProcessor::HtmlTemplateProcessor(ProcessableObject* object,
    wxWindow* window)
    : TemplateProcessor(object, window)
{
}

void HtmlTemplateProcessor::processCommand(const wxString& cmdName,
    const TemplateCmdParams& cmdParams, ProcessableObject* object,
    wxString& processedText)
{
    TemplateProcessor::processCommand(cmdName, cmdParams, object,
        processedText);

    MetadataItem* metadataItem = dynamic_cast<MetadataItem*>(object);
    if (!metadataItem)
        return;

    if (cmdName == "header" && !cmdParams.empty())  // include another file
    {
        std::vector<wxString> pages;
        HtmlHeaderMetadataItemVisitor v(pages);
        metadataItem->acceptVisitor(&v);

        wxString page = loadEntireFile(getTemplatePath() + "header.html");
        bool first = true;
        while (!page.Strip().IsEmpty())
        {
            wxString::size_type p2 = page.find('|');
            wxString part(page);
            if (p2 != wxString::npos)
            {
                part = page.substr(0, p2);
                page.Remove(0, p2+1);
            }
            else
                page.Clear();
            for (std::vector<wxString>::iterator it = pages.begin(); it !=
                pages.end(); ++it)
            {
                if (part.Find(">"+(*it)+"<") == -1)
                    continue;
                if (first)
                    first = false;
                else
                    processedText += " | ";
                wxString allParams = cmdParams.all();
                if (part.Find(">" + allParams + "<") != -1)
                    processedText += allParams;
                else
                    internalProcessTemplateText(processedText, part, object);
            }
        }
    }
}

wxString HtmlTemplateProcessor::escapeChars(const wxString& input, bool processNewlines)
{
    return escapeHtmlChars(input, processNewlines);
}

/*static*/
void HtmlTemplateProcessor::applyDarkModeIfNeeded(wxString& html)
{
    // wxSystemAppearance::IsDark is the right check on every platform that
    // exposes a dark mode (macOS Mojave+, Win10+, GTK with dark theme).
    if (!wxSystemSettings::GetAppearance().IsDark())
        return;

    // Replace the legacy hard-coded bgcolor= attributes from the metadata
    // templates with darker equivalents. We only touch the literal
    // bgcolor="…" forms the templates emit, so user-provided values in
    // descriptions or other content are unaffected.
    struct ColorMap { const wxChar* from; const wxChar* to; };
    static const ColorMap mapping[] = {
        { wxT("bgcolor=\"white\""),   wxT("bgcolor=\"#1e1e1e\"") }, // outer wrapper
        { wxT("bgcolor=\"black\""),   wxT("bgcolor=\"#3a3a3a\"") }, // table border
        { wxT("bgcolor=\"silver\""),  wxT("bgcolor=\"#3a3a3a\"") }, // = #C0C0C0
        { wxT("bgcolor=\"#999999\""), wxT("bgcolor=\"#3a3a3a\"") }, // dark legend border
        { wxT("bgcolor=\"#CCCCCC\""), wxT("bgcolor=\"#2c2c2c\"") }, // legend row
        { wxT("bgcolor=\"#DDDDDD\""), wxT("bgcolor=\"#34343c\"") }, // legend alt row
        { wxT("bgcolor=\"#DDDDFF\""), wxT("bgcolor=\"#2c2c40\"") }, // metadata odd row
        { wxT("bgcolor=\"#CCCCFF\""), wxT("bgcolor=\"#23233a\"") }, // metadata value cell
    };
    for (const ColorMap& m : mapping)
        html.Replace(m.from, m.to);

    // Some templates emit <font color="black"> for cells that sit on the
    // grey backgrounds — that becomes black-on-dark after we recolor the
    // backgrounds. Lift those explicit blacks to a light shade.
    html.Replace(wxT("<font color=\"black\">"), wxT("<font color=\"#e0e0e0\">"));
    html.Replace(wxT("<font color=black>"),     wxT("<font color=\"#e0e0e0\">"));

    // Default text color in legacy wxHtmlWindow is black, which is
    // unreadable on the dark backgrounds we just inserted. Inject a body
    // text color via the first <body...> tag so all unstyled text inherits
    // a light foreground. font color="white" cells (header rows) keep
    // their explicit color since attribute beats inheritance.
    //
    // HTML attribute names are case-insensitive, and external templates
    // (the user manual, license viewer) may use mixed casing. wx 3.3's
    // wxString::Find(const wxString&) doesn't expose a case-insensitive
    // flag, so search a bounded lower-cased prefix instead — <body> is
    // virtually always within the first few KB of any HTML document and
    // copying 8 KB is cheap even for multi-MB inputs.
    const size_t kBodySearchPrefix = 8 * 1024;
    wxString lowerPrefix = html.Mid(0, kBodySearchPrefix).Lower();
    wxString::size_type bodyStart = lowerPrefix.find(wxT("<body"));
    if (bodyStart != wxString::npos)
    {
        wxString::size_type bodyEnd = html.find('>', bodyStart);
        if (bodyEnd != wxString::npos)
        {
            // Look for an existing text= attribute, but require a
            // leading separator (' ', '\t' or '\n') so we don't false-
            // positive on attribute values that contain "text=" as a
            // substring (e.g. <body class="main-text-area">).
            wxString bodyTag = html.Mid(bodyStart, bodyEnd - bodyStart + 1);
            wxString lowerTag = bodyTag.Lower();
            if (lowerTag.Find(wxT(" text=")) == wxNOT_FOUND
                && lowerTag.Find(wxT("\ttext=")) == wxNOT_FOUND
                && lowerTag.Find(wxT("\ntext=")) == wxNOT_FOUND)
            {
                html.insert(bodyEnd, wxT(" text=\"#e0e0e0\""));
            }
        }
    }
}

