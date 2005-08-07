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

  Contributor(s):
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

#include <wx/clipbrd.h>
#include <wx/file.h>
#include <wx/filedlg.h>

#include "images.h"
#include "ugly.h"
#include "urihandler.h"

#include "PrintableHtmlWindow.h"
//-----------------------------------------------------------------------------
HtmlPrinter::HtmlPrinter()
{
	prnM = new wxHtmlEasyPrinting(_("Printing"));
}
//-----------------------------------------------------------------------------
wxHtmlEasyPrinting *HtmlPrinter::getHEP()
{
	static HtmlPrinter instance;
	return instance.prnM;
}
//-----------------------------------------------------------------------------
HtmlPrinter::~HtmlPrinter()
{
	delete prnM;
	prnM = 0;
}
//-----------------------------------------------------------------------------
//! PrintableHtmlWindow class
PrintableHtmlWindow::PrintableHtmlWindow(wxWindow *parent)
	: wxHtmlWindow(parent, -1)
{
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(PrintableHtmlWindow, wxHtmlWindow)
	EVT_RIGHT_UP(PrintableHtmlWindow::OnRightUp)
	EVT_MENU(PrintableHtmlWindow::ID_MENU_COPY, PrintableHtmlWindow::OnMenuCopy)
	EVT_MENU(PrintableHtmlWindow::ID_MENU_NEW_WINDOW, PrintableHtmlWindow::OnMenuNewWindow)
	EVT_MENU(PrintableHtmlWindow::ID_MENU_SAVE, PrintableHtmlWindow::OnMenuSave)
	EVT_MENU(PrintableHtmlWindow::ID_MENU_PRINT, PrintableHtmlWindow::OnMenuPrint)
	EVT_MENU(PrintableHtmlWindow::ID_MENU_PREVIEW, PrintableHtmlWindow::OnMenuPreview)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::OnRightUp(wxMouseEvent& event)
{
    wxMenu m(0);
	m.Append(ID_MENU_NEW_WINDOW, _("&Open link in a new window"));
    m.Append(ID_MENU_COPY,   	 _("&Copy"));
    m.AppendSeparator();
    m.Append(ID_MENU_SAVE,		_("&Save as HTML file..."));
    m.Append(ID_MENU_PREVIEW,	_("Print pre&view..."));
    m.Append(ID_MENU_PRINT,		_("&Print..."));

	bool isLink = false;
    if (m_Cell)	// taken from wx's htmlwin.cpp
    {
        wxPoint pos = CalcUnscrolledPosition(event.GetPosition());
        wxHtmlCell *cell = m_Cell->FindCellByPos(pos.x, pos.y);
        if (cell)
		{
			int ix = cell->GetPosX();
			int iy = cell->GetPosY();
			wxHtmlLinkInfo *i = cell->GetLink(pos.x-ix, pos.y-iy);
			if (i)
			{
				tempLinkM = i->GetHref();
				isLink = true;
			}
		}
    }

	m.Enable(ID_MENU_NEW_WINDOW, isLink);
	if (SelectionToText().IsEmpty())
		m.Enable(ID_MENU_COPY, false);
    PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::setPageSource(const wxString& html)
{
	pageSourceM = html;
	SetPage(pageSourceM);
}
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::OnMenuCopy(wxCommandEvent& WXUNUSED(event))
{
	if (wxTheClipboard->Open())
	{
		wxTheClipboard->SetData( new wxTextDataObject(SelectionToText()) );
		wxTheClipboard->Close();
	}
}
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::OnMenuNewWindow(wxCommandEvent& WXUNUSED(event))
{
	std::string addr = wx2std(tempLinkM);
	YURI uri(addr);
	if (uri.protocol != "fr")				// we don't support "new window" for non-fr protocols
		return;
	uri.addParam("target=new");
	if (!getURIProcessor().handleURI(uri))
		::wxMessageBox(_("Feature not yet implemented."), _("Information"), wxICON_INFORMATION|wxOK);
}
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::OnMenuSave(wxCommandEvent& WXUNUSED(event))
{
	wxString filename = wxFileSelector(_("Save as HTML..."), wxEmptyString, GetOpenedPageTitle(), wxT("*.html"),
		_("HTML files (*.html)|*.html|All files (*.*)|*.*"), wxSAVE|wxOVERWRITE_PROMPT);
	if (!filename.IsEmpty())
	{
		wxFile f;
		if (f.Open(filename, wxFile::write))
		{
			wxString ns(pageSourceM);
			while (true)	// remove links
			{
				int p1 = ns.Upper().find(wxT("<A"));
				int p2 = ns.Upper().find(wxT("</A>"), p1);
				if (p1 == -1 || p2 == -1)
					break;
				ns.Remove(p1, p2 - p1 + 4);
			}
			f.Write(ns);
			f.Close();
		}
	}
}
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::OnMenuPreview(wxCommandEvent& WXUNUSED(event))
{
	HtmlPrinter::getHEP()->SetHeader(GetOpenedPageTitle());
	HtmlPrinter::getHEP()->SetFooter(_("Printed from FlameRobin - www.flamerobin.org"));
	HtmlPrinter::getHEP()->PreviewText(pageSourceM);
}
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::OnMenuPrint(wxCommandEvent& WXUNUSED(event))
{
	HtmlPrinter::getHEP()->SetHeader(GetOpenedPageTitle());
	HtmlPrinter::getHEP()->SetFooter(_("Printed from FlameRobin - www.flamerobin.org"));
	HtmlPrinter::getHEP()->PrintText(pageSourceM);
}
//-----------------------------------------------------------------------------
//! Link is in format: "protocol://action?name=value&amp;name=value...etc.
void PrintableHtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
	std::string addr = wx2std(link.GetHref());
	YURI uri(addr);
	if (uri.protocol != "fr")		// call default handler for other protocols
	{
		wxHtmlWindow::OnLinkClicked(link);
		return;
	}
	if (!getURIProcessor().handleURI(addr))
		::wxMessageBox(_("Feature not yet implemented."), _("Information"), wxICON_INFORMATION|wxOK);
}
//-----------------------------------------------------------------------------
