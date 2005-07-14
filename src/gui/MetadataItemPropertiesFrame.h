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

  Contributor(s): Nando Dessena
*/

#include <wx/wx.h>
#include <wx/wxhtml.h>

#include <string>
#include "BaseFrame.h"
#include "metadata/metadataitem.h"
#include "observer.h"

#ifndef METADATAITEMPROPERTIESFRAME_H
#define METADATAITEMPROPERTIESFRAME_H

//-----------------------------------------------------------------------------
class myHtmlWindow: public wxHtmlWindow
{
public:
	myHtmlWindow(wxWindow *parent);
protected:
	void OnLinkClicked(const wxHtmlLinkInfo& link);
};
//-----------------------------------------------------------------------------
class MetadataItemPropertiesFrame: public BaseFrame, public YxObserver
{
public:
	const YxMetadataItem *getObservedObject() const;
	void processHtmlFile(std::string filename);
	void setPage(const std::string& type);

    MetadataItemPropertiesFrame(wxWindow* parent, YxMetadataItem *object, int id = -1);
private:
	enum { ptSummary, ptConstraints, ptDependencies, ptTriggers, ptTableIndices } pageTypeM;

	YxMetadataItem *objectM;
	void removeObservedObject(YxSubject *object);

    void do_layout();
	void loadPage();	// force reload from outside
	void processCommand(std::string cmd, YxMetadataItem *object, std::string& htmlpage);
	void processHtmlCode(std::string& htmlpage, std::string htmlsource, YxMetadataItem *object = 0);
	void update();
    // used to remember the value among calls to getStorageName(), needed because
    // it's not possible to access objectM (see getStorageName()) after detaching from it.
    mutable std::string storageNameM;
protected:
    myHtmlWindow* window_1;
	virtual const std::string getName() const;
	virtual const std::string getStorageName() const;
	virtual const wxRect getDefaultRect() const;
};
//-----------------------------------------------------------------------------
#endif // METADATAITEMPROPERTIESFRAME_H
