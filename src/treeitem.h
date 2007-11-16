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

protected:
    virtual void update();
public:
    MetadataItem *getObservedMetadata();

    wxTreeItemId findSubNode(MetadataItem *item);
    TreeItem(myTreeCtrl *tree);
};
//-----------------------------------------------------------------------------
#endif // FR_TREEITEM_H
