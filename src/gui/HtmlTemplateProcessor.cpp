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

