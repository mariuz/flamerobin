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

#ifndef FR_SQLTOKENIZER_H
#define FR_SQLTOKENIZER_H

#include <wx/string.h>
#include <map>

enum SqlTokenType {
    /*
     * these are token types used by SqlTokenizer class
     */
    tkEOF, tkUNKNOWN, tkWHITESPACE, tkCOMMENT,
    tkTERM, tkPARENOPEN, tkPARENCLOSE, tkEQUALS, tkCOMMA,
    tkSTRING, tkIDENTIFIER,

    tk_KEYWORDS_START_HERE,
    /*
     * SQL keywords start here, these are returned instead of tkIDENTIFIER
     */
     kwABS, kwABSOLUTE, kwACCENT, kwACOS, kwACOSH, kwACTION, kwACTIVE, kwADD,
     kwADMIN, kwAFTER, kwALL, kwALTER, kwALWAYS, kwAND, kwANY, kwAS, kwASCENDING,
     kwASCII_CHAR, kwASCII_VAL, kwASIN, kwASINH, kwAT, kwATAN, kwATAN2, kwATANH,
     kwAUTO, kwAUTODDL, kwAVG,
     
     kwBACKUP, kwBASE64_DECODE, kwBASE64_ENCODE, kwBASED, kwBASENAME, kwBEFORE,
     kwBEGIN, kwBETWEEN, kwBIGINT, kwBIN_AND, kwBIN_NOT, kwBIN_OR, kwBIN_SHL,
     kwBIN_SHR, kwBIN_XOR, kwBINARY, kwBIND, kwBIND_PARAM, kwBIT_LENGTH, kwBLOB,
     kwBLOBEDIT, kwBLOCK, kwBODY, kwBOOLEAN, kwBOTH, kwBREAK, kwBUFFER, kwBY,
     
     kwCACHE, kwCALLER, kwCASCADE, kwCASE, kwCAST, kwCEIL, kwCHAR, kwCHAR_LENGTH,
     kwCHAR_TO_UUID, kwCHARACTER, kwCHARACTER_LENGTH, kwCHECK, kwCHECK_POINT_LEN,
     kwCLEAR, kwCLOSE, kwCOALESCE, kwCOLLATE, kwCOLLATION, kwCOLUMN, kwCOMMENT,
     kwCOMMIT, kwCOMMITTED, kwCOMMON, kwCOMPARE_DECFLOAT, kwCOMPILETIME, kwCOMPUTED,
     kwCONCATENATE, kwCONDITIONAL, kwCONNECT, kwCONNECTIONS, kwCONSISTENCY,
     kwCONSTRAINT, kwCONTAINING, kwCONTINUE, kwCORR, kwCOS, kwCOSH, kwCOT, kwCOUNT,
     kwCOUNTER, kwCOVAR_POP, kwCOVAR_SAMP, kwCREATE, kwCROSS, kwCRYPT_HASH,
     kwCSTRING, kwCTR_BIG_ENDIAN, kwCTR_LENGTH, kwCTR_LITTLE_ENDIAN, kwCUME_DIST,
     kwCURRENT, kwCURRENT_CONNECTION, kwCURRENT_DATE, kwCURRENT_ROLE, kwCURRENT_TIME,
     kwCURRENT_TIMESTAMP, kwCURRENT_TRANSACTION, kwCURRENT_USER, kwCURSOR,
     
     kwDATA, kwDATABASE, kwDATE, kwDATEADD, kwDATEDIFF, kwDAY, kwDB_KEY, kwDDL, kwDEC,
     kwDECFLOAT, kwDECIMAL, kwDECLARE, kwDECODE, kwDECRYPT, kwDEFAULT, kwDEFINER,
     kwDELETE, kwDELETING, kwDENSE_RANK, kwDESCENDING, kwDESCRIBE, kwDESCRIPTOR,
     kwDETERMINISTIC, kwDIFFERENCE, kwDISABLE, kwDISCONNECT, kwDISPLAY, kwDISTINCT,
     kwDO, kwDOMAIN, kwDOUBLE, kwDROP, 
     
     kwECHO, kwEDIT, kwELSE, kwENABLE, kwENCRYPT, kwEND, kwENGINE, kwENTRY_POINT,
     kwEQU, kwESCAPE, kwEVENT, kwEXCEPTION, kwEXCESS, kwEXCLUDE, kwEXECUTE, kwEXISTS, 
     kwEXIT,kwEXP, kwEXTENDED, kwEXTERN, kwEXTERNAL, kwEXTRACT, 
     
     kwFALSE, kwFETCH, kwFILE, kwFILTER, kwFIRST, kwFIRST_DAY, kwFIRST_VALUE, kwFIRSTNAME,
     kwFLOAT, kwFLOOR, kwFOLLOWING, kwFOR, kwFOREIGN, kwFOUND, kwFREE_IT, kwFROM,
     kwFULL, kwFUNCTION,
     
     kwGDSCODE, kwGEN_ID, kwGEN_UUID, kwGENERATED, kwGENERATOR, kwGEQ, kwGLOBAL, kwGOTO,
     kwGRA, kwGRANT, kwGRANTED, kwGROUP, kwGROUP_COMMIT_, kwGROUP_COMMIT_WAIT,
     
     kwHASH, kwHAVING, kwHELP, kwHEX_DECODE, kwHEX_ENCODE, kwHOUR,
     
     kwIDENTITY, kwIDLE, kwIF, kwIGNORE, kwIIF, kwIMMEDIATE, kwIN, kwINACTIVE, kwINCLUDE,
     kwINCREMENT, kwINDENTITY, kwINDEX, kwINDICATOR, kwINIT, kwINNER, kwINPUT,
     kwINPUT_TYPE, kwINSENSITIVE, kwINSERT, kwINSERTING, kwINT, kwINT128, kwINTEGER,
     kwINTO, kwINVOKER, kwIS, kwISOLATION, kwISQL, kwIV,
     
     kwJOIN, kwKEY,
     
     kwLAG, kwLAST, kwLAST_DAY, kwLAST_VALUE, kwLASTNAME, kwLATERAL, kwLC_MESSAGES, kwLC_TYPE,
     kwLEAD, kwLEADING, kwLEAVE, kwLEFT, kwLEGACY, kwLENGTH, kwLESS, kwLEQ, kwLEV, kwLEVEL, 
     kwLIFETIME, kwLIKE, kwLIMBO, kwLINGER, kwLIST, kwLN, kwLOCAL, kwLOCALTIME, kwLOCALTIMESTAMP, 
     kwLOCK, kwLOG, kwLOG_BUF_SIZE, kwLOG10, kwLOGFILE, kwLONG, kwLOWER, kwLPAD, kwLPARAM,
     
     kwMAKE_DBKEY, kwMANUAL, kwMAPPING, kwMATCHED, kwMATCHING, kwMAX, kwMAX_SEGMENT, kwMAXIMUM,
     kwMAXIMUM_SEGMENT, kwMAXVALUE, kwMERGE, kwMESSAGE, kwMIDDLENAME, kwMILLISECOND, kwMIN,
     kwMINIMUM, kwMINUTE, kwMINVALUE, kwMOD, kwMODE, kwMODULE_NAME, kwMONTH, 
     
     kwNAME, kwNAMES, kwNATIONAL, kwNATIVE, kwNATURAL, kwNCHAR, kwNEQ, kwNEXT, kwNO, kwNOAUTO,
     kwNORMALIZE_DECFLOAT, kwNOT, kwNOT_GTR, kwNOT_LSS, kwNTH_VALUE, kwNTILE, kwNULL, kwNULLIF,
     kwNULLS, kwNUM_LOG_BUFS, kwNUMBER, kwNUMERIC,
     
     kwOCTET_LENGTH, kwOF, kwOFFSET, kwOLDEST, kwON, kwONLY, kwOPEN, kwOPTION, kwOR, kwORDER,
     kwOS_NAME, kwOTHERS, kwOUTER, kwOUTPUT, kwOUTPUT_TYPE, kwOVER, kwOVERFLOW, kwOVERLAY,
     kwOVERRIDING,

     kwPACKAGE, kwPAD, kwPAGE, kwPAGE_SIZE, kwPAGELENGTH, kwPAGES, kwPARAMETER, kwPARTITION,
     kwPASSWORD, kwPERCENT, kwPERCENT_RANK, kwPI, kwPLACING, kwPLAN, kwPLUGIN, kwPOOL,
     kwPOSITION, kwPOST_EVENT, kwPOWER, kwPRECEDING, kwPRECISION, kwPREPARE, kwPRESERVE,
     kwPRIMARY, kwPRIOR, kwPRIVILEGE, kwPRIVILEGES, kwPROCEDURE, kwPROTECTED, kwPUBLIC,
     kwPUBLICATION,
     
     kwQUANTIZE,  kwQUIT,
     
     kwRAND, kwRANGE, kwRANK, kwRAW_PARTITIONS, kwRDB_ERROR, kwRDB_GET_CONTEXT, kwRDB_GET_TRANSACTION_CN,
     kwRDB_RECORD_VERSION, kwRDB_ROLE_IN_USE, kwRDB_SET_CONTEXT, kwRDB_SYSTEM_PRIVILEGE,
     kwREAD, kwREAL, kwRECORD_VERSION, kwRECREATE, kwRECURSIVE, kwREFERENCES, kwREGR_AVGX,
     kwREGR_AVGY, kwREGR_COUNT, kwREGR_INTERCEPT, kwREGR_R2, kwREGR_SLOPE, kwREGR_SXX,
     kwREGR_SXY, kwREGR_SYY, kwRELATIVE, kwRELEASE, kwREPLACE, kwREQUESTS, kwRESERV, kwRESERVING,
     kwRESET, kwRESTART, kwRESTRICT, kwRETAIN, kwRETURN, kwRETURNING, kwRETURNING_VALUES,
     kwRETURNS, kwREVERSE, kwREVOKE, kwRIGHT, kwROLE, kwROLLBACK, kwROUND, kwROW, kwROW_COUNT,
     kwROW_NUMBER, kwROWS, kwRPAD, kwRSA_DECRYPT, kwRSA_ENCRYPT, kwRSA_PRIVATE, kwRSA_PUBLIC,
     kwRSA_SIGN, kwRSA_VERIFY, kwRUNTIME, 
     
     kwSALT_LENGTH, kwSAVEPOINT, kwSCALAR_ARRAY, kwSCHEMA, kwSCROLL, kwSECOND, kwSECUENCE, 
     kwSECURITY, kwSEGMENT, kwSELECT, kwSENSITIVE, kwSERVERWIDE, kwSESSION, kwSET, kwSHADOW, 
     kwSHARED, kwSHELL, kwSHOW, kwSIGN, kwSIGNATURE, kwSIMILAR, kwSIN, kwSINGULAR, kwSINH, 
     kwSIZE, kwSKIP, kwSMALLINT, kwSNAPSHOT, kwSOME, kwSORT, kwSOURCE, kwSPACE, kwSQL, 
     kwSQLCODE, kwSQLERROR, kwSQLSTATE, kwSQLWARNING, kwSQRT, kwSTABILITY, kwSTART, kwSTARTING,
     kwSTARTS, kwSTATEMENT, kwSTATIC, kwSTATISTICS, kwSTDDEV_POP, kwSTDDEV_SAMP, kwSUB_TYPE,
     kwSUBSTRING, kwSUM, kwSUSPEND, kwSYSTEM, 
     
     kwTABLE, kwTAGS, kwTAN, kwTANH, kwTEMPORARY, kwTERMINATOR, kwTHEN, kwTIES, kwTIME, kwTIMEOUT,
     kwTIMESTAMP, kwTIMEZONE_HOUR, kwTIMEZONE_MINUTE, kwTO, kwTOTALORDER, kwTRAILING, kwTRANSACTION,
     kwTRANSLATE, kwTRANSLATION, kwTRAPS, kwTRIGGER, kwTRIM, kwTRUE, kwTRUNC, kwTRUSTED, kwTWO_PHASE,
     kwTYPE,
        
     kwUNBOUNDED, kwUNCOMMITTED, kwUNDO, kwUNION, kwUNIQUE, kwUNKNOWN, kwUPDATE, kwUPDATING, kwUPPER,
     kwUSAGE, kwUSER, kwUSING, kwUUID_TO_CHAR, 
     
     kwVALUE,kwVALUES, kwVAR_POP, kwVAR_SAMP, kwVARBINARY, kwVARCHAR, kwVARIABLE, kwVARYING, kwVERSION,
     kwVIEW,
         
     kwWAIT, kwWAIT_TIME, kwWEEK, kwWEEKDAY, kwWHEN, kwWHENEVER, kwWHERE, kwWHILE, kwWINDOW, kwWITH,
     kwWITHOUT, kwWORK, kwWRITE,
     
     kwYEAR, kwYEARDAY,

     kwZONE

};


// This tokenizer class returns only "important" tokens - it skips whitespace,
// comments and the like



class SqlTokenizer
{
private:
    typedef std::map<wxString, SqlTokenType> KeywordToTokenMap;
    typedef std::map<wxString, SqlTokenType>::value_type KeywordToTokenEntry;

    typedef std::map<SqlTokenType, wxString> TokenToKeywordMap;
    typedef std::map<SqlTokenType, wxString>::value_type TokenToKeywordEntry;

    wxString sqlM;
    wxString termM;
    const wxChar* sqlTokenStartM;
    const wxChar* sqlTokenEndM;
    SqlTokenType sqlTokenTypeM;
    void init();

    static const KeywordToTokenMap& getKeywordToTokenMap();

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
    int getCurrentTokenPosition();
    bool isKeywordToken();
    bool nextToken();
    bool jumpToken(bool skipParenthesis);   // skip whitespace and comments

    void setStatement(const wxString& statement);

    enum KeywordCase { kwDefaultCase, kwLowerCase, kwUpperCase };
    // return keyword string for a given token type
    static wxString getKeyword(SqlTokenType token);
    static wxString getKeyword(SqlTokenType token, bool upperCase);
    // returns array of keyword strings
    static wxArrayString getKeywords(KeywordCase kwc);
    // returns all keywords in one string, separated by spaces
    static wxString getKeywordsString(KeywordCase kwc);
    // returns TokenType of parameter string if word is a keyword,
    // returns tkIdentifier otherwise
    static SqlTokenType getKeywordTokenType(const wxString& word);
    static bool isReservedWord(const wxString& word);
};

#endif // FR_SQLTOKENIZER_H
