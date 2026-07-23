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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "mcp/McpServer.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>

#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"

#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/database.h"
#include "metadata/table.h"
#include "metadata/column.h"
#include "metadata/view.h"
#include "metadata/procedure.h"
#include "metadata/trigger.h"
#include "metadata/generator.h"
#include "metadata/domain.h"
#include "metadata/role.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/package.h"
#include "metadata/CreateDDLVisitor.h"
#include "gui/SchemaDiff.h"
#include "core/StringUtils.h"

using json = nlohmann::json;

namespace fr
{

static MetadataItem* findMetadataObject(DatabasePtr db, const std::string& name)
{
    wxString wxName = wxString::FromUTF8(name.c_str());
    
    // Check Tables
    {
        auto c = db->getTables();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check Views
    {
        auto c = db->getViews();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check Procedures
    {
        auto c = db->getProcedures();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check DML Triggers
    {
        auto c = db->getDMLTriggers();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check DDL Triggers
    {
        auto c = db->getDDLTriggers();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check DB Triggers
    {
        auto c = db->getDBTriggers();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check Generators
    {
        auto c = db->getGenerators();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check Domains
    {
        auto c = db->getDomains();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check Roles
    {
        auto c = db->getRoles();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check Exceptions
    {
        auto c = db->getExceptions();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check UDFs
    {
        auto c = db->getUDFs();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check Packages
    {
        auto c = db->getPackages();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }
    // Check GT Tables
    {
        auto c = db->getGTTables();
        c->ensureChildrenLoaded();
        if (auto item = c->findByName(wxName)) return item.get();
    }

    return nullptr;
}

void McpServer::run()
{
    std::cerr << "[McpServer] Starting FlameRobin MCP server..." << std::endl;

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line.empty())
            continue;

        json request;
        try
        {
            request = json::parse(line);
        }
        catch (const std::exception& e)
        {
            json errResponse;
            errResponse["jsonrpc"] = "2.0";
            errResponse["error"] = {
                {"code", -32700},
                {"message", std::string("Parse error: ") + e.what()}
            };
            std::cout << errResponse.dump() << std::endl;
            continue;
        }

        std::string method = request.value("method", "");
        json id = request.value("id", json(nullptr));

        if (method == "initialize")
        {
            json response;
            response["jsonrpc"] = "2.0";
            response["id"] = id;
            response["result"] = {
                {"protocolVersion", "2024-11-05"},
                {"capabilities", {
                    {"tools", json::object()}
                }},
                {"serverInfo", {
                    {"name", "flamerobin-mcp"},
                    {"version", "1.0.0"}
                }}
            };
            std::cout << response.dump() << std::endl;
        }
        else if (method == "notifications/initialized")
        {
            // No response required for notifications
            continue;
        }
        else if (method == "tools/list")
        {
            json response;
            response["jsonrpc"] = "2.0";
            response["id"] = id;
            
            json tools = json::array();

            // list_databases tool
            json listDbs;
            listDbs["name"] = "list_databases";
            listDbs["description"] = "List all Firebird databases registered in FlameRobin configuration.";
            listDbs["inputSchema"] = {
                {"type", "object"},
                {"properties", json::object()}
            };
            tools.push_back(listDbs);

            // get_schema tool
            json getSchema;
            getSchema["name"] = "get_schema";
            getSchema["description"] = "Retrieve the schema layout (tables, views, columns) of a specific database.";
            getSchema["inputSchema"] = {
                {"type", "object"},
                {"properties", {
                    {"database_name", {
                        {"type", "string"},
                        {"description", "The name of the database as registered in FlameRobin."}
                    }},
                    {"password", {
                        {"type", "string"},
                        {"description", "Optional connection password if not saved."}
                    }}
                }},
                {"required", json::array({"database_name"})}
            };
            tools.push_back(getSchema);

            // execute_query tool
            json execQuery;
            execQuery["name"] = "execute_query";
            execQuery["description"] = "Execute a SQL query (SELECT, INSERT, UPDATE, etc.) on a specific database and return rows/results.";
            execQuery["inputSchema"] = {
                {"type", "object"},
                {"properties", {
                    {"database_name", {
                        {"type", "string"},
                        {"description", "The name of the database as registered in FlameRobin."}
                    }},
                    {"sql", {
                        {"type", "string"},
                        {"description", "The SQL statement to execute."}
                    }},
                    {"password", {
                        {"type", "string"},
                        {"description", "Optional connection password if not saved."}
                    }}
                }},
                {"required", json::array({"database_name", "sql"})}
            };
            tools.push_back(execQuery);

            // get_metadata_ddl tool
            json getDdl;
            getDdl["name"] = "get_metadata_ddl";
            getDdl["description"] = "Retrieve the SQL Data Definition Language (DDL) statement used to create or define a specific database object (table, view, procedure, trigger, generator, domain, role, exception, package).";
            getDdl["inputSchema"] = {
                {"type", "object"},
                {"properties", {
                    {"database_name", {
                        {"type", "string"},
                        {"description", "The name of the database as registered in FlameRobin."}
                    }},
                    {"object_name", {
                        {"type", "string"},
                        {"description", "The name of the metadata object (case-insensitive)."}
                    }},
                    {"password", {
                        {"type", "string"},
                        {"description", "Optional connection password if not saved."}
                    }}
                }},
                {"required", json::array({"database_name", "object_name"})}
            };
            tools.push_back(getDdl);

            // get_database_info tool
            json getDbInfo;
            getDbInfo["name"] = "get_database_info";
            getDbInfo["description"] = "Retrieve detailed system information, file pages, sweep, configuration, transaction history, encryption state, and active transaction list of a database.";
            getDbInfo["inputSchema"] = {
                {"type", "object"},
                {"properties", {
                    {"database_name", {
                        {"type", "string"},
                        {"description", "The name of the database as registered in FlameRobin."}
                    }},
                    {"password", {
                        {"type", "string"},
                        {"description", "Optional connection password if not saved."}
                    }}
                }},
                {"required", json::array({"database_name"})}
            };
            tools.push_back(getDbInfo);

            // explain_query tool
            json explainQuery;
            explainQuery["name"] = "explain_query";
            explainQuery["description"] = "Retrieve and analyze Firebird query execution plan (PLAN) and detailed tree explain (EXPLAIN).";
            explainQuery["inputSchema"] = {
                {"type", "object"},
                {"properties", {
                    {"database_name", {
                        {"type", "string"},
                        {"description", "The name of the database as registered in FlameRobin."}
                    }},
                    {"sql", {
                        {"type", "string"},
                        {"description", "The SQL query to explain."}
                    }},
                    {"password", {
                        {"type", "string"},
                        {"description", "Optional connection password if not saved."}
                    }}
                }},
                {"required", json::array({"database_name", "sql"})}
            };
            tools.push_back(explainQuery);

            // compare_schemas tool
            json compareSchemasTool;
            compareSchemasTool["name"] = "compare_schemas";
            compareSchemasTool["description"] = "Compare two Firebird database schemas and generate executable migration DDL script.";
            compareSchemasTool["inputSchema"] = {
                {"type", "object"},
                {"properties", {
                    {"source_database_name", {
                        {"type", "string"},
                        {"description", "The name of the source (reference) database."}
                    }},
                    {"target_database_name", {
                        {"type", "string"},
                        {"description", "The name of the target database to migrate."}
                    }},
                    {"source_password", {
                        {"type", "string"},
                        {"description", "Optional password for source database."}
                    }},
                    {"target_password", {
                        {"type", "string"},
                        {"description", "Optional password for target database."}
                    }},
                    {"generate_drop_statements", {
                        {"type", "boolean"},
                        {"description", "Set true to generate DROP statements for extra columns/objects in target."}
                    }}
                }},
                {"required", json::array({"source_database_name", "target_database_name"})}
            };
            tools.push_back(compareSchemasTool);

            // get_performance_stats tool
            json getPerfStats;
            getPerfStats["name"] = "get_performance_stats";
            getPerfStats["description"] = "Fetch active session metrics, statement counts, active transactions, and memory/page allocations from Firebird MON$ tables.";
            getPerfStats["inputSchema"] = {
                {"type", "object"},
                {"properties", {
                    {"database_name", {
                        {"type", "string"},
                        {"description", "The name of the database as registered in FlameRobin."}
                    }},
                    {"password", {
                        {"type", "string"},
                        {"description", "Optional connection password if not saved."}
                    }}
                }},
                {"required", json::array({"database_name"})}
            };
            tools.push_back(getPerfStats);

            response["result"] = {
                {"tools", tools}
            };
            std::cout << response.dump() << std::endl;
        }
        else if (method == "tools/call")
        {
            std::string toolName = request["params"].value("name", "");
            json args = request["params"].value("arguments", json::object());

            json response;
            response["jsonrpc"] = "2.0";
            response["id"] = id;

            try
            {
                json toolResult;

                if (toolName == "list_databases")
                {
                    std::shared_ptr<Root> root(new Root());
                    root->load();
                    json dbs = json::array();
                    for (const auto& server : root->getServers())
                    {
                        for (const auto& db : server->getDatabases())
                        {
                            json item;
                            item["name"] = wx2std(db->getName_());
                            item["server"] = wx2std(server->getName_());
                            item["connection_string"] = wx2std(db->getConnectionString());
                            item["username"] = wx2std(db->getUsername());
                            item["role"] = wx2std(db->getRole());
                            item["charset"] = wx2std(db->getConnectionCharset());
                            item["dialect"] = db->getSqlDialect();
                            dbs.push_back(item);
                        }
                    }
                    toolResult["databases"] = dbs;
                }
                else if (toolName == "get_schema")
                {
                    std::string dbName = args.value("database_name", "");
                    std::string password = args.value("password", "");

                    std::shared_ptr<Root> root(new Root());
                    root->load();
                    DatabasePtr targetDb;
                    for (const auto& server : root->getServers())
                    {
                        for (const auto& db : server->getDatabases())
                        {
                            if (wx2std(db->getName_()) == dbName)
                            {
                                targetDb = db;
                                break;
                            }
                        }
                        if (targetDb) break;
                    }

                    if (!targetDb)
                    {
                        throw std::runtime_error("Database '" + dbName + "' not found in FlameRobin configuration.");
                    }

                    wxString pwd = targetDb->getDecryptedPassword();
                    if (!password.empty())
                        pwd = wxString::FromUTF8(password.c_str());

                    targetDb->connect(pwd, nullptr);

                    json schema;
                    schema["database"] = dbName;

                    // Tables
                    auto tables = targetDb->getTables();
                    tables->ensureChildrenLoaded();
                    json tableList = json::array();
                    std::vector<MetadataItem*> tableItems;
                    tables->getChildren(tableItems);
                    for (auto* item : tableItems)
                    {
                        auto* table = dynamic_cast<Table*>(item);
                        if (!table) continue;

                        json tbl;
                        tbl["name"] = wx2std(table->getName_());
                        tbl["type"] = "table";

                        table->ensureChildrenLoaded();
                        std::vector<MetadataItem*> colItems;
                        table->getChildren(colItems);

                        json colList = json::array();
                        for (auto* colItem : colItems)
                        {
                            auto* col = dynamic_cast<Column*>(colItem);
                            if (!col) continue;

                            json c;
                            c["name"] = wx2std(col->getName_());
                            c["type"] = wx2std(col->getDatatype());
                            c["nullable"] = col->isNullable(CheckDomainNullability);
                            c["is_primary_key"] = col->isPrimaryKey();
                            c["is_foreign_key"] = col->isForeignKey();
                            colList.push_back(c);
                        }
                        tbl["columns"] = colList;
                        tableList.push_back(tbl);
                    }
                    schema["tables"] = tableList;

                    // Views
                    auto views = targetDb->getViews();
                    views->ensureChildrenLoaded();
                    json viewList = json::array();
                    std::vector<MetadataItem*> viewItems;
                    views->getChildren(viewItems);
                    for (auto* item : viewItems)
                    {
                        auto* view = dynamic_cast<View*>(item);
                        if (!view) continue;

                        json v;
                        v["name"] = wx2std(view->getName_());
                        v["type"] = "view";

                        view->ensureChildrenLoaded();
                        std::vector<MetadataItem*> colItems;
                        view->getChildren(colItems);

                        json colList = json::array();
                        for (auto* colItem : colItems)
                        {
                            auto* col = dynamic_cast<Column*>(colItem);
                            if (!col) continue;

                            json c;
                            c["name"] = wx2std(col->getName_());
                            c["type"] = wx2std(col->getDatatype());
                            c["nullable"] = col->isNullable(CheckDomainNullability);
                            colList.push_back(c);
                        }
                        v["columns"] = colList;
                        viewList.push_back(v);
                    }
                    schema["views"] = viewList;

                    toolResult = schema;
                }
                else if (toolName == "execute_query")
                {
                    std::string dbName = args.value("database_name", "");
                    std::string sql = args.value("sql", "");
                    std::string password = args.value("password", "");

                    std::shared_ptr<Root> root(new Root());
                    root->load();
                    DatabasePtr targetDb;
                    for (const auto& server : root->getServers())
                    {
                        for (const auto& db : server->getDatabases())
                        {
                            if (wx2std(db->getName_()) == dbName)
                            {
                                targetDb = db;
                                break;
                            }
                        }
                        if (targetDb) break;
                    }

                    if (!targetDb)
                    {
                        throw std::runtime_error("Database '" + dbName + "' not found in FlameRobin configuration.");
                    }

                    wxString pwd = targetDb->getDecryptedPassword();
                    if (!password.empty())
                        pwd = wxString::FromUTF8(password.c_str());

                    targetDb->connect(pwd, nullptr);

                    auto dalDb = targetDb->getDALDatabase();
                    auto tr = dalDb->createTransaction();
                    tr->start();

                    auto st = dalDb->createStatement(tr);
                    st->prepare(sql);
                    st->execute();

                    json rows = json::array();
                    int colCount = st->getColumnCount();
                    if (colCount > 0)
                    {
                        while (st->fetch())
                        {
                            json row;
                            for (int i = 0; i < colCount; ++i)
                            {
                                std::string colName = st->getColumnName(i);
                                if (st->isNull(i))
                                {
                                    row[colName] = nullptr;
                                }
                                else
                                {
                                    fr::ColumnType colType = st->getColumnType(i);
                                    switch (colType)
                                    {
                                        case fr::ColumnType::Integer:
                                            row[colName] = st->getInt32(i);
                                            break;
                                        case fr::ColumnType::BigInt:
                                            row[colName] = st->getInt64(i);
                                            break;
                                        case fr::ColumnType::Float:
                                        case fr::ColumnType::Double:
                                            row[colName] = st->getDouble(i);
                                            break;
                                        case fr::ColumnType::Boolean:
                                            row[colName] = st->getBool(i);
                                            break;
                                        default:
                                            row[colName] = st->getString(i);
                                            break;
                                    }
                                }
                            }
                            rows.push_back(row);
                        }
                    }

                    tr->commit();

                    toolResult["rows"] = rows;
                    toolResult["column_count"] = colCount;
                    toolResult["affected_rows"] = st->getAffectedRows();
                }
                else if (toolName == "get_metadata_ddl")
                {
                    std::string dbName = args.value("database_name", "");
                    std::string objName = args.value("object_name", "");
                    std::string password = args.value("password", "");

                    std::shared_ptr<Root> root(new Root());
                    root->load();
                    DatabasePtr targetDb;
                    for (const auto& server : root->getServers())
                    {
                        for (const auto& db : server->getDatabases())
                        {
                            if (wx2std(db->getName_()) == dbName)
                            {
                                targetDb = db;
                                break;
                            }
                        }
                        if (targetDb) break;
                    }

                    if (!targetDb)
                    {
                        throw std::runtime_error("Database '" + dbName + "' not found in FlameRobin configuration.");
                    }

                    wxString pwd = targetDb->getDecryptedPassword();
                    if (!password.empty())
                        pwd = wxString::FromUTF8(password.c_str());

                    targetDb->connect(pwd, nullptr);

                    MetadataItem* obj = findMetadataObject(targetDb, objName);
                    if (!obj)
                    {
                        throw std::runtime_error("Metadata object '" + objName + "' not found in database '" + dbName + "'.");
                    }

                    CreateDDLVisitor cdv;
                    obj->acceptVisitor(&cdv);
                    
                    toolResult["object_name"] = objName;
                    toolResult["ddl"] = wx2std(cdv.getSql());
                }
                else if (toolName == "get_database_info")
                {
                    std::string dbName = args.value("database_name", "");
                    std::string password = args.value("password", "");

                    std::shared_ptr<Root> root(new Root());
                    root->load();
                    DatabasePtr targetDb;
                    for (const auto& server : root->getServers())
                    {
                        for (const auto& db : server->getDatabases())
                        {
                            if (wx2std(db->getName_()) == dbName)
                            {
                                targetDb = db;
                                break;
                            }
                        }
                        if (targetDb) break;
                    }

                    if (!targetDb)
                    {
                        throw std::runtime_error("Database '" + dbName + "' not found in FlameRobin configuration.");
                    }

                    wxString pwd = targetDb->getDecryptedPassword();
                    if (!password.empty())
                        pwd = wxString::FromUTF8(password.c_str());

                    targetDb->connect(pwd, nullptr);

                    auto dalDb = targetDb->getDALDatabase();
                    fr::DatabaseInfoData info;
                    dalDb->getInfo(&info);

                    json dbInfo;
                    dbInfo["ods_version"] = std::to_string(info.ods) + "." + std::to_string(info.odsMinor);
                    dbInfo["page_size"] = info.pageSize;
                    dbInfo["pages_allocated"] = info.pages;
                    dbInfo["buffers"] = info.buffers;
                    dbInfo["sweep_interval"] = info.sweep;
                    dbInfo["forced_writes"] = info.forcedWrites;
                    dbInfo["reserve_space"] = info.reserve;
                    dbInfo["read_only"] = info.readOnly;
                    dbInfo["oldest_transaction"] = info.oldestTransaction;
                    dbInfo["oldest_active_transaction"] = info.oldestActiveTransaction;
                    dbInfo["oldest_snapshot"] = info.oldestSnapshot;
                    dbInfo["next_transaction"] = info.nextTransaction;
                    dbInfo["crypt_state"] = wx2std(cryptStateToString(info.cryptState));

                    json activeTxs = json::array();
                    for (const auto& tx : info.activeTransactions)
                    {
                        json t;
                        t["id"] = tx.id;
                        t["isolation_level"] = wx2std(isolationLevelToString(tx.isolationLevel));
                        t["read_only"] = tx.readOnly;
                        t["wait"] = tx.wait;
                        activeTxs.push_back(t);
                    }
                    dbInfo["active_transactions"] = activeTxs;

                    toolResult = dbInfo;
                }
                else if (toolName == "explain_query")
                {
                    std::string dbName = args.value("database_name", "");
                    std::string sqlStr = args.value("sql", "");
                    std::string password = args.value("password", "");

                    std::shared_ptr<Root> root(new Root());
                    root->load();
                    DatabasePtr targetDb;
                    for (const auto& server : root->getServers())
                    {
                        for (const auto& db : server->getDatabases())
                        {
                            if (wx2std(db->getName_()) == dbName)
                            {
                                targetDb = db;
                                break;
                            }
                        }
                        if (targetDb) break;
                    }

                    if (!targetDb)
                        throw std::runtime_error("Database '" + dbName + "' not found in FlameRobin configuration.");

                    wxString pwd = targetDb->getDecryptedPassword();
                    if (!password.empty())
                        pwd = wxString::FromUTF8(password.c_str());

                    targetDb->connect(pwd, nullptr);

                    auto dalDb = targetDb->getDALDatabase();
                    auto tr = dalDb->createTransaction();
                    tr->start();
                    auto st = dalDb->createStatement(tr);
                    st->prepare(sqlStr);

                    std::string plan = st->getPlan();
                    tr->rollback();

                    toolResult["plan"] = plan;
                }
                else if (toolName == "compare_schemas")
                {
                    std::string srcDbName = args.value("source_database_name", "");
                    std::string tgtDbName = args.value("target_database_name", "");
                    std::string srcPwdStr = args.value("source_password", "");
                    std::string tgtPwdStr = args.value("target_password", "");
                    bool genDrop = args.value("generate_drop_statements", false);

                    std::shared_ptr<Root> root(new Root());
                    root->load();

                    DatabasePtr srcDb;
                    DatabasePtr tgtDb;
                    for (const auto& server : root->getServers())
                    {
                        for (const auto& db : server->getDatabases())
                        {
                            if (wx2std(db->getName_()) == srcDbName)
                                srcDb = db;
                            if (wx2std(db->getName_()) == tgtDbName)
                                tgtDb = db;
                        }
                    }

                    if (!srcDb)
                        throw std::runtime_error("Source database '" + srcDbName + "' not found.");
                    if (!tgtDb)
                        throw std::runtime_error("Target database '" + tgtDbName + "' not found.");

                    wxString srcPwd = srcDb->getDecryptedPassword();
                    if (!srcPwdStr.empty()) srcPwd = wxString::FromUTF8(srcPwdStr.c_str());
                    srcDb->connect(srcPwd, nullptr);

                    wxString tgtPwd = tgtDb->getDecryptedPassword();
                    if (!tgtPwdStr.empty()) tgtPwd = wxString::FromUTF8(tgtPwdStr.c_str());
                    tgtDb->connect(tgtPwd, nullptr);

                    SchemaDiffOptions opts;
                    opts.generateDropStatements = genDrop;

                    auto diffs = SchemaDiff::compareDatabases(srcDb.get(), tgtDb.get(), opts);
                    std::string migrationScript = wx2std(SchemaDiff::generateMigrationScript(diffs, srcDb.get(), tgtDb.get()));

                    json diffList = json::array();
                    for (const auto& item : diffs)
                    {
                        json d;
                        d["object_type"] = wx2std(item.objectType);
                        d["object_name"] = wx2std(item.objectName);
                        d["description"] = wx2std(item.description);
                        d["migration_sql"] = wx2std(item.migrationSql);
                        diffList.push_back(d);
                    }

                    toolResult["differences_count"] = diffs.size();
                    toolResult["migration_sql"] = migrationScript;
                    toolResult["differences"] = diffList;
                }
                else if (toolName == "get_performance_stats")
                {
                    std::string dbName = args.value("database_name", "");
                    std::string password = args.value("password", "");

                    std::shared_ptr<Root> root(new Root());
                    root->load();
                    DatabasePtr targetDb;
                    for (const auto& server : root->getServers())
                    {
                        for (const auto& db : server->getDatabases())
                        {
                            if (wx2std(db->getName_()) == dbName)
                            {
                                targetDb = db;
                                break;
                            }
                        }
                        if (targetDb) break;
                    }

                    if (!targetDb)
                        throw std::runtime_error("Database '" + dbName + "' not found in FlameRobin configuration.");

                    wxString pwd = targetDb->getDecryptedPassword();
                    if (!password.empty())
                        pwd = wxString::FromUTF8(password.c_str());

                    targetDb->connect(pwd, nullptr);

                    auto dalDb = targetDb->getDALDatabase();
                    json perf;

                    try
                    {
                        auto tr = dalDb->createTransaction();
                        tr->start();
                        auto st = dalDb->createStatement(tr);
                        std::string monSql = "SELECT (SELECT COUNT(*) FROM MON$ATTACHMENTS WHERE MON$ATTACHMENT_ID <> CURRENT_ATTACHMENT_ID) AS active_attachments, "
                            "(SELECT COUNT(*) FROM MON$STATEMENTS WHERE MON$STATE = 1) AS active_statements, "
                            "(SELECT COUNT(*) FROM MON$TRANSACTIONS WHERE MON$STATE = 1) AS active_transactions "
                            "FROM RDB$DATABASE;";
                        st->prepare(monSql);
                        st->execute();

                        if (st->fetch())
                        {
                            perf["active_attachments"] = st->getInt32(0);
                            perf["active_statements"] = st->getInt32(1);
                            perf["active_transactions"] = st->getInt32(2);
                        }
                        tr->commit();
                    }
                    catch (const std::exception&)
                    {
                        perf["active_attachments"] = 0;
                        perf["active_statements"] = 0;
                        perf["active_transactions"] = 0;
                    }

                    fr::DatabaseInfoData info;
                    dalDb->getInfo(&info);
                    perf["page_size"] = info.pageSize;
                    perf["pages_allocated"] = info.pages;
                    perf["buffers"] = info.buffers;
                    perf["oldest_transaction"] = info.oldestTransaction;
                    perf["oldest_active_transaction"] = info.oldestActiveTransaction;
                    perf["next_transaction"] = info.nextTransaction;

                    toolResult = perf;
                }
                else
                {
                    throw std::runtime_error("Unknown tool: " + toolName);
                }

                // Send tool execution success response
                json contentItem;
                contentItem["type"] = "text";
                contentItem["text"] = toolResult.dump(2);

                response["result"] = {
                    {"content", json::array({contentItem})},
                    {"isError", false}
                };
                std::cout << response.dump() << std::endl;
            }
            catch (const std::exception& e)
            {
                // Send tool execution error response
                json contentItem;
                contentItem["type"] = "text";
                contentItem["text"] = std::string("Error during execution: ") + e.what();

                response["result"] = {
                    {"content", json::array({contentItem})},
                    {"isError", true}
                };
                std::cout << response.dump() << std::endl;
            }
        }
        else
        {
            json errResponse;
            errResponse["jsonrpc"] = "2.0";
            errResponse["id"] = id;
            errResponse["error"] = {
                {"code", -32601},
                {"message", "Method not found: " + method}
            };
            std::cout << errResponse.dump() << std::endl;
        }
    }
}

} // namespace fr
