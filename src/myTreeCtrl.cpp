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

  Contributor(s):
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

//
//
//
//
#include <wx/imaglist.h>
#include <wx/dataobj.h>
#include <wx/dnd.h>

#include "ugly.h"
#include "treeitem.h"
#include "images.h"
#include "metadata/root.h"
#include "myTreeCtrl.h"
#include <stack>
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(myTreeCtrl, wxTreeCtrl)
#if wxCHECK_VERSION(2, 5, 4) && !wxCHECK_VERSION(2, 5, 5)
	// this is needed so context menu can be invoked with keyboard with wx2.5.4
	EVT_TREE_ITEM_MENU(myTreeCtrl::ID_tree_ctrl, myTreeCtrl::OnItemMenu)
#endif
    EVT_CONTEXT_MENU(myTreeCtrl::OnContextMenu)
	EVT_TREE_BEGIN_DRAG(myTreeCtrl::ID_tree_ctrl, myTreeCtrl::OnBeginDrag)
	EVT_TREE_ITEM_GETTOOLTIP(myTreeCtrl::ID_tree_ctrl, myTreeCtrl::OnItemGetTooltip)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
#if wxCHECK_VERSION(2, 5, 4) && !wxCHECK_VERSION(2, 5, 5)
void myTreeCtrl::OnItemMenu(wxTreeEvent& event)
{
	wxTreeItemId id = event.GetItem();
	wxRect r;
	GetBoundingRect(id, r);
	wxPoint pos(r.x + r.width/2, r.y + r.height/2);
	wxContextMenuEvent e;
	e.SetPosition(pos);
	OnContextMenu(e);
}
#endif
//-----------------------------------------------------------------------------
void myTreeCtrl::OnItemGetTooltip(wxTreeEvent& event)
{
	wxTreeItemId item = event.GetItem();
	if (item.IsOk())
	{
		YxMetadataItem *m = getMetadataItem(item);
		if (!m)
		{
			event.SetToolTip(wxEmptyString);
			return;
		}

		YDatabase *d = m->getDatabase();
		if (d)
		{
			std::string s = d->getUsername() + "@" + d->getParent()->getName() + ":" +
				d->getPath() + " (" + d->getCharset() + ")";
			event.SetToolTip(std2wx(s));
		}
		else
			event.SetToolTip(wxEmptyString);
	}
}
//-----------------------------------------------------------------------------
void myTreeCtrl::OnBeginDrag(wxTreeEvent& event)
{
	wxTreeItemId item = event.GetItem();
	if (item.IsOk())
	{
		SelectItem(item);
		YxMetadataItem *m = getMetadataItem(item);
		if (!m)
			return;
		wxString test;
		test.Printf(wxT("OBJECT:%d"), (int)m);
		wxTextDataObject textData(test);
		wxDropSource source(textData, this);
		source.DoDragDrop(wxDrag_AllowMove);
	}
	else
		event.Skip();
}
//-----------------------------------------------------------------------------
//! Creates context menu
void myTreeCtrl::OnContextMenu(wxContextMenuEvent& event)
{
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

    wxMenu MyMenu(0);	// create context menu, depending on type of clicked item
	if (!item.IsOk() || item == GetRootItem())	// root item or no item selected, show default menu
	{
		if (item == GetRootItem())
		{
			MyMenu.Append(Menu_RegisterServer, _("Register server..."));
			MyMenu.AppendSeparator();
		}
		MyMenu.Append(Menu_About, _("About FlameRobin..."));
		MyMenu.Append(Menu_Configure, _("Configuration options..."));
		MyMenu.AppendSeparator();
		MyMenu.Append(Menu_Quit, _("Quit"));
	}
	else
	{	// read item data to find out what is the type of item
		YxMetadataItem *i = getMetadataItem(item);
		if (!i)
			return;

		switch (i->getType())
		{
			case ntServer:
				MyMenu.Append(Menu_RegisterDatabase, _("Register existing database..."));
				MyMenu.Append(Menu_CreateDatabase, _("Create new database..."));
				MyMenu.Append(Menu_ManageUsers, _("Manage users..."));
				//MyMenu.AppendSeparator();
				//MyMenu.Append(Menu_RestartServer, _("Restart server"));
				//MyMenu.Append(Menu_StopServer, _("Stop server"));
				MyMenu.AppendSeparator();
				MyMenu.Append(Menu_UnRegisterServer, _("Unregister server"));
				MyMenu.Append(Menu_ServerProperties, _("Server registration info..."));
				break;

			case ntDatabase:
				MyMenu.Append(Menu_Connect, _("Connect"));
				MyMenu.Append(Menu_Disconnect, _("Disconnect"));
				MyMenu.Append(Menu_Reconnect, _("Reconnect"));
				MyMenu.Append(Menu_Query, _("Run a query..."));
				MyMenu.AppendSeparator();
				MyMenu.Append(Menu_ShowConnectedUsers, _("Show connected users"));
				MyMenu.Append(Menu_DatabaseRegistrationInfo, _("Database registration info..."));
				MyMenu.Append(Menu_UnRegisterDatabase, _("Unregister database"));
				MyMenu.AppendSeparator();
				MyMenu.Append(Menu_Backup, _("Backup database..."));
				MyMenu.Append(Menu_Restore, _("Restore database..."));
				break;

			case ntGenerators:
				MyMenu.Append(Menu_ShowAllGeneratorValues, _("Show all values"));
			case ntTables:
			case ntViews:
			case ntProcedures:
			case ntTriggers:
			case ntDomains:
			case ntRoles:
            case ntExceptions:
				MyMenu.Append(Menu_CreateObject, _("Create new..."));
				break;

			case ntFunctions:
				MyMenu.Append(Menu_CreateObject, _("Declare new..."));
				break;

			case ntTable:
			case ntSysTable:
				MyMenu.Append(Menu_Insert, _("Insert into ..."));
			case ntView:
			case ntProcedure:
				MyMenu.Append(Menu_Browse, _("Select * from ..."));
				MyMenu.Append(Menu_BrowseColumns, _("Select col1, col2, ... from ..."));
				MyMenu.AppendSeparator();
				MyMenu.Append(Menu_LoadColumnsInfo, _("Show columns info"));
			case ntTrigger:
			case ntRole:
			case ntDomain:
            case ntException:
			case ntFunction:
				MyMenu.Append(Menu_DropObject, _("Drop"));
				MyMenu.Append(Menu_ObjectProperties, _("Properties..."));
				break;

			case ntGenerator:
				MyMenu.Append(Menu_ShowGeneratorValue, _("Show value"));
				MyMenu.Append(Menu_SetGeneratorValue, _("Set value"));
				MyMenu.Append(Menu_DropObject, _("Drop"));
				MyMenu.Append(Menu_ObjectProperties, _("Properties..."));
				break;

			case ntColumn:
				if (dynamic_cast<YTable *>(i->getParent()))		// only for table columns
				{
					MyMenu.Append(Menu_DropObject, _("Drop"));
					MyMenu.Append(Menu_ObjectProperties, _("Properties..."));
				}
				break;

			default:
				break;
		}
	}

	PopupMenu(&MyMenu, pos);
}
//-----------------------------------------------------------------------------
myTreeCtrl::myTreeCtrl(wxWindow* parent, const wxPoint& pos, const wxSize& size, long style):
    wxTreeCtrl(parent, ID_tree_ctrl, pos, size, style)
{
	loadImages();
}
//-----------------------------------------------------------------------------
//! Override wxWidgets method, since it's buggy (doesn't handle negative values properly)
void myTreeCtrl::SetSpacing(short spacing)
{
	wxTreeCtrl::SetSpacing(spacing);
    m_spacing = spacing;
}
//-----------------------------------------------------------------------------
//! returns the object that selected wxTree node observes
YxMetadataItem *myTreeCtrl::getSelectedMetadataItem()
{
	return getMetadataItem(GetSelection());
}
//-----------------------------------------------------------------------------
//! returns the object that some wxTree node observes
YxMetadataItem *myTreeCtrl::getMetadataItem(wxTreeItemId item)
{
	if (!item.IsOk())
		return 0;

	YTreeItem *d = (YTreeItem *)(GetItemData(item));
	if (!d)
		return 0;

	return d->getObservedMetadata();
}
//-----------------------------------------------------------------------------
//! returns index of image in wxImageList for given NodeType
int myTreeCtrl::getItemImage(NodeType t)
{
	return imageMapM[t];
}
//-----------------------------------------------------------------------------
//! creates wxImageList from icons in tree
void myTreeCtrl::loadImages()
{
	for (int i=0; i<ntLastType; i++)
		imageMapM[i] = 0;

	wxImageList* imageList = new wxImageList(16, 16);
	for (int i=0; i<ntLastType; i++)
	{
		wxBitmap bmp = getImage((NodeType)i);
		if (!bmp.Ok())
			continue;
		wxIcon icon;
		icon.CopyFromBitmap(bmp);
		imageMapM[i] = imageList->Add(icon);
	}
	AssignImageList(imageList);	// autodeleted
}
//-----------------------------------------------------------------------------
bool myTreeCtrl::selectMetadataItem(YxMetadataItem* item)
{
    if (item == 0)
        return false;
    // create a stack of parent metadata items (break before root node)
    std::stack<YxMetadataItem*> metaitems;
	metaitems.push(item);
	YxMetadataItem* parent = item->getParent();
	while (parent != 0 && parent->getParent() != 0)
	{
		metaitems.push(parent);
        parent = parent->getParent();
	}
    // walk stack of metadata items to find item in treenode hierarchy
    wxTreeItemId parentNode = GetRootItem();
    while (!metaitems.empty())
    {
        parent = metaitems.top();
        wxTreeItemIdValue cookie;
        for (wxTreeItemId node = GetFirstChild(parentNode, cookie);
             node.IsOk(); node = GetNextChild(parentNode, cookie))
        {
			YxMetadataItem* nodeItem = getMetadataItem(node);
            if (parent == nodeItem)
            {
				if (item == nodeItem)
				{
					SelectItem(node);
					EnsureVisible(node);
					return true;
				}
                metaitems.pop();
                parentNode = node;
                break;
            }
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
