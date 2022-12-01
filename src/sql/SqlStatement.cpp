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

#include "metadata/database.h"
#include "SqlStatement.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "metadata/relation.h"
#include "sql/SqlStatement.h"


// TOKEN LIST - a helper class

void TokenList::add(const SqlTokenType& item)
{
    tokensM.push_back(item);
}

const SqlTokenType& TokenList::operator[](const size_t& index) const
{
    static const SqlTokenType dummy = tkEOF;
    if (index >= tokensM.size())
        return dummy;
    else
        return tokensM[index];
}

size_t TokenList::size() const
{
    return tokensM.size();
}

// STATEMENT

SqlStatement::SqlStatement(const wxString& sql, Database *db, const wxString&
    terminator)
    :actionM(actNONE), objectTypeM(ntUnknown), databaseM(db), objectM(0),
     identifierTokenIndexM(0), isAlterColumnM(false), isDatatypeM(false),
     terminatorM(terminator), statementM(sql)
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
            break;
        if (stt != tkCOMMENT && stt != tkWHITESPACE)
        {
            tokensM.add(stt);
            wxString ts(tokenizer.getCurrentTokenString());
            tokenStringsM[tokensM.size() - 1] = ts;
            if (stt == tkIDENTIFIER)
            {
                nameM.setFromSql(ts);
                tokenizer.jumpToken(false);
                // break here since we don't want name to be overwritten
                if (tokensM[0] != kwGRANT && tokensM[0] != kwREVOKE
                    && tokensM[0] != kwCOMMENT)
                {
                    break;
                }
            }
        }
        tokenizer.jumpToken(false);
    }

    // needs at least action
    if (tokensM.size() < 1)
        return; // true;

    if (nameM.get().IsEmpty()) // non-reserved keyword used as identifier?
    {
        nameM.setFromSql(tokenStringsM[2]); // we take a lucky guess
        identifierTokenIndexM = 2;          // ex.: CREATE DOMAIN CASCADE
    }
    else
        identifierTokenIndexM = tokensM.size() - 1;

    size_t typeTokenIndex = 1;

    // get action
    switch (tokensM[0])
    {
        case kwALTER:
            actionM = actALTER; 
            break;
        case kwCOMMENT:
            actionM = actCOMMENT; 
            break;
        case kwCREATE:
            actionM = actCREATE; 
            break;
        case kwDECLARE:
            actionM = actDECLARE; 
            break;
        case kwDROP:
            actionM = actDROP; 
            break;
        case kwGRANT:
        case kwREVOKE:
            actionM = actGRANT; 
            break;
        case kwRECREATE:
            actionM = actRECREATE; 
            break;
        case kwSET:
            actionM = actSET; 
            break;
        case kwUPDATE:
            // it's the only statement we care for which has implicit type
            actionM = actUPDATE; 
            objectTypeM = ntTable; 
            break;
        default:
            return; // true;
    }
    // special handling for "CREATE OR ALTER"
    if (actionM == actCREATE && tokensM[1] == kwOR && tokensM[2] == kwALTER)
    {
        actionM = actCREATE_OR_ALTER;
        typeTokenIndex = 3;
        if (identifierTokenIndexM == 2)
        {
            identifierTokenIndexM = 4;
            nameM.setFromSql(tokenStringsM[4]);
        }
    }

    // GRANT blah, blah blah ON [PROCEDURE] object_name TO ...
    // REVOKE blah, blah, .. ON [PROCEDURE] object_name FROM ...
    if (actionM == actGRANT)
    {
        size_t idx = 1;
        while (idx < tokensM.size())
        {
            if (tokensM[idx++] == kwON)
            {
                if (tokensM[idx] == kwPROCEDURE)
                {
                    idx++;
                    objectTypeM = ntProcedure;
                }
                nameM.setFromSql(tokenStringsM[idx]);
                if (objectTypeM == ntProcedure)
                {
                    objectM = databaseM->findByNameAndType(ntProcedure,
                        nameM.get());
                    if (!objectM)
                        objectTypeM = ntUnknown;
                }
                else if (objectTypeM == ntUnknown)   // find relation
                {
                    if (!databaseM)
                        return;
                    objectM = databaseM->findRelation(nameM);
                    if (objectM)
                        objectTypeM = objectM->getType();
                }
                return;
            }
        }
        // GRANT role_name TO ...
        nameM.setFromSql(tokenStringsM[1]);
        objectM = databaseM->findByNameAndType(ntRole, nameM.get());
        if (objectM)
            objectTypeM = ntRole;
        return;
    }

    // COMMENT ON COLUMN table.column IS
    // COMMENT ON PARAMETER procedure.parameter IS
    if (actionM == actCOMMENT && tokensM[1] == kwON)
    {
        Identifier parent(tokenStringsM[3]);
        Identifier child(tokenStringsM[4]);
        if (tokensM[2] == kwCOLUMN)
        {
            if (Relation* r = databaseM->findRelation(parent))
            {
                r->ensureChildrenLoaded();
                if (ColumnPtr c = r->findColumn(child.get()))
                {
                    objectTypeM = ntColumn;
                    objectM = c.get();
                    return;
                }
            }
        }
        if (tokensM[2] == kwPARAMETER)
        {
            Procedure* p = dynamic_cast<Procedure *>(
                databaseM->findByNameAndType(ntProcedure, parent.get()));
            if (p)
            {
                p->ensureChildrenLoaded();
                if (ParameterPtr par = p->findParameter(child.get()))
                {
                    objectTypeM = ntParameter;
                    objectM = par.get();
                    return;
                }
            }
        }
    }

    // get object type
    while (objectTypeM == ntUnknown && typeTokenIndex < tokensM.size())
    {
        switch (tokensM[typeTokenIndex])
        {
        case kwDATABASE:
            objectTypeM = ntDatabase;
            break;
        case kwDOMAIN:
            objectTypeM = ntDomain;
            break;
        case kwEXCEPTION:
            objectTypeM = ntException;
            break;
        case kwEXTERNAL:
            objectTypeM = ntUDF;
            break;
        case kwFUNCTION:
            if (actionM == actCREATE || actionM == actALTER ||
                actionM == actCREATE_OR_ALTER) {
                if (tokensM[0] == kwDECLARE)
                    objectTypeM = ntUDF;
                else
                    objectTypeM = ntFunctionSQL;
            }else{ 
                    objectTypeM = ntFunctionSQL;
                    if (databaseM->findByNameAndType(ntUDF, nameM.get())) 
                        objectTypeM = ntUDF;
            }
            break;
        case kwGENERATOR:
            objectTypeM = ntGenerator;
            break;
        case kwINDEX:
            objectTypeM = ntIndex;
            break;
        case kwPROCEDURE:
            objectTypeM = ntProcedure;
            break;
        case kwROLE:
            objectTypeM = ntRole;
            break;
        case kwTABLE:
            objectTypeM = ntTable;
            break;
        case kwTRIGGER:
            if (actionM == actCREATE || actionM == actALTER || 
                actionM == actCREATE_OR_ALTER) {
                stt = tokenizer.getCurrentToken();
                if (stt == kwACTIVE || stt == kwINACTIVE || 
                    stt == kwBEFORE || stt == kwAFTER ||
                    stt == kwON) {
                    if (stt == kwACTIVE || stt == kwINACTIVE) {
                        tokenizer.jumpToken(false);
                        stt = tokenizer.getCurrentToken();
                        objectTypeM = ntDMLTrigger;
                        if (databaseM->findByNameAndType(ntDBTrigger, nameM.get())) {
                            objectTypeM = ntDBTrigger;
                        }
                        if (databaseM->findByNameAndType(ntDDLTrigger, nameM.get())) {
                            objectTypeM = ntDDLTrigger;
                        }
                    }
                    else {
                        if (stt == kwON) { // DB Trigger
                            objectTypeM = ntDBTrigger;
                        }
                        else {
                            tokenizer.jumpToken(false);
                            stt = tokenizer.getCurrentToken();
                            if (stt == kwINSERT || stt == kwUPDATE ||
                                stt == kwDELETE) { // SQL 2003 DML Trigger
                                objectTypeM = ntDMLTrigger;
                            }
                            else
                                objectTypeM = ntDDLTrigger;
                        }
                    }
                }
                else // Legacy DML Trigger
                    objectTypeM = ntDMLTrigger;
            }
            else {
                objectTypeM = ntDMLTrigger;
                if (actionM == actDROP) {
                    if (databaseM->findByNameAndType(ntDBTrigger, nameM.get())) {
                        objectTypeM = ntDBTrigger;
                    }
                    if (databaseM->findByNameAndType(ntDDLTrigger, nameM.get())) {
                        objectTypeM = ntDDLTrigger;
                    }
                }
            }
            break;
        case kwVIEW:
            objectTypeM = ntView;
            break;
        case kwPACKAGE:
            objectTypeM = ntPackage;
            break;
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
    if (actionM == actUPDATE  && nameM.equals("RDB$RELATION_FIELDS")
        && tokensM[2] == kwSET && tokensM[3] == tkIDENTIFIER)
    {
        Identifier id;
        id.setFromSql(tokenStringsM[3]);
        if (!id.equals("RDB$NULL_FLAG"))
            return; // true;

        actionM = actALTER;
        objectTypeM = ntTable;
        objectM = 0;
        // find "RDB$RELATION_NAME" in map
        for (std::map<int, wxString>::const_iterator mit = tokenStringsM.begin();
            mit != tokenStringsM.end(); mit++)
        {
            if ((*mit).second.CmpNoCase("RDB$RELATION_NAME") == 0)
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

    if (actionM == actALTER)    // check for alter column
    {
        // handle "ALTER TABLE xyz ALTER [COLUMN] fgh TYPE {domain or datatype}
        if (objectTypeM == ntTable && tokensM[identifierTokenIndexM + 1] == kwALTER)
        {
            size_t fieldNameIndex = identifierTokenIndexM + 2;
            if (tokensM[fieldNameIndex] == kwCOLUMN)
                fieldNameIndex++;
            if (tokensM[fieldNameIndex + 1] == kwTYPE)
            {
                isAlterColumnM = true;
                fieldNameM.setFromSql(tokenStringsM[fieldNameIndex]);

                stt = tokensM[fieldNameIndex + 2];
                isDatatypeM = (stt == kwCHAR || stt == kwVARCHAR ||stt == kwBOOLEAN
                    || stt == kwINTEGER || stt == kwSMALLINT || stt == kwBIGINT
                    || stt == kwDECIMAL || stt == kwNUMERIC
                    || stt == kwDATE || stt == kwTIME || stt == kwTIMESTAMP
                    || stt == kwFLOAT || stt == kwBLOB)
                    || (stt == kwDOUBLE && tokensM[fieldNameIndex + 3] == kwPRECISION);
            }
        }
    }
}

wxString SqlStatement::getStatement() const
{
    return statementM;
}

wxString SqlStatement::getTerminator() const
{
    return terminatorM;
}

Relation* SqlStatement::getCreateTriggerRelation() const
{
    if (objectTypeM == ntDMLTrigger && databaseM
        && tokensM[identifierTokenIndexM + 1] == kwFOR
        && tokensM[identifierTokenIndexM + 2] == tkIDENTIFIER)
    {
        Identifier id;
        std::map<int, wxString>::const_iterator ci =
            tokenStringsM.find(identifierTokenIndexM+2);
        id.setFromSql((*ci).second);
        return databaseM->findRelation(id);
    }
    return 0;
}

bool SqlStatement::isDDL() const
{
    // actUPDATE means that we did have the UPDATE statment, but it didn't
    // convert to actALTER (i.e. it's a regular update statement)
    return (objectTypeM != ntUnknown && actionM != actNONE
        && actionM != actUPDATE);
}

bool SqlStatement::isAlterColumn() const
{
    return isAlterColumnM;
}

bool SqlStatement::isDatatype() const
{
    return isDatatypeM;
}

MetadataItem* SqlStatement::getObject() const
{
    return objectM;
}

NodeType SqlStatement::getObjectType() const
{
    return objectTypeM;
}

Identifier SqlStatement::getIdentifier() const
{
    return nameM;
}

wxString SqlStatement::getName() const
{
    return nameM.get();
}

wxString SqlStatement::getFieldName() const
{
    return fieldNameM.get();
}

SqlAction SqlStatement::getAction() const
{
    return actionM;
}

bool SqlStatement::actionIs(const SqlAction& act, NodeType nt) const
{
    if (nt == ntUnknown)
        return actionM == act;
    else
        return (actionM == act && objectTypeM == nt);
}

