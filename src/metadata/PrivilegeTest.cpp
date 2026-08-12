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

#include <iostream>
#include "metadata/generator.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/procedure.h"
#include "metadata/privilege.h"

namespace
{
class MockGenerator : public Generator
{
public:
    MockGenerator(const wxString& name) : Generator(nullptr, name) {}
    virtual void loadProperties() override {}
    virtual const wxString getTypeName() const override { return "SEQUENCE"; }
    virtual void acceptVisitor(MetadataItemVisitor*) override {}
};

class MockDomain : public Domain
{
public:
    MockDomain(const wxString& name) : Domain(nullptr, name) {}
    virtual void loadProperties() override {}
    virtual const wxString getTypeName() const override { return "DOMAIN"; }
    virtual bool isSystem() const override { return false; }
    virtual void acceptVisitor(MetadataItemVisitor*) override {}
};

class MockException : public Exception
{
public:
    MockException(const wxString& name) : Exception(nullptr, name) {}
    virtual void loadProperties() override {}
    virtual const wxString getTypeName() const override { return "EXCEPTION"; }
    virtual void acceptVisitor(MetadataItemVisitor*) override {}
};

class MockProcedure : public Procedure
{
public:
    MockProcedure(const wxString& name) : Procedure(nullptr, name) {}
    virtual void acceptVisitor(MetadataItemVisitor*) override {}
};

bool check(const wxString& actual, const wxString& expected, const char* testName)
{
    if (actual == expected)
        return true;
    std::string expectedText(expected.mb_str());
    std::string actualText(actual.mb_str());
    std::cerr << testName << " failed.\n"
        << "Expected: " << expectedText << "\n"
        << "Actual:   " << actualText << "\n";
    return false;
}
}

int main()
{
    bool ok = true;

    // Test 1: Privilege on Generator (Sequence)
    {
        MockGenerator gen("CUST_NO_GEN2");
        Privilege priv(&gen, "SYSDBA", 0);
        priv.addPrivilege('G', "SYSDBA", true);

        wxString sql = priv.getSql();
        wxString expected = "GRANT USAGE\n ON SEQUENCE CUST_NO_GEN2 TO  SYSDBA WITH GRANT OPTION GRANTED BY SYSDBA;\n";
        ok = check(sql, expected, "Generator privilege GRANT SQL") && ok;
    }

    // Test 2: Privilege on Domain
    {
        MockDomain dom("D_ACCOUNT_ID");
        Privilege priv(&dom, "SYSDBA", 0);
        priv.addPrivilege('G', "SYSDBA", false);

        wxString sql = priv.getSql();
        wxString expected = "GRANT USAGE\n ON DOMAIN D_ACCOUNT_ID TO  SYSDBA GRANTED BY SYSDBA;\n";
        ok = check(sql, expected, "Domain privilege GRANT SQL") && ok;
    }

    // Test 3: Privilege on Exception
    {
        MockException exc("E_INVALID_ID");
        Privilege priv(&exc, "USER1", 0);
        priv.addPrivilege('G', "SYSDBA", false);

        wxString sql = priv.getSql();
        wxString expected = "GRANT USAGE\n ON EXCEPTION E_INVALID_ID TO  USER1 GRANTED BY SYSDBA;\n";
        ok = check(sql, expected, "Exception privilege GRANT SQL") && ok;
    }

    // Test 4: Privilege on Procedure
    {
        MockProcedure proc("SP_PROCESS_ORDER");
        Privilege priv(&proc, "SYSDBA", 0);
        priv.addPrivilege('X', "SYSDBA", true);

        wxString sql = priv.getSql();
        wxString expected = "GRANT EXECUTE\n ON PROCEDURE SP_PROCESS_ORDER TO  SYSDBA WITH GRANT OPTION GRANTED BY SYSDBA;\n";
        ok = check(sql, expected, "Procedure privilege GRANT SQL") && ok;
    }

    return ok ? 0 : 1;
}
