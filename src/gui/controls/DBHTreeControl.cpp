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
// DBHTreeConfigCache: class to cache config data for tree control behaviour
class DBHTreeConfigCache: public ConfigCache
{
private:
    bool allowDragM;
    bool hideDisconnectedDatabasesM;
    bool showColumnsM;
protected:
    virtual void loadFromConfig();
public:
    DBHTreeConfigCache();
    
    static DBHTreeConfigCache& get();

    bool allowDnD();
    bool getHideDisconnectedDatabases();
    bool getShowColumns();
};
//----------------------------------------------------------------------------
DBHTreeConfigCache::DBHTreeConfigCache()
    : ConfigCache(config())
{
}
//-----------------------------------------------------------------------------
DBHTreeConfigCache& DBHTreeConfigCache::get()
{
    static DBHTreeConfigCache dndc;
    return dndc;
}
//-----------------------------------------------------------------------------
void DBHTreeConfigCache::loadFromConfig()
{
    allowDragM = config().get(wxT("allowDragAndDrop"), false);
    hideDisconnectedDatabasesM = config().get(wxT("HideDisconnectedDatabases"),
        false);
    showColumnsM = config().get(wxT("ShowColumnsInTree"), true);
}
//----------------------------------------------------------------------------
bool DBHTreeConfigCache::allowDnD()
{
    ensureCacheValid();
    return allowDragM;
}
//-----------------------------------------------------------------------------
bool DBHTreeConfigCache::getHideDisconnectedDatabases()
{
    ensureCacheValid();
    return hideDisconnectedDatabasesM;
}
//-----------------------------------------------------------------------------
bool DBHTreeConfigCache::getShowColumns()
{
    ensureCacheValid();
    return showColumnsM;
}
//-----------------------------------------------------------------------------
// DBHTreeItemVisitor class
class DBHTreeItemVisitor: public MetadataItemVisitor
{
private:
    DBHTreeControl* treeM;

    bool nodeVisibleM;
    bool nodeTextBoldM;
    wxString nodeTextM;
    int nodeImageIndexM;
    bool showChildrenM;

    void setNodeProperties(MetadataItem* metadataItem);
protected:
    virtual void defaultAction();
public:
    DBHTreeItemVisitor(DBHTreeControl* tree);

    bool getNodeVisible() { return nodeVisibleM; };
    wxString getNodeText() { return nodeTextM; };
    bool getNodeTextBold() { return nodeTextBoldM; };
    int getNodeImage() { return nodeImageIndexM; };
    bool getShowChildren() { return showChildrenM; };

    virtual void visitColumn(Column& column);
    virtual void visitDatabase(Database& database);
    virtual void visitDomain(Domain& domain);
    virtual void visitException(Exception& exception);
    virtual void visitFunction(Function& function);
    virtual void visitGenerator(Generator& generator);
    virtual void visitProcedure(Procedure& procedure);
    virtual void visitParameter(Parameter& parameter);
    virtual void visitRole(Role& role);
    virtual void visitRoot(Root& root);
    virtual void visitServer(Server& server);
    virtual void visitTable(Table& table);
    virtual void visitTrigger(Trigger& trigger);
    virtual void visitView(View& view);
    virtual void visitMetadataItem(MetadataItem& metadataItem);
};
//-----------------------------------------------------------------------------
DBHTreeItemVisitor::DBHTreeItemVisitor(DBHTreeControl* tree)
    : MetadataItemVisitor(), treeM(tree), nodeVisibleM(true),
        nodeTextBoldM(false), nodeTextM(), nodeImageIndexM(-1),
        showChildrenM(false)
{
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::defaultAction()
{
    // all classes that have corresponding tree nodes must have visitClass()
    wxASSERT_MSG(false, wxT("DBHTreeItemVisitor::visit[Classname]() missing"));
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::setNodeProperties(MetadataItem* metadataItem)
{
    wxASSERT(metadataItem);
    nodeVisibleM = true;
    nodeTextBoldM = false;
    nodeTextM = metadataItem->getPrintableName();
    nodeImageIndexM = treeM->getItemImageIndex(metadataItem->getType());
    showChildrenM = metadataItem->getChildrenCount() > 0;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitColumn(Column& column)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&column));
    // only show first line of multiline text (for computed columns)
    size_t nl = nodeTextM.find_first_of(wxT("\n\r"));
    if (nl != wxString::npos)
    {
        nodeTextM.Truncate(nl);
        nodeTextM += wxT("...");
    }
    // image index depends on participation in primary and foreign keys
    // and is different for computed columns
    bool isPK = column.isPrimaryKey();
    bool isFK = column.isForeignKey();
    if (isPK && isFK)
        nodeImageIndexM = treeM->getItemImageIndex(ntPrimaryForeignKey);
    else if (isPK)
        nodeImageIndexM = treeM->getItemImageIndex(ntPrimaryKey);
    else if (isFK)
        nodeImageIndexM = treeM->getItemImageIndex(ntForeignKey);
    else if (!column.getComputedSource().IsEmpty())
        nodeImageIndexM = treeM->getItemImageIndex(ntComputed);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitDatabase(Database& database)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&database));
    // hide disconnected databases
    if (DBHTreeConfigCache::get().getHideDisconnectedDatabases())
        nodeVisibleM = database.isConnected();
    // show Collection nodes even though Database::getChildrenCount() returns 0
    showChildrenM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitDomain(Domain& domain)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&domain));
    // skip autogenerated domains
    nodeVisibleM = !domain.isSystem();
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitException(Exception& exception)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&exception));
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitFunction(Function& function)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&function));
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitGenerator(Generator& generator)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&generator));
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitProcedure(Procedure& procedure)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&procedure));
    // show Parameter nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitParameter(Parameter& parameter)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&parameter));
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitRole(Role& role)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&role));
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitRoot(Root& root)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&root));
    // make root node caption bold even if it has no registered servers
    nodeTextBoldM = true;
    // show Server nodes even though Root::getChildrenCount() returns 0
    showChildrenM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitServer(Server& server)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&server));
    // make server node caption bold even if it has no registered databases
    nodeTextBoldM = true;
    // show Database nodes even though Server::getChildrenCount() returns 0
    showChildrenM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitTable(Table& table)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&table));
    // show Column nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitTrigger(Trigger& trigger)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&trigger));
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitView(View& view)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&view));
    // show Column nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitMetadataItem(MetadataItem& metadataItem)
{
    setNodeProperties(dynamic_cast<MetadataItem*>(&metadataItem));
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
    // first observed Subject is always the represented MetadataItem*
    return (dynamic_cast<MetadataItem*>(getFirstSubject()));
}
//-----------------------------------------------------------------------------
//! parent nodes are responsible for "insert" / "delete"
//! node is responsible for "update"
void DBHTreeItem::update()
{
    wxTreeItemId id = GetId();
    if (!id.IsOk())
        return;

    MetadataItem* object = getObservedMetadata();
    if (!object)
        return;

    // set node properties of current item
    DBHTreeItemVisitor tivObject(treeM);
    object->acceptVisitor(&tivObject);
    if (treeM->GetItemText(id) != tivObject.getNodeText())
        treeM->SetItemText(id, tivObject.getNodeText());
    if (treeM->GetItemImage(id) != tivObject.getNodeImage())
        treeM->SetItemImage(id, tivObject.getNodeImage());

    // check subitems
    std::vector<MetadataItem*> children;
    std::vector<MetadataItem*>::iterator itChild;
    if (tivObject.getShowChildren() && object->getChildren(children))
    {
        wxTreeItemId prevId;
        // create or update child nodes
        for (itChild = children.begin(); itChild != children.end(); ++itChild)
        {
            DBHTreeItemVisitor tivChild(treeM);
            (*itChild)->acceptVisitor(&tivChild);

            wxTreeItemId childId = findSubNode(*itChild);
            if (!childId.IsOk())
            {   
                DBHTreeItem* newItem = new DBHTreeItem(treeM);
                (*itChild)->attachObserver(newItem);
                if (prevId.IsOk())
                {
                    childId = treeM->InsertItem(id, prevId,
                        tivChild.getNodeText(), tivChild.getNodeImage(),
                        -1, newItem);
                }
                else // first
                {
                    childId = treeM->PrependItem(id, tivChild.getNodeText(),
                        tivChild.getNodeImage(), -1, newItem);
                }
                // to populate the tree this has to be recursive
                newItem->update();
            }
            else
            {
                if (treeM->GetItemText(childId) != tivChild.getNodeText())
                    treeM->SetItemText(childId, tivChild.getNodeText());
                if (treeM->GetItemImage(childId) != tivChild.getNodeImage())
                    treeM->SetItemImage(childId, tivChild.getNodeImage());
            }
            prevId = childId;
        }
    }

    bool canCollapseNode = id != treeM->GetRootItem()
        || (treeM->GetWindowStyle() & wxTR_HIDE_ROOT) == 0;

    // remove all children at once
    if (!children.size())
    {
        if (treeM->ItemHasChildren(id))
        {
            if (canCollapseNode)
                treeM->Collapse(id);
            treeM->DeleteChildren(id);
        }
        treeM->SetItemBold(id, tivObject.getNodeTextBold());
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
        if (DBHTreeConfigCache::get().getHideDisconnectedDatabases()
            && itChild != children.end())
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
    if (itemsDeleted && 0 == treeM->GetChildrenCount(id, false)
        && canCollapseNode)
    {
        treeM->Collapse(id);
    }

    treeM->SetItemBold(id, tivObject.getNodeTextBold()
        || treeM->ItemHasChildren(id));
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
    if (!DBHTreeConfigCache::get().allowDnD())
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
    const int checkFlags = wxTREE_HITTEST_ONITEMBUTTON
        | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL;
    wxTreeItemId item = HitTest(pos, flags);
    if (item.IsOk() && (flags & checkFlags))
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
    wxTreeItemId id = AddRoot(caption, getItemImageIndex(rootItem->getType()));
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
int DBHTreeControl::getItemImageIndex(NodeType t)
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
