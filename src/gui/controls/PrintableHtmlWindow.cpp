/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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

#include <wx/clipbrd.h>
#include <wx/file.h>
#include <wx/filedlg.h>

#include "images.h"
#include "urihandler.h"

#include "core/FRError.h"
#include "gui/controls/PrintableHtmlWindow.h"
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
PrintableHtmlWindow::PrintableHtmlWindow(wxWindow* parent)
    : wxHtmlWindow(parent, -1)
{
#ifdef __WXGTK20__
    // default fonts are just too big on GTK2
    int sizes[] = { 7, 8, 10, 12, 16, 22, 30 };
    SetFonts(wxEmptyString, wxEmptyString, sizes);
#endif
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(PrintableHtmlWindow, wxHtmlWindow)
    EVT_RIGHT_UP(PrintableHtmlWindow::OnRightUp)
    EVT_MENU(wxID_COPY, PrintableHtmlWindow::OnMenuCopy)
    EVT_MENU(wxID_NEW, PrintableHtmlWindow::OnMenuNewWindow)
    EVT_MENU(wxID_SAVE, PrintableHtmlWindow::OnMenuSave)
    EVT_MENU(wxID_PRINT, PrintableHtmlWindow::OnMenuPrint)
    EVT_MENU(wxID_PREVIEW, PrintableHtmlWindow::OnMenuPreview)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::OnRightUp(wxMouseEvent& event)
{
    wxMenu m(0);
    m.Append(wxID_NEW, _("&Open link in a new window"));
    m.Append(wxID_COPY, _("&Copy"));
    m.AppendSeparator();
    m.Append(wxID_SAVE, _("&Save as HTML file..."));
    m.Append(wxID_PREVIEW, _("Print pre&view..."));
    m.Append(wxID_PRINT, _("&Print..."));

    bool isLink = false;
    if (m_Cell) // taken from wx's htmlwin.cpp
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

    m.Enable(wxID_NEW, isLink);
    m.Enable(wxID_COPY, !SelectionToText().IsEmpty());
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
    wxString addr = tempLinkM;
    URI uri(addr);
    if (uri.protocol != wxT("fr"))              // we don't support "new window" for non-fr protocols
        return;
    uri.addParam(wxT("target=new"));
    if (!getURIProcessor().handleURI(uri))
        ::wxMessageBox(_("Feature not yet implemented."), _("Information"), wxICON_INFORMATION | wxOK);
}
//-----------------------------------------------------------------------------
void PrintableHtmlWindow::OnMenuSave(wxCommandEvent& WXUNUSED(event))
{
    wxString filename = wxFileSelector(_("Save as HTML..."), wxEmptyString,
        GetOpenedPageTitle(), wxT("*.html"),
        _("HTML files (*.html)|*.html|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
#else
        wxSAVE | wxOVERWRITE_PROMPT, this);
#endif
    if (!filename.IsEmpty())
    {
        wxFile f;
        if (f.Open(filename, wxFile::write))
        {
            wxString ns(pageSourceM);
            while (true)    // remove links, but leave text
            {
                int p1 = ns.Upper().find(wxT("<A"));
                if (p1 == -1)
                    break;
                int pb = ns.Upper().find(wxT(">"), p1);
                if (pb == -1)
                    break;
                int p2 = ns.Upper().find(wxT("</A>"), pb);
                if (p2 == -1)
                    break;
                ns.Remove(p2, 4);
                ns.Remove(p1, pb - p1 + 1);
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
    // shield all URI handlers
    FR_TRY

    wxString addr = link.GetHref();
    URI uri(addr);
    if (uri.protocol == wxT("info"))    // not really a link
        return;
    if (uri.protocol != wxT("fr"))      // call default handler for other protocols
    {
        wxHtmlWindow::OnLinkClicked(link);
        return;
    }
    if (!getURIProcessor().handleURI(uri))
        ::wxMessageBox(_("Feature not yet implemented."), _("Information"), wxICON_INFORMATION|wxOK);

    FR_CATCH
}
//-----------------------------------------------------------------------------
