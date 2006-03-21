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
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena, Michael Hieke
*/

//-----------------------------------------------------------------------------
#ifndef FR_INDEX_H
#define FR_INDEX_H

#include <vector>

#include "metadata/metadataitem.h"
//-----------------------------------------------------------------------------
class Index: public MetadataItem
{
public:
    // needs to be declared here as type is used in private section
    enum IndexType { itAscending, itDescending };

private:
    bool isSystemM;
    bool uniqueFlagM;
    bool activeM;
    IndexType indexTypeM;
    double statisticsM;
    std::vector<wxString> segmentsM;
protected:
    virtual void loadDescription();
    virtual void saveDescription(wxString description);
public:
    Index(bool unique, bool active, bool ascending, double statistics,
        bool system);

    virtual bool isSystem() const;
    bool isActive();
    bool isUnique();
    double getStatistics();
    IndexType getIndexType();
    wxString getFieldsAsString();
    std::vector<wxString> *getSegments();
    virtual void acceptVisitor(MetadataItemVisitor* v);
};
//-----------------------------------------------------------------------------
#endif
