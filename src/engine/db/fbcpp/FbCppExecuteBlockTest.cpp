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

// Regression test for GitHub issue #779:
// Crash after executing EXECUTE BLOCK with parameters, sub-procedures and BLOB
// output when fetching all rows or executing a subsequent statement.

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "engine/db/fbcpp/FbCppBlob.h"
#include "engine/db/fbcpp/FbCppDatabase.h"
#include "engine/db/fbcpp/FbCppStatement.h"
#include "engine/db/fbcpp/FbCppTransaction.h"

namespace
{

bool check(bool cond, const char* msg)
{
    if (cond)
        return true;
    std::cerr << "FAIL: " << msg << "\n";
    return false;
}

bool testExecuteBlockParameterPreprocessing()
{
    bool ok = true;

    // 1. Exact structure from issue #779: leading newline after SET TERM ^;,
    // 5 named input parameters, DECLARE PROCEDURE sub-procedures, comments,
    // and PSQL :VAR references (:LLENGTH, :TABLE_NAME, :FIELD_NAME, :NEW_VALUE, :LHITS).
    const std::string issue779Sql = R"SQL(
execute block (
    XSEARCHTERM     varchar( 200) = :SEARCH_TERM,
    XREPLACEMENT    varchar( 200) = :REPLACEMENT,
    XCASESENSITIVE  boolean = :CASE_SENSITIVE,
    XWHOLEWORDSONLY boolean = :WHOLE_WORDS_ONLY,
    XPREVIEWONLY    boolean = :PREVIEW_ONLY)
returns (
    TABLE_NAME varchar( 63) character set UTF8,
    FIELD_NAME varchar( 63) character set UTF8,
    OLD_VALUE  blob sub_type TEXT,
    NEW_VALUE  blob sub_type TEXT,
    STATUS     varchar( 100))
as
    declare variable LLENGTH integer;
    declare procedure REPLACETEXT (
        XTEXT varchar(8191))
    returns (
        XNEW varchar(8191),
        XHITS integer)
    as
    begin
        XNEW = XTEXT;
        XHITS = 1;
    end
begin
    for select trim(RF.RDB$RELATION_NAME), trim(RF.RDB$FIELD_NAME)
        from RDB$RELATION_FIELDS RF
        where RF.RDB$FIELD_POSITION >= :LLENGTH
        into :TABLE_NAME, :FIELD_NAME
    do
    begin
        execute procedure REPLACETEXT(:OLD_VALUE)
            returning_values :NEW_VALUE, :LHITS;
        suspend;
    end
end
)SQL";

    std::vector<std::string> params;
    std::string processed = fr::FbCppStatement::preprocessSqlForParameters(issue779Sql, params);

    const std::vector<std::string> expectedParams = {
        "SEARCH_TERM", "REPLACEMENT", "CASE_SENSITIVE", "WHOLE_WORDS_ONLY", "PREVIEW_ONLY"
    };
    ok = check(params == expectedParams,
        "issue #779 EXECUTE BLOCK extracts only the 5 header input parameters") && ok;
    ok = check(processed.find(":LLENGTH") != std::string::npos,
        "PSQL :LLENGTH inside body is preserved untouched") && ok;
    ok = check(processed.find("into :TABLE_NAME, :FIELD_NAME") != std::string::npos,
        "PSQL INTO :TABLE_NAME, :FIELD_NAME is preserved untouched") && ok;
    ok = check(processed.find("returning_values :NEW_VALUE, :LHITS") != std::string::npos,
        "PSQL RETURNING_VALUES :NEW_VALUE, :LHITS is preserved untouched") && ok;

    // 2. Parameter name containing BEGIN (e.g. :BEGIN_DATE) in EXECUTE BLOCK header
    // must NOT prematurely stop replacing subsequent header parameters!
    const std::string beginParamSql =
        "  /* comment */\n"
        "  EXECUTE BLOCK (D1 DATE = :BEGIN_DATE, D2 DATE = :END_DATE)\n"
        "  RETURNS (OUT_D DATE)\n"
        "  AS\n"
        "    DECLARE VARIABLE V DATE;\n"
        "  BEGIN\n"
        "    V = :OUT_D;\n"
        "    SUSPEND;\n"
        "  END;";

    std::vector<std::string> beginParams;
    std::string beginProcessed =
        fr::FbCppStatement::preprocessSqlForParameters(beginParamSql, beginParams);
    const std::vector<std::string> expectedBeginParams = { "BEGIN_DATE", "END_DATE" };
    ok = check(beginParams == expectedBeginParams,
        "header parameter named :BEGIN_DATE does not truncate header parameter scan") && ok;
    ok = check(beginProcessed.find("V = :OUT_D;") != std::string::npos,
        "PSQL variable :OUT_D after AS BEGIN is preserved") && ok;
    ok = check(beginProcessed.back() != ';',
        "trailing semicolon stripped") && ok;

    // 3. EXECUTE BLOCK without input parameter list must not touch any :VAR
    const std::string noInputBlock =
        "\nEXECUTE BLOCK RETURNS (ID INT) AS BEGIN SELECT 1 FROM RDB$DATABASE INTO :ID; SUSPEND; END";
    std::vector<std::string> noInputParams;
    std::string noInputProcessed =
        fr::FbCppStatement::preprocessSqlForParameters(noInputBlock, noInputParams);
    ok = check(noInputParams.empty(),
        "parameterless EXECUTE BLOCK reports zero host parameters") && ok;
    ok = check(noInputProcessed.find("INTO :ID") != std::string::npos,
        "parameterless EXECUTE BLOCK preserves INTO :ID") && ok;

    // 4. Escaped quotes in regular SELECT do not confuse parameter replacement
    const std::string quotedSql =
        "SELECT 'it''s :NOT_A_PARAM', :REAL_PARAM FROM RDB$DATABASE";
    std::vector<std::string> quotedParams;
    std::string quotedProcessed =
        fr::FbCppStatement::preprocessSqlForParameters(quotedSql, quotedParams);
    const std::vector<std::string> expectedQuoted = { "REAL_PARAM" };
    ok = check(quotedParams == expectedQuoted,
        "escaped single quotes do not leak fake parameters") && ok;
    ok = check(quotedProcessed.find("'it''s :NOT_A_PARAM'") != std::string::npos,
        "string literal with escaped quote preserved") && ok;

    return ok;
}

bool testDisconnectedBlobHandleSafety()
{
    bool ok = true;

    auto db = std::make_shared<fr::FbCppDatabase>();
    fbcpp::BlobId fakeId;
    fakeId.id.gds_quad_high = 1;
    fakeId.id.gds_quad_low = 1;

    // Constructing and calling open()/close() on FbCppBlob when transaction/connection
    // is inactive must throw a clean std::runtime_error instead of dereferencing
    // dangling references or crashing (#779).
    fr::FbCppBlob orphanBlob(db, nullptr, fakeId);
    bool threwCleanly = false;
    try
    {
        orphanBlob.open();
    }
    catch (const std::runtime_error&)
    {
        threwCleanly = true;
    }
    ok = check(threwCleanly,
        "opening FbCppBlob without active transaction throws clean std::runtime_error") && ok;

    orphanBlob.close();
    orphanBlob.cancel();
    return ok;
}

} // namespace

int main()
{
    bool ok = true;
    ok = testExecuteBlockParameterPreprocessing() && ok;
    ok = testDisconnectedBlobHandleSafety() && ok;

    if (ok)
        std::cout << "All FbCppExecuteBlockTest checks passed.\n";
    return ok ? 0 : 1;
}
