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

  Contributor(s): Nando Dessena, Michael Hieke
*/

#ifndef FR_FIELDPROPERTIESDIALOG_H
#define FR_FIELDPROPERTIESDIALOG_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>

#include "core/Observer.h"
#include "gui/BaseDialog.h"

class Table;
class Column;
//-----------------------------------------------------------------------------
class FieldPropertiesDialog: public BaseDialog, public Observer {
public:
    enum {
        ID_textctrl_fieldname = 101,
        ID_button_ok,
        ID_button_cancel = wxID_CANCEL
    };
    // Database is required so that domains, charsets, generators can be loaded
    FieldPropertiesDialog(wxWindow* parent, Table* table, Column* column = 0);
private:
    Table* tableM;
    Column* columnM;
    void createControls();
    void layoutControls();
    void setColumnM(Column* column);
    void setTableM(Table* table);
    void updateControlsFromColumn();
    void updateControlsFromTable();
};
//-----------------------------------------------------------------------------
#endif // FR_FIELDPROPERTIESDIALOG_H
