/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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

#include "frutils.h"
#include "database.h"
#include "dberror.h"
#include "domain.h"
#include "MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
Domain::Domain():
    MetadataItem()
{
    typeM = ntDomain;
    infoLoadedM = false;    // I had a 2 hour session with debugger to found out that this was missing
}
//-----------------------------------------------------------------------------
bool Domain::loadInfo()
{
    Database *d = getDatabase();
    if (!d)
    {
        //wxMessageBox(_("Domain::loadInfo, database = 0"), _("WARNING"), wxICON_WARNING|wxOK);
        return false;
    }
    IBPP::Database& db = d->getIBPPDatabase();

    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare(
            "select t.rdb$type, f.rdb$field_sub_type, f.rdb$field_length,"
            " f.rdb$field_precision, f.rdb$field_scale, c.rdb$character_set_name, f.rdb$character_length,"
            " f.rdb$null_flag, f.rdb$default_source, l.rdb$collation_name, f.rdb$validation_source "
            " from rdb$fields f"
            " join rdb$types t on f.rdb$field_type=t.rdb$type"
            " left outer join rdb$character_sets c on c.rdb$character_set_id = f.rdb$character_set_id"
            " left outer join rdb$collations l on l.rdb$collation_id = f.rdb$collation_id "
                " and l.rdb$character_set_id = f.rdb$character_set_id"
            " where f.rdb$field_name = ?"
            " and t.rdb$field_name='RDB$FIELD_TYPE'"
        );

        st1->Set(1, wx2std(getName_()));
        st1->Execute();
        if (!st1->Fetch())
        {
            //wxMessageBox(_("Domain not found."), _("Warning."), wxICON_WARNING|wxOK);
            return false;
        }
        st1->Get(1, &datatypeM);
        if (st1->IsNull(2))
            subtypeM = 0;
        else
            st1->Get(2, &subtypeM);
        st1->Get(3, &lengthM);
        if (!st1->IsNull(7))        // use rdb$character_length for character types
            st1->Get(7, &lengthM);
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
        if (st1->IsNull(10))
            collationM = wxEmptyString;
        else
        {
            std::string coll;
            st1->Get(10, coll);
            collationM = std2wx(coll).Strip();
        }
        readBlob(st1, 11, checkM);

        tr1->Commit();
        if (!isSystem())
            notifyObservers();
        infoLoadedM = true;
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }
    return false;
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

    // special case (mess that some tools (ex. IBExpert) make by only setting scale and not changing type)
    if (datatype == 27 && scale < 0)
    {
        retval << "Numeric(15," << -scale << ")";
        return std2wx(retval.str());
    }

    // LONG&INT64: INT/SMALLINT (prec=0), DECIMAL(sub_type=2), NUMERIC(sub_type=1)
    if (datatype == 7 || datatype == 8 || datatype == 16)
    {
        if (scale == 0)
        {
            if (datatype == 7)
                return wxT("Smallint");
            else if (datatype == 8)
                return wxT("Integer");
            else
                return wxT("Numeric(18,0)");
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

    if (datatype == 14 || datatype == 37 || datatype == 40) // char, varchar & cstring, add (length)
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
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
