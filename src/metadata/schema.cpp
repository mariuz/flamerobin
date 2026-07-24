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

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "core/StringUtils.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/schema.h"
#include "engine/db/IStatement.h"

Schema::Schema(DatabasePtr database, const wxString& name)
    : MetadataItem(ntSchema, database.get(), name)
{
}

const wxString Schema::getTypeName() const
{
    return "SCHEMA";
}

void Schema::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitSchema(*this);
}

wxString Schema::getOwner() const
{
    return ownerM;
}

void Schema::setOwner(const wxString& owner)
{
    ownerM = owner;
}

void Schema::loadDescription()
{
}

void Schema::saveDescription(const wxString& description)
{
}

Schemas::Schemas(DatabasePtr database)
    : MetadataCollection<Schema>(ntSchemas, database, _("Schemas"))
{
}

void Schemas::loadChildren()
{
    DatabasePtr db = getDatabase();
    if (!db || !db->getInfo().isFB60OrHigher())
    {
        setChildrenLoaded(true);
        return;
    }

    try
    {
        auto dalDb = db->getDALDatabase();
        if (!dalDb || !dalDb->isConnected())
        {
            setChildrenLoaded(true);
            return;
        }

        auto tr = dalDb->createTransaction();
        tr->start();

        auto st = dalDb->createStatement(tr);
        st->prepare("SELECT RDB$SCHEMA_NAME, RDB$OWNER_NAME, RDB$DESCRIPTION "
                    "FROM RDB$SCHEMAS ORDER BY RDB$SCHEMA_NAME");
        st->execute();

        while (st->fetch())
        {
            wxString name = wxString::FromUTF8(st->getString(0).c_str()).Trim();
            wxString owner = wxString::FromUTF8(st->getString(1).c_str()).Trim();
            SchemaPtr s(new Schema(db, name));
            s->setOwner(owner);
            s->setParent(this);
            insertItem(s);
        }

        tr->commit();
    }
    catch (...) {}

    setChildrenLoaded(true);
}

void Schemas::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitSchemas(*this);
}
