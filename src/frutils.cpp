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

  The Initial Developer of the Original Code is Nando Dessena.

  Portions created by the original developer
  are Copyright (C) 2004 Nando Dessena.

  All Rights Reserved.

  $Id$

  Contributor(s): Michael Hieke
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

#include <string>
#include "metadata/metadataitem.h"
#include "metadata/table.h"
#include "frutils.h"
#include "ugly.h"
#include "main.h"
//-----------------------------------------------------------------------------
//! Returns the application path or an empty string in case of failure.
//  Code courtesy of Julian Smart.
std::string getApplicationPath()
{
	wxString argv0(wxGetApp().argv[0]);
	wxString cwd(::wxGetCwd());
    wxString str;
	wxString appVariableName(wxT("FLAMEROBIN"));

    if (!appVariableName.IsEmpty())			// Try appVariableName
    {
        str = wxGetenv(appVariableName);
        if (!str.IsEmpty())
            return wx2std(str);
    }

#if defined(__WXMAC__) && !defined(__DARWIN__)
    return wx2std(cwd);						// On Mac, the current directory is the relevant one
#endif

    if (wxIsAbsolutePath(argv0))
        return wx2std(wxPathOnly(argv0));
    else
    {
        wxString currentDir(cwd);			// Is it a relative path?
        if (currentDir.Last() != wxFILE_SEP_PATH)
            currentDir += wxFILE_SEP_PATH;
        str = currentDir + argv0;
        if (wxFileExists(str))
            return wx2std(wxPathOnly(str));
    }

    wxPathList pathList;					// Neither an absolute path nor a relative path. Search PATH.
    pathList.AddEnvList(wxT("PATH"));
    str = pathList.FindAbsoluteValidPath(argv0);
    if (!str.IsEmpty())
        return wx2std(wxPathOnly(str));

    return "";								// Failed
}
//-----------------------------------------------------------------------------
void adjustControlsMinWidth(std::list<wxWindow*> controls)
{
	using namespace std;

	int w = 0;
    wxSize sz;
    // find widest control
    for (list<wxWindow*>::iterator it = controls.begin(); it != controls.end(); ++it)
    {
        wxASSERT(*it != 0);
        sz = (*it)->GetSize();
        w = max(w, sz.GetWidth());
    }
    // set minimum width of all controls
    for (list<wxWindow*>::iterator it = controls.begin(); it != controls.end(); ++it)
    {
        sz = (*it)->GetSize();
        (*it)->SetSize(w, sz.GetHeight());
        (*it)->SetSizeHints(w, sz.GetHeight());
    }
}
//-----------------------------------------------------------------------------
void readBlob(IBPP::Statement &st, int column, std::string& result)
{
	result = "";
	if (st->IsNull(column))
		return;

	IBPP::Blob b = IBPP::BlobFactory(st->Database(), st->Transaction());
	st->Get(column, b);

	try				// if blob is empty the exception is thrown
	{				// I tried to check st1->IsNull(1) but it doesn't work
		b->Open();	// to this hack is the only way (for the time being)
	}
	catch (...)
	{
		return;
	}

	char buffer[8192];		// 8K block
	while (true)
	{
		int size = b->Read(buffer, 8192);
		if (size <= 0)
			break;
		buffer[size] = 0;
		result += buffer;
	}
	b->Close();
}
//-----------------------------------------------------------------------------
std::string selectTableColumns(Table *t, wxWindow *parent)
{
	t->checkAndLoadColumns();
	std::vector<MetadataItem *> temp;
	t->getChildren(temp);
	wxArrayString columns;
	for (std::vector<MetadataItem *>::const_iterator it = temp.begin(); it != temp.end(); ++it)
		columns.Add(std2wx((*it)->getName()));

	wxArrayInt selected_columns;
	if (!::wxGetMultipleChoices(selected_columns, _("Select one or more fields... (use ctrl key)"),  _("Table fields"), columns, parent))
		return "";

	std::string retval;
	for (size_t i=0; i<selected_columns.GetCount(); ++i)
	{
		if (i)
			retval += ", ";
		retval += wx2std(columns[selected_columns[i]]);
	}
	return retval;
}
//-----------------------------------------------------------------------------
