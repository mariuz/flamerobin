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

#ifndef FR_DBHTREECONTROL_H
#define FR_DBHTREECONTROL_H

#include <wx/wx.h>
#include <wx/treectrl.h>

class MetadataItem;

class DBHTreeControl: public wxTreeCtrl
{
private:
    // recursive function used by selectMetadataItem
    bool findMetadataItem(MetadataItem *item, wxTreeItemId parent);
    bool allowContextMenuM;

protected:
    short m_spacing;    // fix wxWidgets bug (or lack of feature)

public:
    enum { ID_tree_ctrl = 101 };

    short GetSpacing() const { return m_spacing; }
    void SetSpacing(short spacing);

    void OnBeginDrag(wxTreeEvent& event);
    void OnContextMenu(wxContextMenuEvent& event);
    void OnTreeItemExpanding(wxTreeEvent& event);

    wxTreeItemId addRootNode(MetadataItem* rootItem);

    // Returns observed metadata item based on specified tree item
    MetadataItem *getMetadataItem(wxTreeItemId item);

    // Returns observed metadata item based on currently selected tree item
    MetadataItem *getSelectedMetadataItem();

    // Selects the tree item represented by the metadata item
    bool selectMetadataItem(MetadataItem* item);

    wxTreeItemId getLastItem(wxTreeItemId id);
    wxTreeItemId getNextItem(wxTreeItemId current);
    wxTreeItemId getPreviousItem(wxTreeItemId current);
    bool findText(const wxString& text, bool forward = true);

    void allowContextMenu(bool doAllow = true);

    DBHTreeControl(wxWindow* parent, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS);

    DECLARE_EVENT_TABLE()
};

#endif // FR_DBHTREECONTROL_H
