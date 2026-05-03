/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "engine/MetadataLoader.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/publication.h"
#include "core/StringUtils.h"
#include "frutils.h"

Publication::Publication(DatabasePtr database, const wxString& name)
    : MetadataItem(ntPublication, database.get(), name), allTablesM(false)
{
}

const wxString Publication::getTypeName() const
{
    return "PUBLICATION";
}

void Publication::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitPublication(*this);
}

void Publication::setTables(const wxArrayString& tables)
{
    tablesM = tables;
}

void Publication::setAllTables(bool all)
{
    allTablesM = all;
}

void Publication::ensureTablesLoaded()
{
    if (tablesM.empty() && !allTablesM)
    {
        DatabasePtr db = getDatabase();
        if (!db)
            return;

        MetadataLoader* loader = db->getMetadataLoader();
        MetadataLoaderTransaction tr(loader);
        SubjectLocker lock(this);

        fr::IStatementPtr& st = loader->getStatement(
            "select rdb$table_name from rdb$publication_tables where rdb$publication_name = ? order by 1"
        );
        st->setString(0, wx2std(getName_(), db->getCharsetConverter()));
        st->execute();
        wxArrayString tables;
        while (st->fetch())
        {
            tables.push_back(std2wxIdentifier(st->getString(0), db->getCharsetConverter()));
        }
        setTables(tables);
        // If there are no tables in rdb$publication_tables, it's defined as FOR ALL TABLES
        setAllTables(tables.empty());
    }
}

wxArrayString Publication::getTables()
{
    ensureTablesLoaded();
    return tablesM;
}

bool Publication::getAllTables() const
{
    // Need to ensure loaded to know if it's all tables
    const_cast<Publication*>(this)->ensureTablesLoaded();
    return allTablesM;
}

void Publication::loadDescription()
{
    MetadataItem::loadDescription();
}

void Publication::saveDescription(const wxString& description)
{
    MetadataItem::saveDescription("COMMENT ON PUBLICATION " + getQuotedName() + " IS ",
        description);
}

// Publications
Publications::Publications(DatabasePtr database)
    : MetadataCollection<Publication>(ntPublications, database, _("Publications"))
{
}

void Publications::loadChildren()
{
    DatabasePtr db = getDatabase();
    if (db)
    {
        wxString stmt = "select rdb$publication_name from rdb$publications order by 1";
        setItems(db->loadIdentifiers(stmt, 0));
    }
}

void Publications::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitPublications(*this);
}

// Replication
Replication::Replication(DatabasePtr database)
    : MetadataItem(ntReplication, database.get(), _("Replication"))
{
    publicationsM = std::make_shared<Publications>(database);
}

PublicationsPtr Replication::getPublications()
{
    return publicationsM;
}

const wxString Replication::getTypeName() const
{
    return "REPLICATION";
}

void Replication::loadChildren()
{
    publicationsM->setParent(this);
    setChildrenLoaded(true);
}

bool Replication::getChildren(std::vector<MetadataItem *>& temp)
{
    temp.push_back(publicationsM.get());
    return true;
}

void Replication::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitReplication(*this);
}
