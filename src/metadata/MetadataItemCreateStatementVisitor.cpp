/*
  Copyright (c) 2004-2016 The FlameRobin Development Team

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

#include "metadata/MetadataItemCreateStatementVisitor.h"
#include "sql/StatementBuilder.h"

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateDomainStatement()
{
    return  "CREATE DOMAIN domain_name\n"
            "AS datatype [CHARACTER SET charset]\n"
            "DEFAULT {literal | NULL | USER}\n"
            "[NOT NULL]\n"
            "[CHECK (dom_search_condition)]\n"
            "COLLATE collation;\n";
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateExceptionStatement()
{
    StatementBuilder sb;
    sb << kwCREATE << ' ' << kwEXCEPTION << " name 'exception message';"
        << StatementBuilder::NewLine;
    return sb;
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateFunctionSQLStatement()
{
    wxString s("SET TERM ^ ;\n\n"
        "CREATE FUNCTION name \n"
        " ( input_parameter_name < datatype>, ... ) \n"
        "RETURNS  < datatype>)\n"
        "AS \n"
        "DECLARE VARIABLE variable_name < datatype>; \n"
        "BEGIN\n"
        "  /* write your code here */ \n"
        "END^\n\n"
        "SET TERM ; ^\n");
    return s;
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateUDFStatement()
{
    return "DECLARE EXTERNAL FUNCTION name [datatype | CSTRING (int) "
           "[, datatype | CSTRING (int) ...]]\n"
           "RETURNS {datatype [BY VALUE] | CSTRING (int)} [FREE_IT]\n"
           "ENTRY_POINT 'entryname'\n"
           "MODULE_NAME 'modulename';\n";
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateGeneratorStatement()
{
    StatementBuilder sb;
    sb << kwCREATE << ' ' << kwGENERATOR << " name;"
        << StatementBuilder::NewLine
        << kwSET << ' ' << kwGENERATOR << " name " << kwTO
        << " value;" << StatementBuilder::NewLine;
    return sb;
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreatePackageStatement()
{
    wxString s("SET TERM ^ ;\n\n"
        "CREATE PACKAGE name \n"
        "AS \n"
        "BEGIN\n"
        "  /* write your code here */ \n"
        "END^\n\n"
        "SET TERM ; ^\n");
    return s;
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateProcedureStatement()
{
    wxString s("SET TERM ^ ;\n\n"
            "CREATE PROCEDURE name \n"
            " ( input_parameter_name < datatype>, ... ) \n"
            "RETURNS \n"
            " ( output_parameter_name < datatype>, ... )\n"
            "AS \n"
            "DECLARE VARIABLE variable_name < datatype>; \n"
            "BEGIN\n"
            "  /* write your code here */ \n"
            "END^\n\n"
            "SET TERM ; ^\n");
    return s;
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateRoleStatement()
{
    return  "CREATE ROLE role_name;\n";
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateTableStatement()
{
    return "CREATE TABLE table_name\n"
        "(\n"
        "    column_name {< datatype> | COMPUTED BY (< expr>) | domain}\n"
        "        [DEFAULT { literal | NULL | USER}] [NOT NULL]\n"
        "    ...\n"
        "    CONSTRAINT constraint_name\n"
        "        PRIMARY KEY (column_list),\n"
        "        UNIQUE      (column_list),\n"
        "        FOREIGN KEY (column_list) REFERENCES other_table (column_list),\n"
        "        CHECK       (condition),\n"
        "    ...\n"
        ");\n";
}

wxString MetadataItemCreateStatementVisitor::getCreateGTTTableStatement()
{
    return wxString();
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateDMLTriggerStatement()
{
    return "SET TERM ^ ;\n\n"
        "CREATE TRIGGER name \n"
//        "[FOR table/view] \n"
        " [IN]ACTIVE \n"
        " [{BEFORE | AFTER} INSERT OR UPDATE OR DELETE] \n"
        " POSITION number \n "
        " ON table/view \n"
        "AS \n"
        "BEGIN \n"
        "    /* enter trigger code here */ \n"
        "END^\n\n"
        "SET TERM ; ^\n";
}

wxString MetadataItemCreateStatementVisitor::getCreateDBTriggerStatement()
{
    return "SET TERM ^ ;\n\n"
        "CREATE TRIGGER name  \n"
        " [IN]ACTIVE \n"
        " [ON {[DIS]CONNECT | TRANSACTION {START | COMMIT | ROLLBACK}} ] \n"
        " POSITION number \n"
        "AS \n"
        "BEGIN \n"
        "    /* enter trigger code here */ \n"
        "END^\n\n"
        "SET TERM ; ^\n";
}

wxString MetadataItemCreateStatementVisitor::getCreateDDLTriggerStatement()
{
    return "SET TERM ^ ;\n\n"
        "CREATE TRIGGER name  \n"
        " [IN]ACTIVE \n"
        " {BEFORE | AFTER}  CREATE TABLE | ALTER TABLE | DROP TABLE | \n"
        "                   CREATE PROCEDURE | ALTER PROCEDURE | DROP PROCEDURE | \n"
        "                   CREATE FUNCTION | ALTER FUNCTION | DROP FUNCTION | \n"
        "                   CREATE TRIGGER  | ALTER TRIGGER  | DROP TRIGGER  | \n"
        "                   CREATE EXCEPTION | ALTER EXCEPTION | DROP EXCEPTION | \n"
        "                   CREATE VIEW | ALTER VIEW | DROP VIEW | \n"
        "                   CREATE DOMAIN | ALTER DOMAIN | DROP DOMAIN | \n"
        "                   CREATE ROLE | ALTER ROLE | DROP ROLE \n"
        "                   CREATE SEQUENCE | ALTER SEQUENCE | DROP SEQUENCE | \n"
        "                   CREATE USER | ALTER USER | DROP USER | \n"
        "                   CREATE INDEX | ALTER INDEX | DROP INDEX | \n"
        "                   CREATE COLLATION | DROP COLLATION | ALTER CHARACTER SET | \n"
        "                   CREATE PACKAGE | ALTER PACKAGE | DROP PACKAGE | \n"
        "                   CREATE PACKAGE BODY| DROP PACKAGE BODY \n"
        " POSITION number \n"
        "AS \n"
        "BEGIN \n"
        "    /* enter trigger code here */ \n"
        "END^\n\n"
        "SET TERM ; ^\n";
}

/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateViewStatement()
{
    StatementBuilder sb;
    sb << kwCREATE << ' ' << kwVIEW << " name ( view_column, ...)"
        << StatementBuilder::NewLine << kwAS << StatementBuilder::NewLine
        << "/* write select statement here */"
        << StatementBuilder::NewLine
        << kwWITH << ' ' << kwCHECK << ' ' << kwOPTION << ';'
        << StatementBuilder::NewLine;
    return sb;
}

void MetadataItemCreateStatementVisitor::visitDomains(Domains& /*domains*/)
{
    statementM = getCreateDomainStatement();
}

void MetadataItemCreateStatementVisitor::visitExceptions(
    Exceptions& /*exceptions*/)
{
    statementM = getCreateExceptionStatement();
}

void MetadataItemCreateStatementVisitor::visitFunctionSQLs(
    FunctionSQLs& /*functions*/)
{
    statementM = getCreateFunctionSQLStatement();
}

void MetadataItemCreateStatementVisitor::visitUDFs(
    UDFs& /*udfs*/)
{
    statementM = getCreateUDFStatement();
}

void MetadataItemCreateStatementVisitor::visitGenerators(
    Generators& /*generators*/)
{
    statementM = getCreateGeneratorStatement();
}

void MetadataItemCreateStatementVisitor::visitPackages(
    Packages& /*packages*/)
{
    statementM = getCreatePackageStatement();
}

void MetadataItemCreateStatementVisitor::visitProcedures(
    Procedures& /*procedures*/)
{
    statementM = getCreateProcedureStatement();
}

void MetadataItemCreateStatementVisitor::visitRoles(Roles& /*roles*/)
{
    statementM = getCreateRoleStatement();
}

void MetadataItemCreateStatementVisitor::visitTables(Tables& /*tables*/)
{
    statementM = getCreateTableStatement();
}

void MetadataItemCreateStatementVisitor::visitDBTriggers(DBTriggers& /*triggers*/)
{
    statementM = getCreateDBTriggerStatement();
}

void MetadataItemCreateStatementVisitor::visitDDLTriggers(DDLTriggers& /*triggers*/)
{
    statementM = getCreateDDLTriggerStatement();
}

void MetadataItemCreateStatementVisitor::visitDMLTriggers(DMLTriggers& /*triggers*/)
{
    statementM = getCreateDMLTriggerStatement();
}

void MetadataItemCreateStatementVisitor::visitViews(Views& /*views*/)
{
    statementM = getCreateViewStatement();
}

wxString MetadataItemCreateStatementVisitor::getStatement() const
{
    return statementM;
}

