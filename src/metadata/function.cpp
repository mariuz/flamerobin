/*
  Copyright (c) 2004-2014 The FlameRobin Development Team

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

// needed for platform independent EOL
#include <wx/textbuf.h>

#include <ibpp.h>

#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/function.h"
#include "metadata/MetadataItemVisitor.h"
#include "sql/StatementBuilder.h"

Function::Function(DatabasePtr database, const wxString& name)
    : MetadataItem(ntFunction, database.get(), name)
{
}

wxString Function::getCreateSql()
{
    ensurePropertiesLoaded();
    StatementBuilder sb;
    sb << kwDECLARE << ' ' << kwEXTERNAL << ' ' << kwFUNCTION << ' '
        << getQuotedName() << StatementBuilder::NewLine
        << paramListM  << StatementBuilder::NewLine
        << kwRETURNS << ' ' << retstrM << StatementBuilder::NewLine
        << kwENTRY_POINT << " '" << entryPointM << '\''
        << StatementBuilder::NewLine
        << kwMODULE_NAME << " '" << libraryNameM << "\';"
        << StatementBuilder::NewLine;
    return sb;
}

const wxString Function::getTypeName() const
{
    return "FUNCTION";
}

wxString Function::getDropSqlStatement() const
{
    StatementBuilder sb;
    sb << kwDROP << ' ' << kwEXTERNAL << ' ' << kwFUNCTION << ' '
        << getQuotedName() << ';';
    return sb;
}

wxString Function::getDefinition()
{
    ensurePropertiesLoaded();
    return definitionM;
}

wxString Function::getLibraryName()
{
    ensurePropertiesLoaded();
    return libraryNameM;
}

wxString Function::getEntryPoint()
{
    ensurePropertiesLoaded();
    return entryPointM;
}

void Function::loadProperties()
{
    setPropertiesLoaded(false);

    wxString mechanismNames[] = { "value", "reference",
        "descriptor", "blob descriptor", "scalar array",
        "null", wxEmptyString };
    wxString mechanismDDL[] = { " BY VALUE ", wxEmptyString,
        " BY DESCRIPTOR ", wxEmptyString, " BY SCALAR ARRAY ",
        " NULL ", wxEmptyString };

    bool first = true;
    wxString retstr;
    definitionM = getName_() + "(" + wxTextBuffer::GetEOL();
    paramListM = wxEmptyString;

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    wxMBConv* converter = db->getCharsetConverter();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "SELECT f.RDB$RETURN_ARGUMENT, a.RDB$MECHANISM,"
        " a.RDB$ARGUMENT_POSITION, a.RDB$FIELD_TYPE, a.RDB$FIELD_SCALE,"
        " a.RDB$FIELD_LENGTH, a.RDB$FIELD_SUB_TYPE, a.RDB$FIELD_PRECISION,"
        " f.RDB$MODULE_NAME, f.RDB$ENTRYPOINT, c.RDB$CHARACTER_SET_NAME "
        " FROM RDB$FUNCTIONS f"
        " LEFT OUTER JOIN RDB$FUNCTION_ARGUMENTS a"
        " ON f.RDB$FUNCTION_NAME = a.RDB$FUNCTION_NAME"
        " LEFT OUTER JOIN RDB$CHARACTER_SETS c"
        " ON a.RDB$CHARACTER_SET_ID = c.RDB$CHARACTER_SET_ID"
        " WHERE f.RDB$FUNCTION_NAME = ? "
        " ORDER BY a.RDB$ARGUMENT_POSITION"
    );
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    while (st1->Fetch())
    {
        short returnarg, mechanism, type, scale, length, subtype, precision,
            retpos;
        std::string libraryName, entryPoint, charset;
        st1->Get(1, returnarg);
        st1->Get(2, mechanism);
        st1->Get(3, retpos);
        st1->Get(4, type);
        st1->Get(5, scale);
        st1->Get(6, length);
        st1->Get(7, subtype);
        st1->Get(8, precision);
        st1->Get(9, libraryName);
        libraryNameM = std2wx(libraryName, converter).Strip();
        st1->Get(10, entryPoint);
        entryPointM = std2wx(entryPoint, converter).Strip();
        wxString datatype = Domain::dataTypeToString(type, scale,
            precision, subtype, length);
        if (!st1->IsNull(11))
        {
            st1->Get(11, charset);
            wxString chset(std2wx(charset, converter).Strip());
            if (db->getDatabaseCharset() != chset)
            {
                datatype += " " + SqlTokenizer::getKeyword(kwCHARACTER)
                    + " " + SqlTokenizer::getKeyword(kwSET)
                    + " " + chset;
            }
        }
        if (type == 261)    // avoid subtype information for BLOB
            datatype = SqlTokenizer::getKeyword(kwBLOB);

        int mechIndex = (mechanism < 0 ? -mechanism : mechanism);
        if (mechIndex >= (sizeof(mechanismNames)/sizeof(wxString)))
            mechIndex = (sizeof(mechanismNames)/sizeof(wxString)) - 1;
        wxString param = "    " + datatype + " "
            + SqlTokenizer::getKeyword(kwBY) + " "
            + mechanismNames[mechIndex];
        if (mechanism < 0)
            param += wxString(" ") + SqlTokenizer::getKeyword(kwFREE_IT);
        if (returnarg == retpos)    // output
        {
            retstr = param;
            retstrM = datatype + mechanismDDL[mechIndex];
            if (retpos != 0)
            {
                retstrM = SqlTokenizer::getKeyword(kwPARAMETER) + " ";
                retstrM << retpos;
                if (!paramListM.IsEmpty())
                    paramListM += ", ";
                paramListM += datatype + mechanismDDL[mechIndex];
            }
        }
        else
        {
            if (first)
                first = false;
            else
                definitionM += wxString(",") + wxTextBuffer::GetEOL();
            definitionM += param;
            if (!paramListM.empty())
                paramListM += ", ";
            paramListM += datatype + mechanismDDL[mechIndex];
        }
    }
    definitionM += wxString(wxTextBuffer::GetEOL()) + ")"
        + wxTextBuffer::GetEOL() + SqlTokenizer::getKeyword(kwRETURNS)
        + ":" + wxTextBuffer::GetEOL() + retstr;

    setPropertiesLoaded(true);
}

void Function::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitFunction(*this);
}

// Functions collection
Functions::Functions(DatabasePtr database)
    : MetadataCollection<Function>(ntFunctions, database, _("Functions"))
{
}

void Functions::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitFunctions(*this);
}

void Functions::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = "select rdb$function_name from rdb$functions"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " order by 1";
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}

void Functions::loadChildren()
{
    load(0);
}

const wxString Functions::getTypeName() const
{
    return "FUNCTION_COLLECTION";
}

