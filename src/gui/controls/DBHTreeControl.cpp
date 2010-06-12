/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#include <wx/artprov.h>
#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/imaglist.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "core/Observer.h"
#include "core/StringUtils.h"
#include "core/Subject.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "gui/controls/DBHTreeControl.h"
// nearly all headers in src/metadata would be necessary, but...
#include "metadata/root.h"
#include "sql/SqlTokenizer.h"
//-----------------------------------------------------------------------------
// DBHTreeConfigCache: class to cache config data for tree control behaviour
class DBHTreeConfigCache: public ConfigCache, public Subject
{
private:
    bool allowDragM;
    bool hideDisconnectedDatabasesM;
    bool showColumnParamCountM;
    bool showColumnsM;
    int showComputedM;
    int showDomainsM;
    bool showSystemTablesM;
    bool sortDatabasesM;
    bool sortServersM;
    bool sqlKeywordsUpperCaseM;
    template<typename T>
    unsigned setValue(T& field, T newValue);
protected:
    virtual void loadFromConfig();
    virtual void update();
public:
    DBHTreeConfigCache();

    static DBHTreeConfigCache& get();

    bool allowDnD() { return allowDragM; };
    bool getHideDisconnectedDatabases()
        { return hideDisconnectedDatabasesM; };
    bool getShowColumnParamCount() { return showColumnParamCountM; };
    bool getShowColumns() { return showColumnsM; };
    bool getSortDatabases() { return sortDatabasesM; };
    bool getSortServers() { return sortServersM; };
};
//----------------------------------------------------------------------------
DBHTreeConfigCache::DBHTreeConfigCache()
    : ConfigCache(config()), Subject()
{
    loadFromConfig();
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
    Config& cfg(config());
    unsigned changes = 0;

    changes += setValue(allowDragM,
        cfg.get(wxT("allowDragAndDrop"), false));
    changes += setValue(hideDisconnectedDatabasesM,
        cfg.get(wxT("HideDisconnectedDatabases"), false));
    changes += setValue(showColumnParamCountM,
        cfg.get(wxT("ShowColumnAndParameterCountInTree"), false));
    changes += setValue(showColumnsM,
        cfg.get(wxT("ShowColumnsInTree"), true));
    changes += setValue(sortDatabasesM,
        cfg.get(wxT("OrderDatabasesInTree"), false));
    changes += setValue(sortServersM,
        cfg.get(wxT("OrderServersInTree"), false));
    // these aren't surfaced by methods, but needed to cause observing tree
    // nodes to update themselves
    changes += setValue(showSystemTablesM,
        cfg.get(wxT("ShowSystemTables"), true));
    changes += setValue(showComputedM,
        cfg.get(wxT("ShowComputed"), 1));
    changes += setValue(showDomainsM,
        cfg.get(wxT("ShowDomains"), 2));
    changes += setValue(sqlKeywordsUpperCaseM,
        cfg.get(wxT("SQLKeywordsUpperCase"), false));

    if (changes)
        notifyObservers();
}
//-----------------------------------------------------------------------------
template<typename T>
unsigned DBHTreeConfigCache::setValue(T& field, T newValue)
{
    if (field == newValue)
        return 0;
    field = newValue;
    return 1;
}
//-----------------------------------------------------------------------------
void DBHTreeConfigCache::update()
{
    ConfigCache::update();
    // load changed settings immediately and notify observers 
    loadFromConfig();
}
//-----------------------------------------------------------------------------
// DBHTreeImageList class
class DBHTreeImageList: public wxImageList
{
private:
    std::map<wxArtID, int> artIdIndicesM;
    void addImage(const wxArtID& art);
public:
    DBHTreeImageList();

    static DBHTreeImageList& get();
    int getImageIndex(const wxArtID& id);
    int getImageIndex(NodeType type);
};
//-----------------------------------------------------------------------------
DBHTreeImageList::DBHTreeImageList()
    : wxImageList(16, 16)
{
    addImage(ART_Object);
    addImage(ART_Column);
    addImage(ART_Computed);
    addImage(ART_DatabaseConnected);
    addImage(ART_DatabaseDisconnected);
    addImage(ART_Domain);
    addImage(ART_Domains);
    addImage(ART_Exception);
    addImage(ART_Exceptions);
    addImage(ART_ForeignKey);
    addImage(ART_Function);
    addImage(ART_Functions);
    addImage(ART_Generator);
    addImage(ART_Generators);
    addImage(ART_ParameterInput);
    addImage(ART_ParameterOutput);
    addImage(ART_PrimaryAndForeignKey);
    addImage(ART_PrimaryKey);
    addImage(ART_Procedure);
    addImage(ART_Procedures);
    addImage(ART_Role);
    addImage(ART_Roles);
    addImage(ART_Root);
    addImage(ART_Server);
    addImage(ART_SystemTable);
    addImage(ART_SystemTables);
    addImage(ART_Table);
    addImage(ART_Tables);
    addImage(ART_Trigger);
    addImage(ART_Triggers);
    addImage(ART_View);
    addImage(ART_Views);
}
//-----------------------------------------------------------------------------
/*static*/ DBHTreeImageList& DBHTreeImageList::get()
{
    static DBHTreeImageList til;
    return til;
}
//-----------------------------------------------------------------------------
void DBHTreeImageList::addImage(const wxArtID& art)
{
    wxBitmap bmp(wxArtProvider::GetBitmap(art, wxART_OTHER, wxSize(16, 16)));
    if (!bmp.Ok())
        return;
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    artIdIndicesM[art] = Add(icon);
}
//-----------------------------------------------------------------------------
int DBHTreeImageList::getImageIndex(const wxArtID& id)
{
    std::map<wxArtID, int>::const_iterator it = artIdIndicesM.find(id);
    if (it != artIdIndicesM.end())
        return (*it).second;
    return -1;
}
//-----------------------------------------------------------------------------
int DBHTreeImageList::getImageIndex(NodeType type)
{
    wxArtID id(ART_Object);
    switch (type)
    {
        case ntColumn:
            id = ART_Column; break;
        case ntDatabase:
            id = ART_DatabaseConnected; break;
        case ntDomain:
            id = ART_Domain; break;
        case ntDomains:
            id = ART_Domains; break;
        case ntException:
            id = ART_Exception; break;
        case ntExceptions:
            id = ART_Exceptions; break;
        case ntFunction:
            id = ART_Function; break;
        case ntFunctions:
            id = ART_Functions; break;
        case ntGenerator:
            id = ART_Generator; break;
        case ntGenerators:
            id = ART_Generators; break;
        case ntParameterInput:
            id = ART_ParameterInput; break;
        case ntParameterOutput:
            id = ART_ParameterOutput; break;
        case ntProcedure:
            id = ART_Procedure; break;
        case ntProcedures:
            id = ART_Procedures; break;
        case ntRole:
            id = ART_Role; break;
        case ntRoles:
            id = ART_Roles; break;
        case ntRoot:
            id = ART_Root; break;
        case ntServer:
            id = ART_Server; break;
        case ntSysTable:
            id = ART_SystemTable; break;
        case ntSysTables:
            id = ART_SystemTables; break;
        case ntTable:
            id = ART_Table; break;
        case ntTables:
            id = ART_Tables; break;
        case ntTrigger:
            id = ART_Trigger; break;
        case ntTriggers:
            id = ART_Triggers; break;
        case ntView:
            id = ART_View; break;
        case ntViews:
            id = ART_Views; break;
    }
    return getImageIndex(id);
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
    bool sortChildrenM;
    bool nodeConfigSensitiveM;

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
    bool getSortChildren() { return sortChildrenM; };
    bool isConfigSensitive() { return nodeConfigSensitiveM; };

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
        showChildrenM(false), sortChildrenM(false),
        nodeConfigSensitiveM(false)
{
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::defaultAction()
{
    // all classes that have corresponding tree nodes must have visitClass()
    wxASSERT_MSG(false, wxT("DBHTreeItemVisitor::visit[Classname]() missing"));
}
//-----------------------------------------------------------------------------
bool isNotSystem(MetadataItem* item)
{
    return item && !item->isSystem();
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::setNodeProperties(MetadataItem* metadataItem)
{
    wxASSERT(metadataItem);
    nodeVisibleM = true;
    nodeTextBoldM = false;

    // domain collection contains system domains that are not visible
    size_t childCount = 0;
    if (metadataItem->getType() == ntDomains)
    {
        std::vector<MetadataItem*> children;
        if (metadataItem->getChildren(children))
        {
            childCount = std::count_if(children.begin(), children.end(),
                &isNotSystem);
        }
    }
    else
    {
        childCount = metadataItem->getChildrenCount();
    }

    // don't touch node caption if it has already been set
    if (nodeTextM.empty())
    {
        nodeTextM = metadataItem->getName_();
        if (childCount)
            nodeTextM << wxT(" (") << childCount << wxT(")");
    }

    nodeImageIndexM = DBHTreeImageList::get().getImageIndex(
        metadataItem->getType());
    showChildrenM = childCount > 0;
    sortChildrenM = false;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitColumn(Column& column)
{
    // node text: column_name + column_datatype [+ "not null"]
    nodeTextM = column.getName_() + wxT(" ") + column.getDatatype();
    // only show first line of multiline text
    size_t nl = nodeTextM.find_first_of(wxT("\n\r"));
    if (nl != wxString::npos)
    {
        nodeTextM.Truncate(nl);
        nodeTextM += wxT("...");
    }
    if (!column.isNullable())
    {
        nodeTextM << wxT(" ") << SqlTokenizer::getKeyword(kwNOT)
            << wxT(" ") << SqlTokenizer::getKeyword(kwNULL);
    }
    // set remaining default properties, nodeTextM will not be touched
    setNodeProperties(&column);
    // image index depends on participation in primary and foreign keys
    // and is different for computed columns
    bool isPK = column.isPrimaryKey();
    bool isFK = column.isForeignKey();

    wxArtID artInvalid = wxART_MAKE_ART_ID(ART_INVALID);
    wxArtID artId = artInvalid;

    if (isPK && isFK)
        artId = ART_PrimaryAndForeignKey;
    else if (isPK)
        artId = ART_PrimaryKey;
    else if (isFK)
        artId = ART_ForeignKey;
    else if (!column.getComputedSource().IsEmpty())
        artId = ART_Computed;

    if (artId != artInvalid)
        nodeImageIndexM = DBHTreeImageList::get().getImageIndex(artId);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitDatabase(Database& database)
{
    setNodeProperties(&database);
    // show different images for connected and disconnected databases
    bool connected = database.isConnected();
    nodeImageIndexM = DBHTreeImageList::get().getImageIndex(
        connected ? ART_DatabaseConnected : ART_DatabaseDisconnected);
    // hide disconnected databases
    if (DBHTreeConfigCache::get().getHideDisconnectedDatabases())
        nodeVisibleM = connected;
    // show Collection nodes even though Database::getChildrenCount() returns 0
    showChildrenM = true;
    // update if settings change: "Show system tables in tree"
    nodeConfigSensitiveM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitDomain(Domain& domain)
{
    // skip autogenerated domains
    nodeVisibleM = !domain.isSystem();
    if (!nodeVisibleM)
        return;

    // node text: domain_name + domain_datatype [+ "not null"]
    nodeTextM = domain.getName_() + wxT(" ") + domain.getDatatypeAsString();
    if (!domain.isNullable())
    {
        nodeTextM << wxT(" ") << SqlTokenizer::getKeyword(kwNOT)
            << wxT(" ") << SqlTokenizer::getKeyword(kwNULL);
    }
    // set remaining default properties, nodeTextM will not be touched
    setNodeProperties(&domain);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitException(Exception& exception)
{
    setNodeProperties(&exception);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitFunction(Function& function)
{
    setNodeProperties(&function);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitGenerator(Generator& generator)
{
    // show generator value, but only if it has been loaded already
    if (generator.propertiesLoaded())
    {
        std::ostringstream ss;
        ss << generator.getValue();
        nodeTextM = generator.getName_() + wxT(" = ") + std2wx(ss.str());
    }
    // set remaining default properties, nodeTextM will not be touched
    setNodeProperties(&generator);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitParameter(Parameter& parameter)
{
    nodeTextM = (parameter.isOutputParameter() ? wxT("out ") : wxT("in "))
        + parameter.getName_() + wxT(" ") + parameter.getDatatype();
    // set remaining default properties, nodeTextM will not be touched
    setNodeProperties(&parameter);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitProcedure(Procedure& procedure)
{
    setNodeProperties(&procedure);
    if (procedure.childrenLoaded())
    {
        // make node caption bold when parameter data is loaded
        // (even if the procedure has no parameters at all)
        nodeTextBoldM = true;
        // show number of parameters?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            nodeTextM += wxString::Format(wxT(" (%d, %d)"),
                procedure.getInputParamCount(),
                procedure.getOutputParamCount());
        }
    }
    // show Parameter nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    // update if settings change
    nodeConfigSensitiveM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitRole(Role& role)
{
    setNodeProperties(&role);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitRoot(Root& root)
{
    setNodeProperties(&root);
    // make root node caption bold even if it has no registered servers
    nodeTextBoldM = true;
    // show Server nodes even though Root::getChildrenCount() returns 0
    showChildrenM = true;
    sortChildrenM = DBHTreeConfigCache::get().getSortServers();
    // update if settings change (sort servers?)
    nodeConfigSensitiveM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitServer(Server& server)
{
    setNodeProperties(&server);
    // make server node caption bold even if it has no registered databases
    nodeTextBoldM = true;
    // show Database nodes even though Server::getChildrenCount() returns 0
    showChildrenM = true;
    sortChildrenM = DBHTreeConfigCache::get().getSortDatabases();
    // update if settings change (sort databases?)
    nodeConfigSensitiveM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitTable(Table& table)
{
    setNodeProperties(&table);
    if (table.childrenLoaded())
    {
        // make node caption bold when column data has been loaded
        nodeTextBoldM = true;
        // show number of columns?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t colCount = table.getColumnCount();
            nodeTextM += wxString::Format(wxT(" (%d)"), colCount);
        }
    }
    // show Column nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    // update if settings change
    nodeConfigSensitiveM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitTrigger(Trigger& trigger)
{
    setNodeProperties(&trigger);
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitView(View& view)
{
    setNodeProperties(&view);
    if (view.childrenLoaded())
    {
        // make node caption bold when column data has been loaded
        nodeTextBoldM = true;
        // show number of columns?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t colCount = view.getColumnCount();
            nodeTextM += wxString::Format(wxT(" (%d)"), colCount);
        }
    }
    // show Column nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    // update if settings change
    nodeConfigSensitiveM = true;
}
//-----------------------------------------------------------------------------
void DBHTreeItemVisitor::visitMetadataItem(MetadataItem& metadataItem)
{
    setNodeProperties(&metadataItem);
}
//-----------------------------------------------------------------------------
// DBHTreeItem is a special kind of observer, which observes special kind
// of subjects: MetadataItem instances
class DBHTreeItemData: public wxTreeItemData, public Observer
{
private:
    DBHTreeControl* treeM;
protected:
    virtual void update();
public:
    DBHTreeItemData(DBHTreeControl* tree);

    wxTreeItemId findSubNode(MetadataItem* item);
    MetadataItem* getObservedMetadata();
};
//-----------------------------------------------------------------------------
DBHTreeItemData::DBHTreeItemData(DBHTreeControl* tree)
    : Observer(), treeM(tree)
{
}
//-----------------------------------------------------------------------------
//! returns tree subnode that points to given metadata object
wxTreeItemId DBHTreeItemData::findSubNode(MetadataItem* item)
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
MetadataItem* DBHTreeItemData::getObservedMetadata()
{
    // first observed Subject is always the represented MetadataItem*
    return (dynamic_cast<MetadataItem*>(getFirstSubject()));
}
//-----------------------------------------------------------------------------
struct MetadataItemSorter
{
    bool operator() (MetadataItem* item1, MetadataItem* item2)
    {
        int i = item1->getName_().CmpNoCase(item2->getName_());
        if (i == 0)
            i = item1->getName_().Cmp(item2->getName_());
        return i < 0;
    };
};
//-----------------------------------------------------------------------------
//! parent nodes are responsible for "insert" / "delete"
//! node is responsible for "update"
void DBHTreeItemData::update()
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
        // sort child nodes if necessary
        if (tivObject.getSortChildren())
        {
            MetadataItemSorter sorter;
            std::sort(children.begin(), children.end(), sorter);
        }

        wxTreeItemId prevId;
        // create or update child nodes
        for (itChild = children.begin(); itChild != children.end(); ++itChild)
        {
            DBHTreeItemVisitor tivChild(treeM);
            (*itChild)->acceptVisitor(&tivChild);
            if (!tivChild.getNodeVisible())
                continue;

            wxTreeItemId childId = findSubNode(*itChild);
            // order of child nodes may have changed
            // since nodes can't be moved they have to be recreated
            if (childId.IsOk())
            {
                wxTreeItemId prevChildId = treeM->GetPrevSibling(childId);
                if (prevChildId != prevId)
                {
                    treeM->Delete(childId);
                    childId.Unset();
                }
            }

            if (!childId.IsOk())
            {
                DBHTreeItemData* newItem = new DBHTreeItemData(treeM);
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
                // attachObserver() will call update() on the newly created
                // child node.  This will correctly populate the tree
                (*itChild)->attachObserver(newItem);
                // tree node data objects may optionally observe the settings
                // cache object, for example to create / delete column and
                // parameter nodes if the "ShowColumnsInTree" setting changes
                if (tivChild.isConfigSensitive())
                    DBHTreeConfigCache::get().attachObserver(newItem);
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

    // select item under the mouse first, since right-click doesn't change
    // the selection under GTK
    // NOTE: removing the SelectItem() call for wxMSW does not work either,
    // because commands will be enabled for the selected, not for the
    // highlighted item :-(
    wxPoint pos = ScreenToClient(event.GetPosition());
    int flags;
    const int checkFlags = wxTREE_HITTEST_ONITEMBUTTON
#ifdef __WXMSW__
        | wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT
#endif
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

    if (MyMenu.GetMenuItemCount())
        PopupMenu(&MyMenu, pos);
}
//-----------------------------------------------------------------------------
DBHTreeControl::DBHTreeControl(wxWindow* parent, const wxPoint& pos,
        const wxSize& size, long style)
    : wxTreeCtrl(parent, ID_tree_ctrl, pos, size, style)
{
    allowContextMenuM = true;
/*  FIXME: dows not play nice with wxGenericImageList...
           need to check whether sharing the image list has bad side-effects!

    // create copy of static image list, set to be autodeleted
    AssignImageList(new wxImageList(DBHTreeImageList::get()));
*/
    SetImageList(&DBHTreeImageList::get());
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
wxTreeItemId DBHTreeControl::addRootNode(MetadataItem* rootItem)
{
    wxASSERT(rootItem);
    // no need to set node text and image index, because
    // rootItem->attachObserver() will call DBHTreeItemData::update()
    wxTreeItemId id = AddRoot(wxEmptyString, -1);
    DBHTreeItemData* rootdata = new DBHTreeItemData(this);
    SetItemData(id, rootdata);
    rootItem->attachObserver(rootdata);
    // server nodes may need to be reordered
    DBHTreeConfigCache::get().attachObserver(rootdata);
    return id;
}
//-----------------------------------------------------------------------------
//! returns the object that selected wxTree node observes
MetadataItem* DBHTreeControl::getSelectedMetadataItem()
{
    return getMetadataItem(GetSelection());
}
//-----------------------------------------------------------------------------
//! returns the object that some wxTree node observes
MetadataItem* DBHTreeControl::getMetadataItem(wxTreeItemId item)
{
    if (!item.IsOk())
        return 0;

    DBHTreeItemData* tid = (DBHTreeItemData*)GetItemData(item);
    return (tid) ? tid->getObservedMetadata() : 0;
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
