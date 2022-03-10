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

#include "core/URIProcessor.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/MultilineEnterDialog.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/MetadataItemURIHandlerHelper.h"

class ObjectDescriptionHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    ObjectDescriptionHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const ObjectDescriptionHandler handlerInstance;
};

const ObjectDescriptionHandler ObjectDescriptionHandler::handlerInstance;

bool ObjectDescriptionHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_description")
        return false;

    MetadataItem* m = extractMetadataItemFromURI<MetadataItem>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!m || !w)
        return true;

    wxString desc = m->getDescription();
    if (GetMultilineTextFromUser(w, wxString::Format(_("Description of %s"),
        uri.getParam("object_name").c_str()), desc))
    {
        wxBusyCursor wait;
        m->setDescription(desc);
    }
    return true;
}


