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

#ifndef METADATAITEMPROPERTIESFRAME_H
#define METADATAITEMPROPERTIESFRAME_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/wxhtml.h>

#include <string>

#include "core/Observer.h"
#include "gui/BaseFrame.h"
#include "metadata/metadataitem.h"
#include "PrintableHtmlWindow.h"
//-----------------------------------------------------------------------------
class MetadataItemPropertiesFrame: public BaseFrame, public Observer
{
private:
    enum { ptSummary, ptConstraints, ptDependencies, ptTriggers, ptTableIndices } pageTypeM;

    MetadataItem *objectM;
    void removeSubject(Subject* subject);

    void do_layout();
    void loadPage();    // force reload from outside
    void processCommand(std::string cmd, MetadataItem *object, std::string& htmlpage);
    void processHtmlCode(std::string& htmlpage, std::string htmlsource, MetadataItem *object = 0);
    void update();
    // used to remember the value among calls to getStorageName(), needed because
    // it's not possible to access objectM (see getStorageName()) after detaching from it.
    mutable std::string storageNameM;
protected:
    PrintableHtmlWindow* window_1;
    virtual const std::string getName() const;
    virtual const std::string getStorageName() const;
    virtual const wxRect getDefaultRect() const;
public:
    MetadataItem *getObservedObject() const;
    void processHtmlFile(std::string fileName);
    void setPage(const std::string& type);

    MetadataItemPropertiesFrame(wxWindow* parent, MetadataItem *object, int id = -1);
};
//-----------------------------------------------------------------------------
#endif // METADATAITEMPROPERTIESFRAME_H
