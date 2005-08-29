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

  Contributor(s):
*/

#ifndef FR_TREEITEM_H
#define FR_TREEITEM_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/treebase.h>
#include <wx/treectrl.h>

#include <vector>

#include "core/Observer.h"
#include "metadata/metadataitem.h"
#include "myTreeCtrl.h"
//-----------------------------------------------------------------------------
// TreeItem is a special kind of observer, which observes special kind 
// of subjects: YxMetadataItems
class TreeItem: public wxTreeItemData, public Observer
{
private:
	myTreeCtrl *treeM;

public:
	MetadataItem *getObservedMetadata();

	wxTreeItemId findSubNode(MetadataItem *item);
	TreeItem(myTreeCtrl *tree);

	virtual void update();
};
//-----------------------------------------------------------------------------
#endif // FR_TREEITEM_H
