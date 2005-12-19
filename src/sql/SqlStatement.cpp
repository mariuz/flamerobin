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

  Contributor(s): Michael Hieke
*/

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

#include "metadata/database.h"
#include "SqlStatement.h"
//-----------------------------------------------------------------------------
// TOKEN LIST - a helper class
//-----------------------------------------------------------------------------
void TokenList::add(const SqlTokenType& item)
{
	tokensM.push_back(item);
}
//-----------------------------------------------------------------------------
const SqlTokenType& TokenList::operator[](const size_t& index) const
{
	static const SqlTokenType dummy = tkEOF;
	if (index >= tokensM.size())
		return dummy;
	else
		return tokensM[index];
}
//-----------------------------------------------------------------------------
size_t TokenList::size() const
{
	return tokensM.size();
}
//-----------------------------------------------------------------------------
// STATEMENT
//-----------------------------------------------------------------------------
SqlStatement::SqlStatement(const wxString& sql, Database *db)
	:actionM(actNONE), objectTypeM(ntUnknown), databaseM(db), objectM(0),
	 identifierTokenIndexM(0), isAlterColumnM(false), isDatatypeM(false)
{
    // use the tokenizer to split the statements into a vector of tokens
    // also keep the token strings for identifiers and strings
    SqlTokenizer tokenizer(sql);

    // first get the tokens up to the first identifier
    SqlTokenType stt;
    while (true)
    {
        stt = tokenizer.getCurrentToken();
        if (stt == tkEOF)
            return;// true;
        if (stt != tkCOMMENT && stt != tkWHITESPACE)
        {
            tokensM.add(stt);
            if (stt == tkIDENTIFIER || stt == tkSTRING)
            {
                wxString ts(tokenizer.getCurrentTokenString());
                tokenStringsM[tokensM.size() - 1] = ts;
                if (stt == tkIDENTIFIER)
                {
                    nameM.setFromSql(ts);
                    tokenizer.nextToken();
                    break;
                }
            }
        }
        tokenizer.nextToken();
    }

    // needs at least action
    if (tokensM.size() < 1)
        return; // true;

    identifierTokenIndexM = tokensM.size() - 1;
    size_t typeTokenIndex = 1;

    // get action
    switch (tokensM[0])
    {
        case kwALTER:
            actionM = actALTER; break;
        case kwCREATE:
            actionM = actCREATE; break;
        case kwDECLARE:
            actionM = actDECLARE; break;
        case kwDROP:
            actionM = actDROP; break;
        case kwRECREATE:
            actionM = actRECREATE; break;
        case kwSET:
            actionM = actSET; break;
        case kwUPDATE:
            // it's the only statement we care for which has implicit type
            actionM = actUPDATE; objectTypeM = ntTable; break;
        default:
            return; // true;
    }
    // special handling for "CREATE OR ALTER"
    if (actionM == actCREATE && tokensM[1] == kwOR && tokensM[2] == kwALTER)
    {
        actionM = actCREATE_OR_ALTER;
        typeTokenIndex = 3;
    }

    // get object type
    while (objectTypeM == ntUnknown && typeTokenIndex < tokensM.size())
    {
        switch (tokensM[typeTokenIndex])
        {
            case kwDATABASE:
                objectTypeM = ntDatabase; break;
            case kwDOMAIN:
                objectTypeM = ntDomain; break;
            case kwEXCEPTION:
                objectTypeM = ntException; break;
            case kwFUNCTION:
                objectTypeM = ntFunction; break;
            case kwGENERATOR:
                objectTypeM = ntGenerator; break;
            case kwINDEX:
                objectTypeM = ntIndex; break;
            case kwPROCEDURE:
                objectTypeM = ntProcedure; break;
            case kwROLE:
                objectTypeM = ntRole; break;
            case kwTABLE:
                objectTypeM = ntTable; break;
            case kwTRIGGER:
                objectTypeM = ntTrigger; break;
            case kwVIEW:
                objectTypeM = ntView; break;
            default:
                // this will scan over things like "EXTERNAL", "UNIQUE",
                // "ASCENDING", "STATISTICS" etc., until object type is found
                typeTokenIndex++;
                break;
        }
    }
    if (objectTypeM == ntUnknown || !databaseM)
        return; // false;

    objectM = databaseM->findByNameAndType(objectTypeM, nameM.get());

    // map "CREATE OR ALTER" and "RECREATE" to correct action
    if (actionM == actCREATE_OR_ALTER || actionM == actRECREATE)
    	actionM = (objectM ? actALTER : actCREATE);

	// -------------- STEP 2 ------------------------------------------------
	// if we decide to have a two-step evaluation, this is the breaking point


    // get remaining tokens, and token content for identifiers + strings
    while (tkEOF != (stt = tokenizer.getCurrentToken()))
    {
        if (stt != tkCOMMENT && stt != tkWHITESPACE)
        {
            tokensM.add(stt);
            if (stt == tkIDENTIFIER || stt == tkSTRING)
            {
                wxString ts(tokenizer.getCurrentTokenString());
                tokenStringsM[tokensM.size() - 1] = ts;
            }
        }
        tokenizer.nextToken();
    }

    // check for "UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG"
    // convert this change in NULL flag to "ALTER TABLE" and act accordingly
    if (actionM == actUPDATE  && nameM.equals(wxT("RDB$RELATION_FIELDS"))
        && tokensM[2] == kwSET && tokensM[3] == tkIDENTIFIER)
    {
        Identifier id;
        id.setFromSql(tokenStringsM[3]);
        if (!id.equals(wxT("RDB$NULL_FLAG")))
            return; // true;

        actionM = actALTER;
        objectTypeM = ntTable;
        objectM = 0;
        // find "RDB$RELATION_NAME" in map
        for (std::map<int, wxString>::const_iterator mit = tokenStringsM.begin();
            mit != tokenStringsM.end(); mit++)
        {
            if ((*mit).second.CmpNoCase(wxT("RDB$RELATION_NAME")) == 0)
            {
                size_t i = (*mit).first;
                if (tokensM[i + 1] == tkEQUALS && tokensM[i + 2] == tkSTRING)
                {
                    nameM.setFromSql(tokenStringsM[i + 2]);
                    objectM = databaseM->findByNameAndType(ntTable, nameM.get());
                    break;
                }
            }
        }
        if (!objectM)
            return; // true;
    }

	if (actionM == actALTER)	// check for alter column
    {
        // handle "ALTER TABLE xyz ALTER [COLUMN] fgh TYPE {domain or datatype}
        if (objectTypeM == ntTable && tokensM[identifierTokenIndexM + 1] == kwALTER)
        {
            size_t fieldNameIndex = identifierTokenIndexM + 2;
            if (tokensM[fieldNameIndex] == kwCOLUMN)
                fieldNameIndex++;
            if (tokensM[fieldNameIndex] == tkIDENTIFIER && tokensM[fieldNameIndex + 1] == kwTYPE)
            {
				isAlterColumnM = true;
                fieldNameM.setFromSql(tokenStringsM[fieldNameIndex]);

                stt = tokensM[fieldNameIndex + 2];
                isDatatypeM = (stt == kwCHAR || stt == kwVARCHAR
                    || stt == kwINTEGER || stt == kwSMALLINT
                    || stt == kwDECIMAL || stt == kwNUMERIC
                    || stt == kwDATE || stt == kwTIME || stt == kwTIMESTAMP
                    || stt == kwFLOAT || stt == kwBLOB)
                    || (stt == kwDOUBLE && tokensM[fieldNameIndex + 3] == kwPRECISION);
            }
        }
	}
}
//-----------------------------------------------------------------------------
Relation* SqlStatement::getCreateTriggerRelation()
{
	if (objectTypeM == ntTrigger && databaseM
        && tokensM[identifierTokenIndexM + 1] == kwFOR
        && tokensM[identifierTokenIndexM + 2] == tkIDENTIFIER)
    {
        Identifier id;
        id.setFromSql(tokenStringsM[identifierTokenIndexM + 2]);
        return databaseM->findRelation(id);
    }
    return 0;
}
//-----------------------------------------------------------------------------
bool SqlStatement::isDDL() const
{
	return (objectTypeM != ntUnknown && actionM != actNONE);
}
//-----------------------------------------------------------------------------
bool SqlStatement::isAlterColumn() const
{
	return isAlterColumnM;
}
//-----------------------------------------------------------------------------
bool SqlStatement::isDatatype() const
{
	return isDatatypeM;
}
//-----------------------------------------------------------------------------
MetadataItem* SqlStatement::getObject()
{
	return objectM;
}
//-----------------------------------------------------------------------------
NodeType SqlStatement::getObjectType() const
{
	return objectTypeM;
}
//-----------------------------------------------------------------------------
Identifier SqlStatement::getIdentifier()
{
	return nameM;
}
//-----------------------------------------------------------------------------
wxString SqlStatement::getName()
{
	return nameM.get();
}
//-----------------------------------------------------------------------------
wxString SqlStatement::getFieldName()
{
	return fieldNameM.get();
}
//-----------------------------------------------------------------------------
SqlAction SqlStatement::getAction() const
{
	return actionM;
}
//-----------------------------------------------------------------------------
bool SqlStatement::actionIs(const SqlAction& act, NodeType nt) const
{
	if (nt == ntUnknown)
		return actionM == act;
	else
		return (actionM == act && objectTypeM == nt);
}
//-----------------------------------------------------------------------------
