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

#include "core/TemplateProcessor.h"
#include "metadata/relation.h"

//-----------------------------------------------------------------------------
class MetadataTemplateCmdHandler: public TemplateCmdHandler
{
private:
    static const MetadataTemplateCmdHandler handlerInstance; // singleton; registers itself on creation.
public:
    virtual void handleTemplateCmd(TemplateProcessor *tp, wxString cmdName,
        TemplateCmdParams cmdParams, MetadataItem* object, wxString& processedText);
};
//-----------------------------------------------------------------------------
const MetadataTemplateCmdHandler MetadataTemplateCmdHandler::handlerInstance;
//-----------------------------------------------------------------------------
void MetadataTemplateCmdHandler::handleTemplateCmd(TemplateProcessor *tp,
    wxString cmdName, TemplateCmdParams cmdParams, MetadataItem* object,
    wxString& processedText)
{
    // {%foreach:<collection>:<separator>:<text>%}
    // repeats <text> once for each item in <collection>, pasting a <separator>
    // before each item except the first.
    if ((cmdName == wxT("foreach")) && (cmdParams.Count() >= 3))
    {
        wxString sep;
        tp->internalProcessTemplateText(sep, cmdParams[1], object);
        if (cmdParams[0] == wxT("column"))
        {
            Relation* r = dynamic_cast<Relation*>(object);
            if (!r)
                return;
            r->ensureChildrenLoaded();
            bool firstItem = true;
            for (RelationColumns::iterator it = r->begin(); it != r->end(); ++it)
            {
                wxString newText;
                tp->internalProcessTemplateText(newText, cmdParams[2], (*it).get());
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }
        // add more collections here.
        else
            return;
    }
}
//-----------------------------------------------------------------------------
