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

  Contributor(s): Nando Dessena.
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
//-----------------------------------------------------------------------------

#include "metadata/metadataitem.h"
#include "metadata/database.h"
#include "ugly.h"
#include "dberror.h"
#include "MultilineEnterDialog.h"
#include "urihandler.h"

class ObjectDescriptionHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    // singleton; registers itself on creation.
    static const ObjectDescriptionHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const ObjectDescriptionHandler ObjectDescriptionHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool ObjectDescriptionHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "edit_description")
		return false;

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	YxMetadataItem *m = (YxMetadataItem *)mo;

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	if (m)
	{
		wxString desc = std2wx(m->getDescription());
		if (GetMultilineTextFromUser(wxString::Format(_("Description of %s"), std2wx(uriObj.getParam("object_name")).c_str()), desc, w))
		{
			wxBusyCursor wait;
			if (!m->setDescription(wx2std(desc)))
				wxMessageBox(std2wx(lastError().getMessage()), _("Error while writing description."));
		}
	}
	return true;
}
//-----------------------------------------------------------------------------

