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

#ifndef FR_PROCEDURE_H
#define FR_PROCEDURE_H

#include "metadata/metadataitem.h"
#include "metadata/parameter.h"
//-----------------------------------------------------------------------------
class Procedure: public MetadataItem
{
private:
    MetadataCollection<Parameter> parametersM;
    bool parametersLoadedM;
    bool loadParameters();
protected:
    virtual void loadDescription();
    virtual void saveDescription(wxString description);
public:
    Procedure();
    Procedure(const Procedure& rhs);

    virtual void lockChildren();
    virtual void unlockChildren();

    wxString getCreateSqlTemplate() const;   // overrides MetadataItem::getCreateSqlTemplate()

    bool getChildren(std::vector<MetadataItem *>& temp);
    Parameter *addParameter(Parameter &c);

    wxString getExecuteStatement();
    wxString getSelectStatement(bool withColumns);
    bool isSelectable();

    bool checkAndLoadParameters(bool force = false);
    bool getSource(wxString& source);
    wxString getAlterSql();
    wxString getDefinition();

    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
