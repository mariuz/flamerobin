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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "core/FRError.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/Collation.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"


std::string Collation::getLoadStatement(bool list)
{
    std::string stmt("select 	"
        "c.RDB$COLLATION_NAME, "       //1
        "c.RDB$COLLATION_ID, "         //2
        "c.RDB$COLLATION_ATTRIBUTES, " //3
        "c.RDB$BASE_COLLATION_NAME, "  //4
        "c.RDB$SPECIFIC_ATTRIBUTES,  " //5
        "c.RDB$CHARACTER_SET_ID "      //6
        "from RDB$COLLATIONS c ");
    if (list)
    {
        stmt += " order by c.RDB$COLLATION_NAME";
    }
    else
        stmt += " where c.RDB$COLLATION_NAME = ?";
    return stmt;
}

void Collation::loadProperties(IBPP::Statement& statement, wxMBConv* converter)
{
    setPropertiesLoaded(false);

    int Lid;
    std::string Lstr;

    //statement->Get(1, Lstr);
    //setName(std2wxIdentifier(Lstr, converter));

    statement->Get(2, Lid);
    setMetadataId(Lid);

    statement->Get(3, Lid);
    setAttributes(Lid);

    statement->Get(4, Lstr);
    setBaseCollectionName(std2wxIdentifier(Lstr, converter));

    statement->Get(5, Lstr);
    setSpecificAttributes(std2wxIdentifier(Lstr, converter));

    statement->Get(6, Lid);
    setCharacterSetId(Lid);

    setPropertiesLoaded(true);
}

void Collation::loadProperties()
{
    setPropertiesLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(getLoadStatement(false));
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    if (!st1->Fetch())
        throw FRError(_("Exception not found: ") + getName_());

    loadProperties(st1, converter);
}

Collation::Collation()
    :MetadataItem()
{
}

Collation::Collation(DatabasePtr database, const wxString& name, int id)
    :MetadataItem(ntCollation, database.get(), name, id),
    attributesM(0), baseCollectionNameM(""), specificAttibutesM("")
{
}

Collation::Collation(MetadataItem* parent, const wxString& name, int id)
    :MetadataItem(ntCollation, parent, name, id),
    attributesM(0), baseCollectionNameM(""), specificAttibutesM("")
{
}


Collation::~Collation()
{
}

CharacterSet* Collation::getCharacterSet() const
{
    return dynamic_cast<CharacterSet*>(getParent());
}

int Collation::getAttributes()
{
    return attributesM;
}

void Collation::setAttributes(int& attributes)
{
    attributesM = attributes;
}

int Collation::getCharacterSetId()
{
    return characterSetIdM;
}

void Collation::setCharacterSetId(int& characterSetId)
{
    characterSetIdM = characterSetId;
}

wxString Collation::getBaseCollectionName()
{
    return baseCollectionNameM;
}

void Collation::setBaseCollectionName(wxString baseName)
{
    baseCollectionNameM = baseName;
}

wxString Collation::getSpecificAttibutes()
{
    return specificAttibutesM;
}

void Collation::setSpecificAttributes(wxString attibutes)
{
    specificAttibutesM = attibutes;
}

void Collation::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitCollation(*this);
}

const wxString Collation::getTypeName() const
{
    return "COLLATION";
}

wxString Collation::getSource()
{
    ensurePropertiesLoaded();
    wxString sql  ="  FOR " + getDatabase()->getCharsetById(characterSetIdM)->getName_() + " \n";
    if (!getBaseCollectionName().IsEmpty())
        sql += "  FROM EXTERNAL ('" + getBaseCollectionName() + "') ";
        
        
    if(getAttributes() & TEXTTYPE_ATTR_CASE_INSENSITIVE)
        sql += "\n  PAD SPACE";
    if (getAttributes() & TEXTTYPE_ATTR_PAD_SPACE)
        sql += "\n  CASE INSENSITIVE";
    if (getAttributes() & TEXTTYPE_ATTR_ACCENT_INSENSITIVE)
        sql += "\n  ACCENT INSENSITIVE";
    
    if (!getSpecificAttibutes().IsEmpty())
        sql += "\n  '" + getSpecificAttibutes() + "'";

    return sql;
}

wxString Collation::getAlterSql()
{
    return getDropSqlStatement()+"\n\n "
        
        "CREATE COLLATION " + getName_() + "\n" + getSource() + ";\n";
}

void SysCollations::loadChildren()
{

    load(0);
}

SysCollations::SysCollations(DatabasePtr database)
    : MetadataCollection<Collation>(ntCollations, database, _("SysCollation"))
{
}

void SysCollations::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitSysCollations(*this);
}

void SysCollations::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    wxString stmt(  " Select RDB$COLLATION_NAME "
                    " from RDB$COLLATIONS  "
                    " Order By RDB$COLLATION_NAME "
    );
    setItems(db->loadIdentifiers(stmt, progressIndicator));
}

const wxString SysCollations::getTypeName() const
{
    return  "SYSCOLLATION_COLLECTION";
}

void Collations::loadChildren()
{
    load(0);
}

Collations::Collations(DatabasePtr database)
    : MetadataCollection<Collation>(ntCollations, database, _("Collation"))
{
}

void Collations::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitCollations(*this);
}

void Collations::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    wxString stmt(" Select RDB$COLLATION_NAME "
        " from RDB$COLLATIONS  "
        " where RDB$SYSTEM_FLAG = 0 "
        " Order By RDB$COLLATION_NAME ");
    setItems(db->loadIdentifiers(stmt, progressIndicator));
}

const wxString Collations::getTypeName() const
{
    return  "COLLATION_COLLECTION";
}
