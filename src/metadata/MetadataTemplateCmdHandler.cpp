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
#include "core/TemplateProcessor.h"
#include "metadata/privilege.h"
#include "metadata/procedure.h"
#include "metadata/relation.h"
#include "metadata/role.h"

//-----------------------------------------------------------------------------
using namespace std;
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

        // {%foreach:privilegeitem:<separator>:<type>:<text>%}
        else if ((cmdParams[0] == wxT("privilegeitem")) && (cmdParams.Count() >= 4))
        {
            Privilege* p = dynamic_cast<Privilege*>(object);
            if (!p)
                return;
            PrivilegeItems list;
            p->getPrivilegeItems(cmdParams[2], list);
            bool firstItem = true;
            for (PrivilegeItems::iterator it = list.begin(); it != list.end(); ++it)
            {
                wxString newText;
                tp->internalProcessTemplateText(newText, cmdParams[3], &(*it));
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }

        // {%foreach:privilege:<separator>:<text>%}
        else if (cmdParams[0] == wxT("privilege"))
        {
            Relation* rel = dynamic_cast<Relation*>(object);
            Procedure* proc = dynamic_cast<Procedure*>(object);
            Role* role = dynamic_cast<Role*>(object);
            std::vector<Privilege>* p = 0;
            if (rel)
                p = rel->getPrivileges();
            if (proc)
                p = proc->getPrivileges();
            if (role)
                p = role->getPrivileges();
            if (!p)
                return;
            for (std::vector<Privilege>::iterator it = p->begin(); it != p->end(); ++it)
                tp->internalProcessTemplateText(processedText, cmdParams.all(2), &(*it));
        }

        // add more collections here.
        else
            return;
    }

    else if (cmdName == wxT("privilegeinfo") && (cmdParams.Count() > 0)) 
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
            
        if (cmdParams[0] == wxT("grantee_name"))
            processedText += tp->escapeChars(p->getGrantee());
    }

    // {%privilegeitemcount:type%}
    else if ((cmdName == wxT("privilegeitemcount")) && (cmdParams.Count() >= 1))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
        PrivilegeItems list;
        p->getPrivilegeItems(cmdParams[0], list);
        processedText << list.size();
    }
    
    // {%privilegeiteminfo:property%}
    else if ((cmdName == wxT("privilegeiteminfo")) && (cmdParams.Count() >= 1))
    {
        PrivilegeItem* pi = dynamic_cast<PrivilegeItem*>(object);
        if (!pi)
            return;
        
        if (cmdParams[0] == wxT("grantor"))
            processedText += pi->grantor;
        else if (cmdParams[0] == wxT("grant_option"))
            processedText += getBooleanAsString(pi->grantOption);
        else if (cmdParams[0] == wxT("columns"))
        {
            for (vector<wxString>::iterator it = pi->columns.begin();
                it != pi->columns.end(); ++it)
            {
                if (it != pi->columns.begin())
                    processedText += wxT(",");
                processedText += (*it);
            }
        }
    }    
}
//-----------------------------------------------------------------------------
