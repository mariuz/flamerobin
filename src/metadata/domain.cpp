/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/MetadataItemVisitor.h"
#include "sql/SqlTokenizer.h"
//-----------------------------------------------------------------------------
Domain::Domain(DatabasePtr database, const wxString& name)
    : MetadataItem((hasSystemPrefix(name) ? ntSysDomain : ntDomain),
        database.get(), name)
{
}
//-----------------------------------------------------------------------------
void Domain::loadProperties()
{
    setPropertiesLoaded(false);

    Database* d = getDatabase(wxT("Domain::loadProperties"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = d->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        "select f.rdb$field_type, f.rdb$field_sub_type, f.rdb$field_length,"
        " f.rdb$field_precision, f.rdb$field_scale, c.rdb$character_set_name, "
        " f.rdb$character_length, f.rdb$null_flag, f.rdb$default_source, "
        " l.rdb$collation_name, f.rdb$validation_source, f.rdb$computed_blr, "
        " c.rdb$bytes_per_character "
        " from rdb$fields f"
        " left outer join rdb$character_sets c "
            " on c.rdb$character_set_id = f.rdb$character_set_id"
        " left outer join rdb$collations l "
            " on l.rdb$collation_id = f.rdb$collation_id "
            " and l.rdb$character_set_id = f.rdb$character_set_id"
        " where f.rdb$field_name = ?"
    );

    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    if (!st1->Fetch())
        throw FRError(_("Domain not found: ") + getName_());
    st1->Get(1, &datatypeM);
    if (st1->IsNull(2))
        subtypeM = 0;
    else
        st1->Get(2, &subtypeM);

    // determine the (var)char field length
    // - system tables use field_len and char_len is null
    // - computed columns have field_len/bytes_per_char, char_len is 0
    // - view columns have field_len/bytes_per_char, char_len is null
    // - regular table columns and SP params have field_len/bytes_per_char
    //   they also have proper char_len, but we don't use it now
    st1->Get(3, &lengthM);
    int bpc = 0;   // bytes per char
    if (!st1->IsNull(13))
        st1->Get(13, &bpc);
    if (bpc && (!st1->IsNull(7) || !st1->IsNull(12)))
        lengthM /= bpc;

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
        std::string s;
        st1->Get(6, s);
        charsetM = std2wxIdentifier(s, converter);
    }
    isNotNullM = !st1->IsNull(8);

    hasDefaultM = !st1->IsNull(9);
    if (hasDefaultM)
    {
        readBlob(st1, 9, defaultM, converter);

        // Some users reported two spaces before DEFAULT word in source
        // Also, equals sign is also allowed in newer FB versions
        // Trim(false) is trim-left
        if (defaultM.Trim(false).Upper().StartsWith(wxT("DEFAULT")))
            defaultM.Remove(0, 8);
        else if (defaultM.StartsWith(wxT("="))) // "=" or "= "
            defaultM.Remove(0, 1).Trim(false);
    }

    if (st1->IsNull(10))
        collationM = wxEmptyString;
    else
    {
        std::string s;
        st1->Get(10, s);
        collationM = std2wxIdentifier(s, converter);
    }
    readBlob(st1, 11, checkM, converter);

    setPropertiesLoaded(true);
}
//-----------------------------------------------------------------------------
bool Domain::isString()
{
    ensurePropertiesLoaded();
    return (datatypeM == 14 || datatypeM == 10 || datatypeM == 37);
}
//-----------------------------------------------------------------------------
bool Domain::isSystem() const
{
    wxString prefix(getName_().substr(0, 4));
    if (prefix == wxT("MON$"))
        return true;
    if (prefix != wxT("RDB$"))
        return false;
    long l;
    return getName_().Mid(4).ToLong(&l);    // numeric = system
}
//-----------------------------------------------------------------------------
//! returns column's datatype as human readable wxString.
wxString Domain::getDatatypeAsString()
{
    ensurePropertiesLoaded();
    return dataTypeToString(datatypeM, scaleM, precisionM, subtypeM, lengthM);
}
//-----------------------------------------------------------------------------
/* static*/
wxString Domain::dataTypeToString(short datatype, short scale, short precision,
    short subtype, short length)
{
    wxString retval;

    // special case (mess that some tools (ex. IBExpert) make by only
    // setting scale and not changing type)
    if (datatype == 27 && scale < 0)
    {
        retval = SqlTokenizer::getKeyword(kwNUMERIC);
        retval << wxT("(15,") << -scale << wxT(")");
        return retval;
    }

    // INTEGER(prec=0), DECIMAL(sub_type=2), NUMERIC(sub_t=1), BIGINT(sub_t=0)
    if (datatype == 7 || datatype == 8 || datatype == 16)
    {
        if (scale == 0)
        {
            if (datatype == 7)
                return SqlTokenizer::getKeyword(kwSMALLINT);
            if (datatype == 8)
                return SqlTokenizer::getKeyword(kwINTEGER);
        }

        if (scale == 0 && subtype == 0)
            return SqlTokenizer::getKeyword(kwBIGINT);

        retval = SqlTokenizer::getKeyword(
            (subtype == 2) ? kwDECIMAL : kwNUMERIC);
        retval << wxT("(");
        if (precision <= 0 || precision > 18)
            retval << 18;
        else
            retval << precision;
        retval << wxT(",") << -scale << wxT(")");
        return retval;
    }

    switch (datatype)
    {
        case 10:
            return SqlTokenizer::getKeyword(kwFLOAT);
        case 27:
            return SqlTokenizer::getKeyword(kwDOUBLE) + wxT(" ")
                + SqlTokenizer::getKeyword(kwPRECISION);

        case 12:
            return SqlTokenizer::getKeyword(kwDATE);
        case 13:
            return SqlTokenizer::getKeyword(kwTIME);
        case 35:
            return SqlTokenizer::getKeyword(kwTIMESTAMP);

        // add subtype for blob
        case 261:
            retval = SqlTokenizer::getKeyword(kwBLOB) + wxT(" ")
                + SqlTokenizer::getKeyword(kwSUB_TYPE) + wxT(" ");
            retval << subtype;
            return retval;

        // add length for char, varchar and cstring
        case 14:
            retval = SqlTokenizer::getKeyword(kwCHAR);
            break;
        case 37:
            retval = SqlTokenizer::getKeyword(kwVARCHAR);
            break;
        case 40:
            retval = SqlTokenizer::getKeyword(kwCSTRING);
            break;
    }
    retval << wxT("(") << length << wxT(")");
    return retval;
}
//-----------------------------------------------------------------------------
wxString Domain::getDefault()
{
    ensurePropertiesLoaded();
    return defaultM;
}
//-----------------------------------------------------------------------------
wxString Domain::getCollation()
{
    ensurePropertiesLoaded();
    return collationM;
}
//-----------------------------------------------------------------------------
wxString Domain::getCheckConstraint()
{
    ensurePropertiesLoaded();
    return checkM;
}
//-----------------------------------------------------------------------------
bool Domain::isNullable()
{
    ensurePropertiesLoaded();
    return !isNotNullM;
}
//-----------------------------------------------------------------------------
bool Domain::hasDefault()
{
    ensurePropertiesLoaded();
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
            type = SqlTokenizer::getKeyword(kwBLOB);
    }
}
//-----------------------------------------------------------------------------
wxString Domain::getCharset()
{
    ensurePropertiesLoaded();
    return charsetM;
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
const wxString Domain::getTypeName() const
{
    return wxT("DOMAIN");
}
//-----------------------------------------------------------------------------
void Domain::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDomain(*this);
}
//-----------------------------------------------------------------------------
// Domains collection
Domains::Domains(DatabasePtr database)
    : MetadataCollection<Domain>(ntDomains, database, _("Domains"))
{
}
//-----------------------------------------------------------------------------
void Domains::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDomains(*this);
}
//-----------------------------------------------------------------------------
void Domains::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = wxT("select f.rdb$field_name from rdb$fields f")
        wxT(" left outer join rdb$types t on f.rdb$field_type=t.rdb$type")
        wxT(" where t.rdb$field_name='RDB$FIELD_TYPE'")
        wxT(" and f.rdb$field_name not starting with 'RDB$'")
        wxT(" order by 1");
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}
//-----------------------------------------------------------------------------
void Domains::loadChildren()
{
    load(0);
}
//-----------------------------------------------------------------------------
// System domains collection
SysDomains::SysDomains(DatabasePtr database)
    : MetadataCollection<Domain>(ntSysDomains, database, _("System domains"))
{
}
//-----------------------------------------------------------------------------
void SysDomains::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitSysDomains(*this);
}
//-----------------------------------------------------------------------------
