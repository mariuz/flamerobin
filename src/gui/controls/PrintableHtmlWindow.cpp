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
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/settings.h>
#include <wx/regex.h>

#include "core/URIProcessor.h"
#include "core/FRError.h"
#include "config/Config.h"
#include "gui/controls/PrintableHtmlWindow.h"

enum {
    CmdShowDevTools = wxID_HIGHEST + 1
};

#ifdef _DEBUG
    enum { CmdCopyAllHtml = wxID_HIGHEST + 2 };
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
        webViewM->EnableAccessToDevTools(true);
        webViewM->Bind(wxEVT_WEBVIEW_NAVIGATING, &PrintableHtmlWindow::OnWebViewNavigating, this);
        webViewM->Bind(wxEVT_RIGHT_UP, &PrintableHtmlWindow::OnRightUp, this);
    }
}

PrintableHtmlWindow::~PrintableHtmlWindow()
{
    if (!tempFileM.IsEmpty() && wxFileExists(tempFileM))
    {
        wxRemoveFile(tempFileM);
    }
}

BEGIN_EVENT_TABLE(PrintableHtmlWindow, wxPanel)
    EVT_RIGHT_UP(PrintableHtmlWindow::OnRightUp)
    EVT_MENU(wxID_COPY, PrintableHtmlWindow::OnMenuCopy)
    #ifdef _DEBUG
        EVT_MENU(CmdCopyAllHtml, PrintableHtmlWindow::OnMenuCopyAllHtml)
    #endif
    EVT_MENU(CmdShowDevTools, PrintableHtmlWindow::OnMenuShowDevTools)
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
    m.Append(CmdShowDevTools, _("Developer &Tools"));
    m.AppendSeparator();
    m.Append(wxID_SAVE, _("&Save as HTML file..."));
    m.Append(wxID_PREVIEW, _("Print pre&view..."));
    m.Append(wxID_PRINT, _("&Print..."));

    m.Enable(wxID_COPY, webViewM && webViewM->CanCopy());
    m.Enable(CmdShowDevTools, webViewM != nullptr);
    PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}

void PrintableHtmlWindow::setPageSource(const wxString& html)
{
    pageSourceM = html;
    if (webViewM)
    {
        wxString processedHtml = html;

        // Convert templates path to a file:// URL format
        wxString templatesPath = config().getHtmlTemplatesPath();
        wxString fileUrl = templatesPath;
        fileUrl.Replace("\\", "/");
        if (!fileUrl.StartsWith("/"))
            fileUrl = "file:///" + fileUrl;
        else
            fileUrl = "file://" + fileUrl;

        wxString templatesPathForward = templatesPath;
        templatesPathForward.Replace("\\", "/");

        // Standardize all template path separators in the HTML to forward slashes
        processedHtml.Replace(templatesPath, templatesPathForward);

        // Strip any existing file:// or file:/// prefixes to prevent double prefixing
        processedHtml.Replace("file:///" + templatesPathForward, templatesPathForward);
        processedHtml.Replace("file://" + templatesPathForward, templatesPathForward);

        // Convert all raw template paths to file:// URLs
        processedHtml.Replace(templatesPathForward, fileUrl);

        // Construct and inject a modern, responsive CSS stylesheet
        bool isDark = wxSystemSettings::GetAppearance().IsDark();
        wxString bgColor = isDark ? "#1e1e1e" : "#ffffff";
        wxString textColor = isDark ? "#e0e0e0" : "#2d3748";
        wxString borderColor = isDark ? "#3d3d3d" : "#e2e8f0";
        wxString headerBgColor = isDark ? "#2c2c3e" : "#edf2f7";
        wxString headerTextColor = isDark ? "#ffffff" : "#1a202c";
        wxString altRowBgColor = isDark ? "#252535" : "#f7fafc";
        wxString linkColor = isDark ? "#4a90e2" : "#3182ce";
        wxString linkHoverColor = isDark ? "#6ba4e8" : "#2b6cb0";

        wxString css = wxString::Format(
            "<link href=\"https://fonts.googleapis.com/icon?family=Material+Icons\" rel=\"stylesheet\">\n"
            "<style>\n"
            "html, body {\n"
            "    margin: 0 !important;\n"
            "    padding: 12px !important;\n"
            "    font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, Helvetica, Arial, sans-serif !important;\n"
            "    font-size: 13px !important;\n"
            "    line-height: 1.5 !important;\n"
            "    background-color: %s !important;\n"
            "    color: %s !important;\n"
            "}\n"
            "table {\n"
            "    width: 100%% !important;\n"
            "    max-width: 100%% !important;\n"
            "    border-collapse: separate !important;\n"
            "    border-spacing: 0 !important;\n"
            "    border: 1px solid %s !important;\n"
            "    border-radius: 6px !important;\n"
            "    margin: 12px 0 !important;\n"
            "    overflow: hidden !important;\n"
            "    background-color: transparent !important;\n"
            "}\n"
            "tr[bgcolor=\"navy\"], tr[bgcolor=\"#CCCCFF\"] {\n"
            "    background-color: %s !important;\n"
            "    color: %s !important;\n"
            "}\n"
            "tr[bgcolor=\"navy\"] td, tr[bgcolor=\"#CCCCFF\"] td {\n"
            "    color: %s !important;\n"
            "    font-weight: 600 !important;\n"
            "}\n"
            "tr[bgcolor=\"#DDDDFF\"] {\n"
            "    background-color: %s !important;\n"
            "}\n"
            "tr[bgcolor=\"#DDDDDD\"] {\n"
            "    background-color: %s !important;\n"
            "}\n"
            "td {\n"
            "    padding: 8px 12px !important;\n"
            "    border-bottom: 1px solid %s !important;\n"
            "}\n"
            "tr:last-child td {\n"
            "    border-bottom: none !important;\n"
            "}\n"
            "a {\n"
            "    color: %s !important;\n"
            "    text-decoration: none !important;\n"
            "}\n"
            "a:hover {\n"
            "    text-decoration: underline !important;\n"
            "    color: %s !important;\n"
            "}\n"
            "img {\n"
            "    vertical-align: middle !important;\n"
            "}\n"
            ".material-icons {\n"
            "    font-size: 16px !important;\n"
            "    width: 16px !important;\n"
            "    height: 16px !important;\n"
            "    display: inline-block !important;\n"
            "    vertical-align: middle !important;\n"
            "    text-align: center !important;\n"
            "    line-height: 16px !important;\n"
            "}\n"
            ".icon-ok {\n"
            "    color: #4caf50 !important;\n"
            "    font-size: 18px !important;\n"
            "}\n"
            ".icon-ok2 {\n"
            "    color: #00bcd4 !important;\n"
            "    font-size: 18px !important;\n"
            "}\n"
            ".icon-redx {\n"
            "    color: #f44336 !important;\n"
            "    font-size: 18px !important;\n"
            "}\n"
            ".icon-drop {\n"
            "    color: #f44336 !important;\n"
            "}\n"
            ".icon-view {\n"
            "    color: #2196f3 !important;\n"
            "}\n"
            ".icon-compute {\n"
            "    color: #ff9800 !important;\n"
            "}\n"
            "</style>\n",
            bgColor, textColor, borderColor, headerBgColor, headerTextColor, headerTextColor, altRowBgColor, altRowBgColor, borderColor, linkColor, linkHoverColor
        );

        wxString::size_type headPos = processedHtml.Lower().find("</head>");
        if (headPos != wxString::npos)
            processedHtml.insert(headPos, css);
        else
            processedHtml.insert(0, css);

        // Replace vector SVG img tags with Google Material Icons dynamically
        wxRegEx imgRegex("<img[^>]*src=\"" + fileUrl + "([^/\"\\?#]+)\\.svg\"[^>]*>");
        while (imgRegex.Matches(processedHtml))
        {
            size_t start, len;
            imgRegex.GetMatch(&start, &len, 0);
            wxString iconName = imgRegex.GetMatch(processedHtml, 1);
            wxString spanTag;
            if (iconName == "ok")
                spanTag = "<span class=\"material-icons icon-ok\">check_circle</span>";
            else if (iconName == "ok2")
                spanTag = "<span class=\"material-icons icon-ok2\">done_all</span>";
            else if (iconName == "redx")
                spanTag = "<span class=\"material-icons icon-redx\">cancel</span>";
            else if (iconName == "drop")
                spanTag = "<span class=\"material-icons icon-drop\">delete</span>";
            else if (iconName == "view")
                spanTag = "<span class=\"material-icons icon-view\">visibility</span>";
            else if (iconName == "compute")
                spanTag = "<span class=\"material-icons icon-compute\">build</span>";
            else
                spanTag = "<span class=\"material-icons\">help</span>";

            processedHtml.replace(start, len, spanTag);
        }

        // Remove old temp file if it exists
        if (!tempFileM.IsEmpty() && wxFileExists(tempFileM))
        {
            wxRemoveFile(tempFileM);
            tempFileM.Clear();
        }

        // Generate a unique temp file in the templates directory
        // to avoid "Unsafe attempt to load URL" same-origin file:// security restrictions
        tempFileM = wxString::Format("%sfr_temp_%p.html", templatesPath, this);

        wxFile file(tempFileM, wxFile::write);
        if (file.IsOpened())
        {
            file.Write(processedHtml);
            file.Close();
            LoadFile(tempFileM);
        }
        else
        {
            // Fallback to SetPage if file cannot be created
            webViewM->SetPage(processedHtml, fileUrl);
        }
    }
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

void PrintableHtmlWindow::OnMenuShowDevTools(wxCommandEvent& WXUNUSED(event))
{
    if (webViewM)
        webViewM->ShowDevTools();
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
