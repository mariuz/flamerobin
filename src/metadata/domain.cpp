/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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

//-----------------------------------------------------------------------------
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <sstream>

#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
Domain::Domain():
    MetadataItem()
{
    typeM = ntDomain;
    infoLoadedM = false;
}
//-----------------------------------------------------------------------------
void Domain::loadInfo()
{
    Database* d = getDatabase(wxT("Domain::loadInfo"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select t.rdb$type, f.rdb$field_sub_type, f.rdb$field_length,"
        " f.rdb$field_precision, f.rdb$field_scale, c.rdb$character_set_name, "
        " c.rdb$bytes_per_character, f.rdb$null_flag, f.rdb$default_source, "
        " l.rdb$collation_name, f.rdb$validation_source "
        " from rdb$fields f"
        " join rdb$types t on f.rdb$field_type=t.rdb$type"
        " left outer join rdb$character_sets c "
            " on c.rdb$character_set_id = f.rdb$character_set_id"
        " left outer join rdb$collations l "
            " on l.rdb$collation_id = f.rdb$collation_id "
            " and l.rdb$character_set_id = f.rdb$character_set_id"
        " where f.rdb$field_name = ?"
        " and t.rdb$field_name='RDB$FIELD_TYPE'"
    );

    st1->Set(1, wx2std(getName_()));
    st1->Execute();
    if (!st1->Fetch())
        throw FRError(_("Domain not found: ") + getName_());
    st1->Get(1, &datatypeM);
    if (st1->IsNull(2))
        subtypeM = 0;
    else
        st1->Get(2, &subtypeM);
    st1->Get(3, &lengthM);
    // rdb$character_length does not work properly with computed columns
    if (!st1->IsNull(7))
    {
        int bpc;
        st1->Get(7, bpc);
        lengthM /= bpc;
    }
    if (st1->IsNull(4))
        precisionM = 0;
    else
        st1->Get(4, &precisionM);
    if (st1->IsNull(5))
        scaleM = 0;
    else
        st1->Get(5, &scaleM);
    if (st1->IsNull(6))
        charsetM = wxT("");
    else
    {
        std::string charset;
        st1->Get(6, charset);
        charsetM = std2wx(charset).Strip();
    }
    isNotNullM = !st1->IsNull(8);
    readBlob(st1, 9, defaultM);

    hasDefaultM = !st1->IsNull(9);
    if (hasDefaultM)
    {
        // Some users reported two spaces before DEFAULT word in source
        // Perhaps some other tools can put garbage here? Should we
        // parse it as SQL to clean up comments, whitespace, etc?
        defaultM.Trim(false).Remove(0, 8);
    }

    if (st1->IsNull(10))
        collationM = wxEmptyString;
    else
    {
        std::string coll;
        st1->Get(10, coll);
        collationM = std2wx(coll).Strip();
    }
    readBlob(st1, 11, checkM);

    if (!isSystem())
        notifyObservers();
    infoLoadedM = true;
}
//-----------------------------------------------------------------------------
bool Domain::isString()
{
    if (!infoLoadedM)
        loadInfo();
    return (datatypeM == 14 || datatypeM == 10 || datatypeM == 37);
}
//-----------------------------------------------------------------------------
//! returns column's datatype as human readable wxString.
wxString Domain::getDatatypeAsString()
{
    if (!infoLoadedM)
        loadInfo();

    return datatype2string(datatypeM, scaleM, precisionM, subtypeM, lengthM);
}
//-----------------------------------------------------------------------------
wxString Domain::datatype2string(short datatype, short scale, short precision,
    short subtype, short length)
{
    std::ostringstream retval;      // this will be returned

    // special case (mess that some tools (ex. IBExpert) make by only
    // setting scale and not changing type)
    if (datatype == 27 && scale < 0)
    {
        retval << "Numeric(15," << -scale << ")";
        return std2wx(retval.str());
    }

    // INTEGERs (prec=0), DECIMAL(sub_type=2), NUMERIC(sub_type=1)
    if (datatype == 7 || datatype == 8 || datatype == 16)
    {
        if (scale == 0 && (datatype == 7 || datatype == 8))
        {
            if (datatype == 7)
                return wxT("Smallint");
            else
                return wxT("Integer");
        }
        else
        {
            retval << (subtype == 2 ? "Decimal(" : "Numeric(");
            if (precision <= 0 || precision > 18)
                retval << 18;
            else
                retval << precision;
            retval << "," << -scale << ")";
            return std2wx(retval.str());
        }
    }

    wxString names[] = {
        wxT("Char"),
        wxT("Float"),
        wxT("Double precision"),
        wxT("Timestamp"),
        wxT("Varchar"),
        wxT("Blob"),
        wxT("Date"),
        wxT("Time"),
        wxT("CSTRING")
    };
    short mapper[9] = { 14, 10, 27, 35, 37, 261, 12, 13, 40 };

    for (int i = 0; i < 9; ++i)
    {
        if (mapper[i] == datatype)
        {
            retval << wx2std(names[i]);
            break;
        }
    }

    // char, varchar & cstring, add (length)
    if (datatype == 14 || datatype == 37 || datatype == 40)
        retval << "(" << length << ")";

    if (datatype == 261)    // blob
        retval << " sub_type " << subtype;

    return std2wx(retval.str());
}
//-----------------------------------------------------------------------------
wxString Domain::getDefault()
{
    if (!infoLoadedM)
        loadInfo();
    return defaultM;
}
//-----------------------------------------------------------------------------
wxString Domain::getCollation()
{
    if (!infoLoadedM)
        loadInfo();
    return collationM;
}
//-----------------------------------------------------------------------------
wxString Domain::getCheckConstraint()
{
    if (!infoLoadedM)
        loadInfo();
    return checkM;
}
//-----------------------------------------------------------------------------
bool Domain::isNullable()
{
    if (!infoLoadedM)
        loadInfo();
    return !isNotNullM;
}
//-----------------------------------------------------------------------------
bool Domain::hasDefault()
{
    if (!infoLoadedM)
        loadInfo();
    return hasDefaultM;
}
//-----------------------------------------------------------------------------
void Domain::getDatatypeParts(wxString& type, wxString& size, wxString& scale)
{
    size = scale = wxEmptyString;
    wxString datatype = getDatatypeAsString();
    wxString::size_type p1 = datatype.find(wxT("("));
    if (p1 != wxString::npos)
    {
        type = datatype.substr(0, p1);
        wxString::size_type p2 = datatype.find(wxT(","));
        if (p2 == wxString::npos)
            p2 = datatype.find(wxT(")"));
        else
        {
            wxString::size_type p3 = datatype.find(wxT(")"));
            scale = datatype.substr(p2 + 1, p3 - p2 - 1);
        }
        size = datatype.substr(p1 + 1, p2 - p1 - 1);
    }
    else
    {
        type = datatype;
        // HACK ALERT: some better fix needed, but we don't want the subtype
        if (datatypeM == 261)
            type = wxT("Blob");
    }
}
//-----------------------------------------------------------------------------
wxString Domain::getCharset()
{
    if (!infoLoadedM)
        loadInfo();

    return charsetM;
}
//-----------------------------------------------------------------------------
wxString Domain::getPrintableName()
{
    wxString retval = getName_() + wxT(" ") + getDatatypeAsString();
    if (isNotNullM)
        retval += wxT(" not null");
    return retval;
}
//-----------------------------------------------------------------------------
wxString Domain::getAlterSqlTemplate() const
{
    return wxT("ALTER DOMAIN ") + getQuotedName() + wxT("\n")
        wxT("  SET DEFAULT { literal | NULL | USER }\n")
        wxT("  | DROP DEFAULT\n")
        wxT("  | ADD [CONSTRAINT] CHECK (condition)\n")
        wxT("  | DROP CONSTRAINT\n")
        wxT("  | new_name\n")
        wxT("  | TYPE new_datatype;\n");
}
//-----------------------------------------------------------------------------
wxString Domain::getCreateSqlTemplate() const
{
    return  wxT("CREATE DOMAIN domain_name\n")
            wxT("AS datatype [CHARACTER SET charset]\n")
            wxT("DEFAULT {literal | NULL | USER}\n")
            wxT("[NOT NULL]\n")
            wxT("[CHECK (dom_search_condition)]\n")
            wxT("COLLATE collation;\n");
}
//-----------------------------------------------------------------------------
const wxString Domain::getTypeName() const
{
    return wxT("DOMAIN");
}
//-----------------------------------------------------------------------------
void Domain::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$FIELDS ")
        wxT("where RDB$FIELD_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Domain::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$FIELDS set RDB$DESCRIPTION = ? ")
        wxT("where RDB$FIELD_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
void Domain::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDomain(*this);
}
//-----------------------------------------------------------------------------
