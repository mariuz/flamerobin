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

#include <wx/imaglist.h>
#include <wx/dataobj.h>
#include <wx/dnd.h>

#include <algorithm>
#include <vector>

#include "config/Config.h"
#include "core/Observer.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "gui/controls/DBHTreeControl.h"
#include "images.h"
#include "metadata/root.h"
//-----------------------------------------------------------------------------
//! Helper class to cache the config value and observe it's changes
class DragAndDropConfig: public Observer
{
private:
    bool loadedM;
    bool allowDragM;
protected:
    virtual void update();
public:
    DragAndDropConfig();
    static DragAndDropConfig& get();
    bool allowDnD();
};
//----------------------------------------------------------------------------
DragAndDropConfig::DragAndDropConfig()
    : loadedM(false)
{
    config().attachObserver(this);
}
//-----------------------------------------------------------------------------
DragAndDropConfig& DragAndDropConfig::get()
{
    static DragAndDropConfig dndc;
    return dndc;
}
//-----------------------------------------------------------------------------
void DragAndDropConfig::update()
{   // we observe config() object, so we better do as little as possible
    loadedM = false;
}
//----------------------------------------------------------------------------
bool DragAndDropConfig::allowDnD()
{
    if (!loadedM)
        allowDragM = config().get(wxT("allowDragAndDrop"), false);
    loadedM = true;
    return allowDragM;
}
//-----------------------------------------------------------------------------
// DBHTreeItem is a special kind of observer, which observes special kind
// of subjects: MetadataItem instances
class DBHTreeItem: public wxTreeItemData, public Observer
{
private:
    DBHTreeControl* treeM;
protected:
    virtual void update();
public:
    DBHTreeItem(DBHTreeControl* tree);

    wxTreeItemId findSubNode(MetadataItem* item);
    MetadataItem* getObservedMetadata();
};
//-----------------------------------------------------------------------------
DBHTreeItem::DBHTreeItem(DBHTreeControl* tree)
    : Observer(), treeM(tree)
{
}
//-----------------------------------------------------------------------------
//! returns tree subnode that points to given metadata object
wxTreeItemId DBHTreeItem::findSubNode(MetadataItem* item)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId id = GetId();
    for (wxTreeItemId ci = treeM->GetFirstChild(id, cookie); ci.IsOk();
        ci = treeM->GetNextChild(id, cookie))
    {
        if (treeM->getMetadataItem(ci) == item)
            return ci;
    }
    return wxTreeItemId();
}
//-----------------------------------------------------------------------------
MetadataItem* DBHTreeItem::getObservedMetadata()
{
    // tree item observes only one item.
    return (static_cast<MetadataItem*>(getFirstSubject()));
}
//-----------------------------------------------------------------------------
//! parent nodes are responsible for "insert" / "delete"
//! node is responsible for "update"
void DBHTreeItem::update()
{
    wxTreeItemId id = GetId();
    if (!id.IsOk())
        return;

    MetadataItem *object = getObservedMetadata();
    if (!object)
        return;

    bool hideDisconnected = config().get(wxT("HideDisconnectedDatabases"),
        false);

    // check current item
    wxString itemText(object->getPrintableName());
    if (treeM->GetItemText(id) != itemText)
        treeM->SetItemText(id, itemText);

    NodeType ndt = object->getType();
    bool affectedBySetting = (ndt == ntTable || ndt == ntSysTable
        || ndt == ntView || ndt == ntProcedure);
    bool showColumns = !affectedBySetting ||
        config().get(wxT("ShowColumnsInTree"), true);

    // check subitems
    std::vector<MetadataItem*> children;
    std::vector<MetadataItem*>::iterator itChild;
    if (showColumns && object->getChildren(children))
    {   // check if some tree node has to be added
        wxTreeItemId prevItem;
        for (itChild = children.begin(); itChild != children.end(); ++itChild)
        {
            // skip autogenerated domains
            if ((*itChild)->getType() == ntDomain && (*itChild)->isSystem())
                continue;
            // hide disconnected databases
            if (hideDisconnected)
            {
                Database* db = dynamic_cast<Database*>(*itChild);
                if (db && !db->isConnected())
                    continue;
            }

            itemText = (*itChild)->getPrintableName();
            // only show first line of multiline text (computed columns)
            size_t newline_pos = itemText.find_first_of(wxT("\n\r"));
            if (newline_pos != wxString::npos)
            {
                itemText.Truncate(newline_pos);
                itemText += wxT("...");
            }
            wxTreeItemId item = findSubNode(*itChild);

            if (!item.IsOk())
            {   // add if needed
                DBHTreeItem* newItem = new DBHTreeItem(treeM);
                (*itChild)->attachObserver(newItem);
                int image = treeM->getItemImage((*itChild)->getType());
                if (object->getType() == ntTable)
                {
                    Column* c = static_cast<Column*>(*itChild);
                    bool isPK = c->isPrimaryKey();
                    bool isFK = c->isForeignKey();
                    if (isPK && isFK)
                        image = treeM->getItemImage(ntPrimaryForeignKey);
                    else if (isPK)
                        image = treeM->getItemImage(ntPrimaryKey);
                    else if (isFK)
                        image = treeM->getItemImage(ntForeignKey);
                    else if (!c->getComputedSource().IsEmpty())
                        image = treeM->getItemImage(ntComputed);
                }

                if (prevItem.IsOk())
                    prevItem = treeM->InsertItem(id, prevItem, itemText, image, -1, newItem);
                else    // first
                    prevItem = treeM->PrependItem(id, itemText, image, -1, newItem);
                // to populate the tree this has to be recursive
                newItem->update();
            }
            else
            {   // change text if needed
                if (treeM->GetItemText(item) != itemText)
                    treeM->SetItemText(item, itemText);
                prevItem = item;
            }
        }
    }

    // remove all children at once
    if (!children.size())
    {
        if (treeM->ItemHasChildren(id))
        {
            if (id != treeM->GetRootItem() ||
                (treeM->GetWindowStyle() & wxTR_HIDE_ROOT) == 0)
            {
                treeM->Collapse(id);
            }
            treeM->DeleteChildren(id);
            treeM->SetItemBold(id, false);
        }
        return;
    }

    // remove delete items - one by one
    bool itemsDeleted = false;
    wxTreeItemIdValue cookie;
    wxTreeItemId item = treeM->GetFirstChild(id, cookie);
    while (item.IsOk())
    {
        itChild = find(children.begin(), children.end(),
            treeM->getMetadataItem(item));
        // we may need to hide disconnected databases
        if (hideDisconnected && itChild != children.end())
        {
            Database* db = dynamic_cast<Database*>(*itChild);
            if (db && !db->isConnected())
                itChild = children.end();
        }
        // delete tree node and all children if metadata item not found
        if (itChild == children.end())
        {
            itemsDeleted = true;
            treeM->DeleteChildren(item);
            treeM->Delete(item);
            item = treeM->GetFirstChild(id, cookie);
            continue;
        }
        item = treeM->GetNextChild(id, cookie);
    }
    // force-collapse node if all children deleted
    if (itemsDeleted && 0 == treeM->GetChildrenCount(id, false) &&
       (item != treeM->GetRootItem() ||
        (treeM->GetWindowStyle() & wxTR_HIDE_ROOT) == 0) )
    {
        treeM->Collapse(id);
    }

    treeM->SetItemBold(id, treeM->ItemHasChildren(id));
    if (object->orderedChildren())
        treeM->SortChildren(id);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(DBHTreeControl, wxTreeCtrl)
    EVT_CONTEXT_MENU(DBHTreeControl::OnContextMenu)
    EVT_TREE_BEGIN_DRAG(DBHTreeControl::ID_tree_ctrl, DBHTreeControl::OnBeginDrag)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void DBHTreeControl::OnBeginDrag(wxTreeEvent& event)
{
    if (!DragAndDropConfig::get().allowDnD())
    {
        event.Skip();
        return;
    }

    wxTreeItemId item = event.GetItem();
    if (item.IsOk())
    {
        SelectItem(item);   // Needed for MSW!!!
        MetadataItem *m = getMetadataItem(item);
        if (!m)
            return;
        wxString test;
        test.Printf(wxT("OBJECT:%ld"), (uintptr_t)m);
        wxTextDataObject textData(test);
        wxDropSource source(textData, this);
        source.DoDragDrop(wxDrag_AllowMove);
    }
    else
        event.Skip();
}
//-----------------------------------------------------------------------------
//! Creates context menu
void DBHTreeControl::OnContextMenu(wxContextMenuEvent& event)
{
    if (!allowContextMenuM)
        return;

    // select item under the mouse first, since right-click doesn't change selection under GTK
    wxPoint pos = ScreenToClient(event.GetPosition());
    int flags;
    wxTreeItemId item = HitTest(pos, flags);
    if (item.IsOk() && (flags & (wxTREE_HITTEST_ONITEMBUTTON|wxTREE_HITTEST_ONITEMICON|wxTREE_HITTEST_ONITEMLABEL)))
        SelectItem(item);
    else
    {
        item = GetSelection();
        wxRect r;
        if (item.IsOk() && GetBoundingRect(item, r, true))
        {
            pos = r.GetPosition();
            pos.y += r.height/2;
        }
    }

    wxMenu MyMenu(0);    // create context menu, depending on type of clicked item
    if (!item.IsOk())
        item = GetRootItem();
    MetadataItem* i = getMetadataItem(item);
    if (!i)
        return;
    ContextMenuMetadataItemVisitor cmv(&MyMenu);
    i->acceptVisitor(&cmv);

    PopupMenu(&MyMenu, pos);
}
//-----------------------------------------------------------------------------
DBHTreeControl::DBHTreeControl(wxWindow* parent, const wxPoint& pos, const wxSize& size, long style):
    wxTreeCtrl(parent, ID_tree_ctrl, pos, size, style)
{
    allowContextMenuM = true;
    loadImages();
}
//-----------------------------------------------------------------------------
void DBHTreeControl::allowContextMenu(bool doAllow)
{
    allowContextMenuM = doAllow;
}
//-----------------------------------------------------------------------------
//! Override wxWidgets method, since it's buggy (doesn't handle negative values properly)
void DBHTreeControl::SetSpacing(short spacing)
{
    wxTreeCtrl::SetSpacing(spacing);
    m_spacing = spacing;
}
//-----------------------------------------------------------------------------
wxTreeItemId DBHTreeControl::addRootNode(const wxString& caption,
    MetadataItem* rootItem)
{
    wxASSERT(rootItem);
    wxTreeItemId id = AddRoot(caption, getItemImage(rootItem->getType()));
    DBHTreeItem* rootdata = new DBHTreeItem(this);
    SetItemData(id, rootdata);
    rootItem->attachObserver(rootdata);
    return id;
}
//-----------------------------------------------------------------------------
//! returns the object that selected wxTree node observes
MetadataItem *DBHTreeControl::getSelectedMetadataItem()
{
    return getMetadataItem(GetSelection());
}
//-----------------------------------------------------------------------------
//! returns the database of the object that selected wxTree node observes
Database *DBHTreeControl::getSelectedDatabase()
{
    MetadataItem *m = getSelectedMetadataItem();
    if (!m)
        return 0;
    return m->getDatabase();
}
//-----------------------------------------------------------------------------
//! returns the server of the object that selected wxTree node observes
Server *DBHTreeControl::getSelectedServer()
{
    MetadataItem *m = getSelectedMetadataItem();
    Server *s = dynamic_cast<Server *>(m);
    if (s)
        return s;
    Database *d = getSelectedDatabase();
    if (!d)
        return 0;
    return d->getServer();
}
//-----------------------------------------------------------------------------
//! returns the object that some wxTree node observes
MetadataItem* DBHTreeControl::getMetadataItem(wxTreeItemId item)
{
    if (!item.IsOk())
        return 0;

    DBHTreeItem* ti = (DBHTreeItem*)GetItemData(item);
    if (!ti)
        return 0;

    return ti->getObservedMetadata();
}
//-----------------------------------------------------------------------------
//! returns index of image in wxImageList for given NodeType
int DBHTreeControl::getItemImage(NodeType t)
{
    return imageMapM[t];
}
//-----------------------------------------------------------------------------
//! creates wxImageList from icons in tree
void DBHTreeControl::loadImages()
{
    for (int i=0; i<ntLastType; i++)
        imageMapM[i] = 0;

    wxImageList* imageList = new wxImageList(16, 16);
    for (int i=0; i<ntLastType; i++)
    {
        wxBitmap bmp(getImage((NodeType)i));
        if (!bmp.Ok())
            continue;
        wxIcon icon;
        icon.CopyFromBitmap(bmp);
        imageMapM[i] = imageList->Add(icon);
    }
    AssignImageList(imageList);    // autodeleted
}
//-----------------------------------------------------------------------------
// recursively searches children for item
bool DBHTreeControl::findMetadataItem(MetadataItem *item, wxTreeItemId parent)
{
    wxTreeItemIdValue cookie;
    if (item == getMetadataItem(parent))
    {
        SelectItem(parent);
        EnsureVisible(parent);
        return true;
    }
    for (wxTreeItemId node = GetFirstChild(parent, cookie); node.IsOk();
        node = GetNextChild(parent, cookie))
    {
        if (findMetadataItem(item, node))
            return true;
    }
    return false;
}
//-----------------------------------------------------------------------------
bool DBHTreeControl::selectMetadataItem(MetadataItem* item)
{
    if (item == 0)
        return false;

    return findMetadataItem(item, GetRootItem());
}
//----------------------------------------------------------------------------
//! recursively get the last child of item
wxTreeItemId DBHTreeControl::getLastItem(wxTreeItemId id)
{
    wxTreeItemId temp = GetLastChild(id);
    if (temp.IsOk())
        return getLastItem(temp);
    else
        return id;
}
//----------------------------------------------------------------------------
//! get the previous item vertically
wxTreeItemId DBHTreeControl::getPreviousItem(wxTreeItemId current)
{
    wxTreeItemId temp = current;
    temp = GetPrevSibling(temp);
    if (!temp.IsOk())
    {
        temp = GetItemParent(current);
        if (temp.IsOk())
            return temp;
        else
            return getLastItem(GetRootItem());
    }
    else
        return getLastItem(temp);
}
//----------------------------------------------------------------------------
//! get the next item vertically
wxTreeItemId DBHTreeControl::getNextItem(wxTreeItemId current)
{
    wxTreeItemId temp = current;
    wxTreeItemIdValue cookie;   // dummy - not really used
    if (ItemHasChildren(temp))
        temp = GetFirstChild(temp, cookie);
    else
    {
        while (true)
        {
            if (temp == GetRootItem()) // back to the root (start search from top)
                break;
            wxTreeItemId t = temp;
            temp = GetNextSibling(t);
            if (temp.IsOk())
                break;
            else
                temp = GetItemParent(t);
        }
    }
    return temp;
}
//----------------------------------------------------------------------------
//! searches for next node whose text starts with "text"
//! where "text" can contain wildcards: * and ?
bool DBHTreeControl::findText(const wxString& text, bool forward)
{
    wxString searchString = text.Upper() + wxT("*");
    // start from the current position in tree and look forward
    // for item that starts with that name
    wxTreeItemId start = GetSelection();
    wxTreeItemId temp = start;
    while (true)
    {
        wxString current = GetItemText(temp);
        if (current.Upper().Matches(searchString))   // found?
        {
            if (temp != start)
                SelectItem(temp);
            EnsureVisible(temp);
            return true;
        }

        if (forward)
            temp = getNextItem(temp);
        else
            temp = getPreviousItem(temp);
        if (temp == start)  // not found (we wrapped around completely)
            return false;
    }
}
//-----------------------------------------------------------------------------
