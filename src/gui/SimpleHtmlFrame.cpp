/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena, Michael Hieke
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

#include "config/Config.h"
#include "images.h"
#include "PrintableHtmlWindow.h"
#include "SimpleHtmlFrame.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
bool showHtmlFile(wxWindow* parent, const wxFileName& fileName)
{
    if (!fileName.FileExists())
    {
        wxString msg;
        msg.Printf(_("The HTML document \"%s\" does not exist!"), 
            fileName.GetFullPath().c_str());
        wxMessageBox(msg, _("FlameRobin"), wxOK | wxICON_ERROR);
        return false;
    }

    SimpleHtmlFrame* shf = SimpleHtmlFrame::findFrameFor(fileName);
    if (shf)
    {
        shf->Raise();
        return true;
    }
    shf = new SimpleHtmlFrame(parent, fileName.GetFullPath());
    shf->Show();
    return true;
}
//-----------------------------------------------------------------------------
SimpleHtmlFrame::SimpleHtmlFrame(wxWindow* parent, const wxFileName& fileName)
    : BaseFrame(parent, -1, wxEmptyString)
{
    html_window = new PrintableHtmlWindow(this);
    CreateStatusBar();
    html_window->SetRelatedFrame(this, wxT("%s"));
    html_window->SetRelatedStatusBar(0);

    html_window->LoadPage(fileName.GetFullPath());
    setIdString(this, getFrameId(fileName));

#include "fricon.xpm"
    wxBitmap bmp(fricon_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------
const wxRect SimpleHtmlFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 600, 420);
}
//-----------------------------------------------------------------------------
const wxString SimpleHtmlFrame::getName() const
{
    return wxT("SimpleHtmlFrameFrame");
}
//-----------------------------------------------------------------------------
wxString SimpleHtmlFrame::getFrameId(const wxFileName& fileName)
{
    if (fileName.HasName())
        return wxString(wxT("SimpleHtmlFrame/") + fileName.GetFullPath());
    else
        return wxEmptyString;
}
//-----------------------------------------------------------------------------
SimpleHtmlFrame* SimpleHtmlFrame::findFrameFor(const wxFileName& fileName)
{
    BaseFrame* bf = frameFromIdString(getFrameId(fileName));
    if (!bf)
        return 0;
    return dynamic_cast<SimpleHtmlFrame*>(bf);
}
//-----------------------------------------------------------------------------
