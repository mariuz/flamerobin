/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "core/StringUtils.h"
#include "frutils.h"
#include "metadata/metadataitem.h"
#include "HtmlTemplateProcessor.h"

//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
HtmlTemplateProcessor::HtmlTemplateProcessor(MetadataItem *m, wxWindow *window)
    : TemplateProcessor(m, window)
{
}
//-----------------------------------------------------------------------------
void HtmlTemplateProcessor::processCommand(wxString cmdName, TemplateCmdParams cmdParams,
	MetadataItem *object, wxString& processedText)
{
	TemplateProcessor::processCommand(cmdName, cmdParams, object, processedText);

	if (cmdName == wxT("header") && !cmdParams.empty())  // include another file
    {
        vector<wxString> pages;            // pages this object has
        pages.push_back(wxT("Summary"));
        if (object->getType() == ntRole)        // special case, roles
            pages.push_back(wxT("Privileges")); // don't have dependencies
        if (object->getType() == ntDatabase)
            pages.push_back(wxT("Triggers"));   // FB 2.1
        switch (object->getType())
        {
            case ntSysTable:
            case ntTable:
                pages.push_back(wxT("Constraints"));
                pages.push_back(wxT("Indices"));
            case ntView:
                pages.push_back(wxT("Triggers"));
            case ntProcedure:
                pages.push_back(wxT("Privileges"));
            case ntTrigger:
            case ntException:
            case ntFunction:
            case ntGenerator:
                pages.push_back(wxT("Dependencies"));
            case ntDatabase:
            case ntRole:
                pages.push_back(wxT("DDL"));
        };
        wxString page = loadEntireFile(getTemplatePath() + wxT("header.html"));
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
            for (vector<wxString>::iterator it = pages.begin(); it !=
                pages.end(); ++it)
            {
                if (part.Find(wxT(">")+(*it)+wxT("<")) == -1)
                    continue;
                if (first)
                    first = false;
                else
                    processedText += wxT(" | ");
                wxString allParams = cmdParams.all();
                if (part.Find(wxT(">") + allParams + wxT("<")) != -1)
                    processedText += allParams;
                else
                    internalProcessTemplateText(processedText, part, object);
            }
		}
	}
}
//-----------------------------------------------------------------------------
wxString HtmlTemplateProcessor::escapeChars(const wxString& input, bool processNewlines)
{
	return escapeHtmlChars(input, processNewlines);
}
//-----------------------------------------------------------------------------
