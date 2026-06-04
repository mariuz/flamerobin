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

#include <wx/clipbrd.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/platform.h>
#include <wx/wxhtml.h>

#include "core/URIProcessor.h"
#include "core/FRError.h"
#include "gui/controls/PrintableHtmlWindow.h"

#ifdef _DEBUG
    enum { CmdCopyAllHtml = wxID_HIGHEST + 1 };
#endif

HtmlPrinter::HtmlPrinter()
{
    prnM = new wxHtmlEasyPrinting(_("Printing"));
}

wxHtmlEasyPrinting *HtmlPrinter::getHEP()
{
    static HtmlPrinter instance;
    return instance.prnM;
}

HtmlPrinter::~HtmlPrinter()
{
    delete prnM;
    prnM = 0;
}

//! PrintableHtmlWindow class
PrintableHtmlWindow::PrintableHtmlWindow(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    webViewM = wxWebView::New(this, wxID_ANY);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(webViewM, 1, wxEXPAND);
    SetSizer(sizer);
    Layout();

    if (webViewM)
    {
        webViewM->Bind(wxEVT_WEBVIEW_NAVIGATING, &PrintableHtmlWindow::OnWebViewNavigating, this);
        webViewM->Bind(wxEVT_RIGHT_UP, &PrintableHtmlWindow::OnRightUp, this);
    }
}

BEGIN_EVENT_TABLE(PrintableHtmlWindow, wxPanel)
    EVT_RIGHT_UP(PrintableHtmlWindow::OnRightUp)
    EVT_MENU(wxID_COPY, PrintableHtmlWindow::OnMenuCopy)
    #ifdef _DEBUG
        EVT_MENU(CmdCopyAllHtml, PrintableHtmlWindow::OnMenuCopyAllHtml)
    #endif
    EVT_MENU(wxID_SAVE, PrintableHtmlWindow::OnMenuSave)
    EVT_MENU(wxID_PRINT, PrintableHtmlWindow::OnMenuPrint)
    EVT_MENU(wxID_PREVIEW, PrintableHtmlWindow::OnMenuPreview)
END_EVENT_TABLE()

void PrintableHtmlWindow::OnRightUp(wxMouseEvent& WXUNUSED(event))
{
    wxMenu m;
    m.Append(wxID_COPY, _("&Copy"));
    #ifdef _DEBUG
        m.AppendSeparator();
        m.Append(CmdCopyAllHtml, _("Copy &HTML code"));
    #endif
    m.AppendSeparator();
    m.Append(wxID_SAVE, _("&Save as HTML file..."));
    m.Append(wxID_PREVIEW, _("Print pre&view..."));
    m.Append(wxID_PRINT, _("&Print..."));

    m.Enable(wxID_COPY, webViewM && webViewM->CanCopy());
    PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}

void PrintableHtmlWindow::setPageSource(const wxString& html)
{
    pageSourceM = html;
    if (webViewM)
        webViewM->SetPage(html, "");
}

bool PrintableHtmlWindow::LoadFile(const wxString& filepath)
{
    if (webViewM)
    {
        wxString url = filepath;
        url.Replace("\\", "/");
        if (!url.StartsWith("file://"))
        {
            if (url.StartsWith("/"))
                url = "file://" + url;
            else
                url = "file:///" + url;
        }
        webViewM->LoadURL(url);
        return true;
    }
    return false;
}

void PrintableHtmlWindow::GetViewStart(int* x, int* y)
{
    if (x) *x = 0;
    if (y) *y = 0;
}

void PrintableHtmlWindow::Scroll(int /*x*/, int /*y*/)
{
}

wxString PrintableHtmlWindow::GetOpenedPageTitle()
{
    if (webViewM)
        return webViewM->GetCurrentTitle();
    return wxEmptyString;
}

void PrintableHtmlWindow::SetRelatedFrame(wxFrame* /*frame*/, const wxString& /*format*/)
{
}

void PrintableHtmlWindow::SetRelatedStatusBar(int /*bar*/)
{
}

void PrintableHtmlWindow::OnMenuCopy(wxCommandEvent& WXUNUSED(event))
{
    if (webViewM)
        webViewM->Copy();
}

void PrintableHtmlWindow::OnMenuCopyAllHtml(wxCommandEvent& WXUNUSED(event))
{
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(pageSourceM));
        wxTheClipboard->Close();
    }
}

void notImplementedMessage(wxWindow* parent)
{
    ::wxMessageBox(_("Feature not yet implemented."), _("Information"),
        wxICON_INFORMATION | wxOK, parent);
}

void PrintableHtmlWindow::OnMenuSave(wxCommandEvent& WXUNUSED(event))
{
    wxString filename = wxFileSelector(_("Save as HTML..."), wxEmptyString,
        GetOpenedPageTitle(), "*.html",
        _("HTML files (*.html)|*.html|All files (*.*)|*.*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (filename.IsEmpty())
        return;

    wxFile f;
    if (f.Open(filename, wxFile::write))
    {
        wxString ns(pageSourceM.Upper());
        while (true)    // remove links, but leave text
        {
            size_t p1 = ns.find("<A");
            if (p1 == wxNOT_FOUND)
                break;
            size_t pb = ns.find(">", p1);
            if (pb == wxNOT_FOUND)
                break;
            size_t p2 = ns.find("</A>", pb);
            if (p2 == wxNOT_FOUND)
                break;
            ns.Remove(p2, 4);
            ns.Remove(p1, pb - p1 + 1);
        }
        f.Write(ns);
        f.Close();
    }
}

void PrintableHtmlWindow::OnMenuPreview(wxCommandEvent& WXUNUSED(event))
{
    HtmlPrinter::getHEP()->SetHeader(GetOpenedPageTitle());
    HtmlPrinter::getHEP()->SetFooter(
        _("Printed from FlameRobin - www.flamerobin.org"));
    HtmlPrinter::getHEP()->PreviewText(pageSourceM);
}

void PrintableHtmlWindow::OnMenuPrint(wxCommandEvent& WXUNUSED(event))
{
    HtmlPrinter::getHEP()->SetHeader(GetOpenedPageTitle());
    HtmlPrinter::getHEP()->SetFooter(
        _("Printed from FlameRobin - www.flamerobin.org"));
    HtmlPrinter::getHEP()->PrintText(pageSourceM);
}

//! Link is in format: "protocol://action?name=value&amp;name=value...etc.
void PrintableHtmlWindow::OnWebViewNavigating(wxWebViewEvent& event)
{
    wxString addr = event.GetURL();
    URI uri(addr);
    if (uri.protocol == "info")    // not really a link
    {
        event.Veto();
        return;
    }
    if (uri.protocol != "fr") // let default handler handle other protocols
        return;

    event.Veto();

    // open in new tab if control/command key is down
    // open in new window if shift key is down
    bool openInTab;
    if (wxPlatformInfo::Get().GetOperatingSystemId() & wxOS_MAC)
        openInTab = ::wxGetKeyState(WXK_COMMAND);
    else
        openInTab = ::wxGetKeyState(WXK_CONTROL);
    if (openInTab)
        uri.addParam("target=new_tab");
    else if (::wxGetKeyState(WXK_SHIFT))
        uri.addParam("target=new");

    if (!getURIProcessor().handleURI(uri))
        notImplementedMessage(this);
}
