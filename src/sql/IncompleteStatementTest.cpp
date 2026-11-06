#include <iostream>
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "metadata/database.h"
#include "metadata/function.h"
#include "metadata/package.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "sql/IncompleteStatement.h"

namespace
{
bool checkEq(const wxString& actual, const wxString& expected, const char* testName)
{
    if (actual == expected)
    {
        std::cout << "[PASS] " << testName << "\n";
        return true;
    }
    std::cerr << "[FAIL] " << testName
              << "\n  expected: '" << expected.ToStdString()
              << "'\n  actual:   '" << actual.ToStdString() << "'\n";
    return false;
}

bool checkTrue(bool cond, const char* testName)
{
    if (cond)
    {
        std::cout << "[PASS] " << testName << "\n";
        return true;
    }
    std::cerr << "[FAIL] " << testName << "\n";
    return false;
}
} // anonymous namespace

int main()
{
    bool ok = true;

    DatabasePtr db(new Database());

    // 1. User package MY_PKG with function CALC_TAX and procedure GET_ITEMS
    PackagePtr userPkg(new Package(db, "MY_PKG"));
    FunctionSQLPtr fnCalcTax(new FunctionSQL(userPkg.get(), "CALC_TAX"));
    ProcedurePtr prGetItems(new Procedure(userPkg.get(), "GET_ITEMS"));
    ProcedurePtr prProcessOrder(new Procedure(userPkg.get(), "PROCESS_ORDER"));

    wxString emptyRel;
    wxString emptyField;
    ParameterPtr inParam(new Parameter(prGetItems.get(), "IN_CATEGORY_ID"));
    inParam->initialize("INTEGER", 0, -1, true, "", false, false, emptyRel, emptyField);
    ParameterPtr outId(new Parameter(prGetItems.get(), "ITEM_ID"));
    outId->initialize("INTEGER", 1, -1, true, "", false, false, emptyRel, emptyField);
    ParameterPtr outName(new Parameter(prGetItems.get(), "ITEM_NAME"));
    outName->initialize("VARCHAR(50)", 1, -1, true, "", false, false, emptyRel, emptyField);
    prGetItems->setParameters({inParam, outId, outName});

    userPkg->setMethods({fnCalcTax}, {prGetItems, prProcessOrder});
    db->getPackages()->insertItem(userPkg);

    // 2. System package RDB$PROFILER
    PackagePtr sysPkg(new Package(db, "RDB$PROFILER"));
    FunctionSQLPtr fnStart(new FunctionSQL(sysPkg.get(), "START_SESSION"));
    ProcedurePtr prFinish(new Procedure(sysPkg.get(), "FINISH_SESSION"));
    sysPkg->setMethods({fnStart}, {prFinish});
    db->getSysPackages()->insertItem(sysPkg);

    // Test 1: Autocomplete members of user package MY_PKG.
    {
        wxString sql = "SELECT MY_PKG. FROM RDB$DATABASE";
        int dotAfterPos = sql.Find("MY_PKG.") + 7;
        IncompleteStatement is(db.get(), sql);
        wxString members = is.getObjectColumns("MY_PKG", dotAfterPos, true);
        ok = checkEq(members, "CALC_TAX GET_ITEMS PROCESS_ORDER",
            "User package member completion (MY_PKG.)") && ok;
    }

    // Test 2: Autocomplete members of system package RDB$PROFILER.
    {
        wxString sql = "SELECT RDB$PROFILER. FROM RDB$DATABASE";
        int dotAfterPos = sql.Find("RDB$PROFILER.") + 13;
        IncompleteStatement is(db.get(), sql);
        wxString members = is.getObjectColumns("RDB$PROFILER", dotAfterPos, true);
        ok = checkEq(members, "FINISH_SESSION START_SESSION",
            "System package member completion (RDB$PROFILER.)") && ok;
    }

    // Test 3: Selectable package procedure output parameters via alias (p.)
    {
        wxString sql = "SELECT p. FROM MY_PKG.GET_ITEMS(10) p";
        int dotAfterPos = sql.Find("p.") + 2;
        IncompleteStatement is(db.get(), sql);
        wxString cols = is.getObjectColumns("p", dotAfterPos, true);
        ok = checkEq(cols, "ITEM_ID ITEM_NAME",
            "Selectable package procedure output parameters via alias (p.)") && ok;
    }

    // Test 4: Selectable package procedure output parameters via qualified name (MY_PKG.GET_ITEMS.)
    {
        wxString sql = "SELECT MY_PKG.GET_ITEMS. FROM MY_PKG.GET_ITEMS(10)";
        int dotAfterPos = sql.Find("MY_PKG.GET_ITEMS.") + 17;
        IncompleteStatement is(db.get(), sql);
        wxString cols = is.getObjectColumns("MY_PKG.GET_ITEMS", dotAfterPos, true);
        ok = checkEq(cols, "ITEM_ID ITEM_NAME",
            "Selectable package procedure output parameters via qualified name") && ok;
    }

    // Test 5: Unqualified column completion in SELECT from selectable package procedure
    {
        wxString sql = "SELECT IT FROM MY_PKG.GET_ITEMS(10) p";
        int cursorPos = sql.Find("IT") + 2;
        IncompleteStatement is(db.get(), sql);
        wxString cols = is.getObjectColumns("", cursorPos, true);
        ok = checkEq(cols, "ITEM_ID ITEM_NAME",
            "Unqualified column completion for selectable package procedure in FROM") && ok;
    }

    // Test 6: Database::findByName resolves PACKAGE.FUNCTION and PACKAGE.PROCEDURE
    {
        ok = checkTrue(db->findByName("MY_PKG.CALC_TAX") == fnCalcTax.get(),
            "Database::findByName resolves MY_PKG.CALC_TAX") && ok;
        ok = checkTrue(db->findByName("MY_PKG.GET_ITEMS") == prGetItems.get(),
            "Database::findByName resolves MY_PKG.GET_ITEMS") && ok;
        ok = checkTrue(db->findByName("RDB$PROFILER.START_SESSION") == fnStart.get(),
            "Database::findByName resolves RDB$PROFILER.START_SESSION") && ok;
    }

    return ok ? 0 : 1;
}
