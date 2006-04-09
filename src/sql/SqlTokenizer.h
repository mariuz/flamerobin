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

#ifndef FR_SQLTOKENIZER_H
#define FR_SQLTOKENIZER_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
//-----------------------------------------------------------------------------
enum SqlTokenType {
    /*
     * these are token types used by SqlTokenizer class
     */
    tkEOF, tkUNKNOWN, tkWHITESPACE, tkCOMMENT,
    tkTERM, tkPARENOPEN, tkPARENCLOSE, tkEQUALS,
    tkSTRING, tkIDENTIFIER,

    tk_KEYWORDS_START_HERE,
    /*
     * SQL keywords start here, these are returned instead of tkIDENTIFIER
     */
    kwABS, kwACTION, kwACTIVE, kwADD, kwADMIN, kwAFTER, kwALL, kwALTER,
    kwAND, kwANY, kwAS, /*kwASC,*/ kwASCENDING, kwAT, kwAUTO, kwAUTODDL, kwAVG,

    kwBASED, kwBASENAME, kwBASE_NAME, kwBEFORE, kwBEGIN, kwBETWEEN,
    kwBIGINT, kwBLOB, kwBLOBEDIT, kwBOOLEAN, kwBOTH, kwBREAK, kwBUFFER,
    kwBY,

    kwCACHE, kwCASCADE, kwCASE, kwCAST, kwCHAR, kwCHARACTER,
    kwCHARACTER_LENGTH, kwCHAR_LENGTH, kwCHECK, kwCHECK_POINT_LEN,
    kwCHECK_POINT_LENGTH, kwCLOSE, kwCOALESCE, kwCOLLATE, kwCOLLATION,
    kwCOLUMN, kwCOMMIT, kwCOMMITTED, kwCOMPILETIME, kwCOMPUTED,
    kwCONDITIONAL, kwCONNECT, kwCONSTRAINT, kwCONTAINING, kwCONTINUE,
    kwCOUNT, kwCREATE, kwCSTRING, kwCURRENT, kwCURRENT_CONNECTION,
    kwCURRENT_DATE, kwCURRENT_ROLE, kwCURRENT_TIME, kwCURRENT_TIMESTAMP,
    kwCURRENT_TRANSACTION, kwCURRENT_USER, kwCURSOR,

    kwDATABASE, kwDATE, kwDAY, kwDB_KEY, kwDEBUG, kwDEC, kwDECIMAL,
    kwDECLARE, kwDEFAULT, kwDELETE, kwDELETING, /*kwDESC,*/ kwDESCENDING,
    kwDESCRIBE, kwDESCRIPTOR, kwDISCONNECT, kwDISPLAY, kwDISTINCT, kwDO,
    kwDOMAIN, kwDOUBLE, kwDROP,

    kwECHO, kwEDIT, kwELSE, kwEND, kwENTRY_POINT, kwESCAPE, kwEVENT,
    kwEXCEPTION, kwEXECUTE, kwEXISTS, kwEXIT, kwEXTERN, kwEXTERNAL,
    kwEXTRACT,

    kwFALSE, kwFETCH, kwFILE, kwFILTER, kwFIRST, kwFLOAT, kwFOR,
    kwFOREIGN, kwFOUND, kwFREE_IT, kwFROM, kwFULL, kwFUNCTION,

    kwGDSCODE, kwGENERATOR, kwGEN_ID, kwGLOBAL, kwGOTO, kwGRANT, kwGROUP,
    kwGROUP_COMMIT_, kwGROUP_COMMIT_WAIT,

    kwHAVING, kwHELP, kwHOUR,

    kwIF, kwIIF, kwIMMEDIATE, kwIN, kwINACTIVE, kwINDEX, kwINDICATOR,
    kwINIT, kwINNER, kwINPUT, kwINPUT_TYPE, kwINSERT, kwINSERTING, kwINT,
    kwINTEGER, kwINTO, kwIS, kwISOLATION, kwISQL,

    kwJOIN,
    kwKEY,

    kwLAST, kwLC_MESSAGES, kwLC_TYPE, kwLEADING, kwLEAVE, kwLEFT, kwLENGTH,
    kwLEV, kwLEVEL, kwLIKE, kwLOCK, kwLOGFILE, kwLOG_BUFFER_SIZE,
    kwLOG_BUF_SIZE, kwLONG,

    kwMANUAL, kwMAX, kwMAXIMUM, kwMAXIMUM_SEGMENT, kwMAX_SEGMENT, kwMERGE,
    kwMESSAGE, kwMIN, kwMINIMUM, kwMINUTE, kwMODULE_NAME, kwMONTH,

    kwNAMES, kwNATIONAL, kwNATURAL, kwNCHAR, kwNO, kwNOAUTO, kwNOT, kwNULL,
    kwNULLIF, kwNULLS, kwNUMERIC, kwNUM_LOG_BUFFERS, kwNUM_LOG_BUFS,

    kwOCTET_LENGTH, kwOF, kwON, kwONLY, kwOPEN, kwOPTION, kwOR, kwORDER,
    kwOUTER, kwOUTPUT, kwOUTPUT_TYPE, kwOVERFLOW,

    kwPAGE, kwPAGELENGTH, kwPAGES, kwPAGE_SIZE, kwPARAMETER, kwPASSWORD,
    kwPERCENT, kwPLAN, kwPOSITION, kwPOST_EVENT, kwPRECISION, kwPREPARE,
    kwPRESERVE, kwPRIMARY, kwPRIVILEGES, kwPROCEDURE, kwPROTECTED, kwPUBLIC,

    kwQUIT,

    kwRAW_PARTITIONS, kwREAD, kwREAL, kwRECORD_VERSION, kwRECREATE,
    kwREFERENCES, kwRELEASE, kwRESERV, kwRESERVING, kwRESTRICT, kwRETAIN,
    kwRETURN, kwRETURNING_VALUES, kwRETURNS, kwREVOKE, kwRIGHT, kwROLE,
    kwROLLBACK, kwROWS, kwROW_COUNT, kwRUNTIME,

    kwSAVEPOINT, kwSCHEMA, kwSECOND, kwSEGMENT, kwSELECT, kwSET, kwSHADOW,
    kwSHARED, kwSHELL, kwSHOW, kwSINGULAR, kwSIZE, kwSKIP, kwSMALLINT,
    kwSNAPSHOT, kwSOME, kwSORT, kwSQLCODE, kwSQLERROR, kwSQLWARNING,
    kwSTABILITY, kwSTARTING, kwSTARTS, kwSTATEMENT, kwSTATIC, kwSTATISTICS,
    kwSUBSTRING, kwSUB_TYPE, kwSUM, kwSUSPEND,

    kwTABLE, kwTEMPORARY, kwTERMINATOR, kwTHEN, kwTIES, kwTIME, kwTIMESTAMP,
    kwTO, kwTRAILING, kwTRANSACTION, kwTRANSLATE, kwTRANSLATION, kwTRIGGER,
    kwTRIM, kwTRUE, kwTYPE,

    kwUNCOMMITTED, kwUNION, kwUNIQUE, kwUNKNOWN, kwUPDATE, kwUPDATING,
    kwUPPER, kwUSER, kwUSING,

    kwVALUE, kwVALUES, kwVARCHAR, kwVARIABLE, kwVARYING, kwVERSION, kwVIEW,

    kwWAIT, kwWAIT_TIME, kwWEEKDAY, kwWHEN, kwWHENEVER, kwWHERE, kwWHILE,
    kwWITH, kwWORK, kwWRITE,

    kwYEAR, kwYEARDAY
};
//-----------------------------------------------------------------------------
// This tokenizer class returns only "important" tokens - it skips whitespace,
// comments and the like
class SqlTokenizer
{
private:
    wxString sqlM;
    wxString termM;
    const wxChar* sqlTokenStartM;
    const wxChar* sqlTokenEndM;
    SqlTokenType sqlTokenTypeM;
    void init();

    void defaultToken();
    void keywordIdentifierToken();
    void multilineCommentToken();
    void quotedIdentifierToken();
    void singleLineCommentToken();
    void stringToken();
    void symbolToken(SqlTokenType type);
    void whitespaceToken();
public:
    SqlTokenizer();
    SqlTokenizer(const wxString& statement);

    SqlTokenType getCurrentToken();
    wxString getCurrentTokenString();
    bool isKeywordToken();
    bool nextToken();

    void setStatement(const wxString& statement);

    // returns TokenType of parameter string if possibleKeyword is a keyword,
    // returns tkIdentifier otherwise
    static SqlTokenType getKeywordTokenType(const wxString& possibleKeyword);
};
//-----------------------------------------------------------------------------
#endif // FR_SQLTOKENIZER_H
