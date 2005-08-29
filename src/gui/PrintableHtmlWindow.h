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
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena
*/

#ifndef FR_PRINTABLE_HTML_WINDOW_H
#define FR_PRINTABLE_HTML_WINDOW_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/wxhtml.h>

class wxHtmlEasyPrinting;
//-----------------------------------------------------------------------------
class HtmlPrinter
{
private:
	wxHtmlEasyPrinting *prnM;
	HtmlPrinter();
public:
	static wxHtmlEasyPrinting *getHEP();
	~HtmlPrinter();
};
//-----------------------------------------------------------------------------
class PrintableHtmlWindow: public wxHtmlWindow
{
private:
	wxString pageSourceM;
	wxString tempLinkM;		// set before context menu pops up, and used in handler for menu item
public:
	PrintableHtmlWindow(wxWindow *parent);
	void setPageSource(const wxString& html);
	enum {  ID_MENU_COPY=500, ID_MENU_NEW_WINDOW, ID_MENU_SAVE, ID_MENU_PRINT, ID_MENU_PREVIEW };
protected:
	virtual void OnLinkClicked(const wxHtmlLinkInfo& link);

	void OnRightUp(wxMouseEvent& event);
    void OnMenuCopy(wxCommandEvent& event);
    void OnMenuNewWindow(wxCommandEvent& event);
    void OnMenuSave(wxCommandEvent& event);
    void OnMenuPrint(wxCommandEvent& event);
    void OnMenuPreview(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
