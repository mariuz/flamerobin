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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/SchemaDiff.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/column.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/generator.h"
#include "metadata/Index.h"
#include "metadata/procedure.h"
#include "metadata/relation.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"

std::vector<SchemaDiffItem> SchemaDiff::compareDatabases(
    Database* sourceDb, Database* targetDb, const SchemaDiffOptions& options)
{
    std::vector<SchemaDiffItem> diffs;
    if (!sourceDb || !targetDb)
        return diffs;

    sourceDb->ensureChildrenLoaded();
    targetDb->ensureChildrenLoaded();

    // 1. Domains
    if (options.compareDomains)
    {
        DomainsPtr domains = sourceDb->getDomains();
        if (domains)
        {
            for (auto it = domains->begin(); it != domains->end(); ++it)
            {
                Domain* d = dynamic_cast<Domain*>((*it).get());
                if (!d || d->isSystem()) continue;

                if (!targetDb->findByName(d->getName_()))
                {
                    SchemaDiffItem item;
                    item.objectType = "Domain";
                    item.objectName = d->getName_();
                    item.diffType = SchemaDiffItem::diffMissingInTarget;
                    item.description = wxString::Format(_("Domain '%s' missing in target database"), d->getName_().c_str());
                    item.migrationSql = wxString::Format("CREATE DOMAIN %s AS %s;\n",
                        d->getQuotedName().c_str(), d->getDatatypeAsString().c_str());
                    diffs.push_back(item);
                }
            }
        }
    }

    // 2. Generators / Sequences
    if (options.compareGenerators)
    {
        GeneratorsPtr gens = sourceDb->getGenerators();
        if (gens)
        {
            for (auto it = gens->begin(); it != gens->end(); ++it)
            {
                Generator* g = dynamic_cast<Generator*>((*it).get());
                if (!g || g->isSystem()) continue;

                if (!targetDb->findByName(g->getName_()))
                {
                    SchemaDiffItem item;
                    item.objectType = "Generator";
                    item.objectName = g->getName_();
                    item.diffType = SchemaDiffItem::diffMissingInTarget;
                    item.description = wxString::Format(_("Sequence/Generator '%s' missing in target database"), g->getName_().c_str());
                    item.migrationSql = wxString::Format("CREATE SEQUENCE %s;\n", g->getQuotedName().c_str());
                    diffs.push_back(item);
                }
            }
        }
    }

    // 3. Exceptions
    if (options.compareExceptions)
    {
        ExceptionsPtr exes = sourceDb->getExceptions();
        if (exes)
        {
            for (auto it = exes->begin(); it != exes->end(); ++it)
            {
                Exception* e = dynamic_cast<Exception*>((*it).get());
                if (!e || e->isSystem()) continue;

                if (!targetDb->findByName(e->getName_()))
                {
                    SchemaDiffItem item;
                    item.objectType = "Exception";
                    item.objectName = e->getName_();
                    item.diffType = SchemaDiffItem::diffMissingInTarget;
                    item.description = wxString::Format(_("Exception '%s' missing in target database"), e->getName_().c_str());
                    item.migrationSql = wxString::Format("CREATE EXCEPTION %s '%s';\n",
                        e->getQuotedName().c_str(), e->getMessage().c_str());
                    diffs.push_back(item);
                }
            }
        }
    }

    // 4. Tables and Columns
    if (options.compareTables)
    {
        TablesPtr tables = sourceDb->getTables();
        if (tables)
        {
            for (auto it = tables->begin(); it != tables->end(); ++it)
            {
                Table* srcTbl = dynamic_cast<Table*>((*it).get());
                if (!srcTbl || srcTbl->isSystem()) continue;

                Table* tgtTbl = dynamic_cast<Table*>(targetDb->findRelation(srcTbl->getName_()));
                if (!tgtTbl)
                {
                    SchemaDiffItem item;
                    item.objectType = "Table";
                    item.objectName = srcTbl->getName_();
                    item.diffType = SchemaDiffItem::diffMissingInTarget;
                    item.description = wxString::Format(_("Table '%s' missing in target database"), srcTbl->getName_().c_str());

                    CreateDDLVisitor cdv(0);
                    srcTbl->acceptVisitor(&cdv);
                    item.migrationSql = cdv.getSql() + "\n";
                    diffs.push_back(item);
                }
                else if (options.compareColumns)
                {
                    srcTbl->ensureChildrenLoaded();
                    tgtTbl->ensureChildrenLoaded();

                    for (auto cit = srcTbl->begin(); cit != srcTbl->end(); ++cit)
                    {
                        Column* srcCol = dynamic_cast<Column*>((*cit).get());
                        if (!srcCol) continue;
                        ColumnPtr tgtCol = tgtTbl->findColumn(srcCol->getName_());
                        if (!tgtCol)
                        {
                            SchemaDiffItem item;
                            item.objectType = "Column";
                            item.objectName = srcTbl->getName_() + "." + srcCol->getName_();
                            item.diffType = SchemaDiffItem::diffMissingInTarget;
                            item.description = wxString::Format(_("Column '%s' in table '%s' missing in target database"),
                                srcCol->getName_().c_str(), srcTbl->getName_().c_str());
                            item.migrationSql = wxString::Format("ALTER TABLE %s ADD %s %s;\n",
                                srcTbl->getQuotedName().c_str(),
                                srcCol->getQuotedName().c_str(),
                                srcCol->getDatatype().c_str());
                            diffs.push_back(item);
                        }
                        else if (srcCol->getDatatype() != tgtCol->getDatatype())
                        {
                            SchemaDiffItem item;
                            item.objectType = "Column";
                            item.objectName = srcTbl->getName_() + "." + srcCol->getName_();
                            item.diffType = SchemaDiffItem::diffModified;
                            item.description = wxString::Format(_("Column '%s' in table '%s' type mismatch (%s vs %s)"),
                                srcCol->getName_().c_str(), srcTbl->getName_().c_str(),
                                srcCol->getDatatype().c_str(), tgtCol->getDatatype().c_str());
                            item.migrationSql = wxString::Format("ALTER TABLE %s ALTER COLUMN %s TYPE %s;\n",
                                srcTbl->getQuotedName().c_str(),
                                srcCol->getQuotedName().c_str(),
                                srcCol->getDatatype().c_str());
                            diffs.push_back(item);
                        }
                    }

                    if (options.generateDropStatements)
                    {
                        for (auto cit = tgtTbl->begin(); cit != tgtTbl->end(); ++cit)
                        {
                            Column* tgtCol = dynamic_cast<Column*>((*cit).get());
                            if (!tgtCol) continue;
                            if (!srcTbl->findColumn(tgtCol->getName_()))
                            {
                                SchemaDiffItem item;
                                item.objectType = "Column";
                                item.objectName = tgtTbl->getName_() + "." + tgtCol->getName_();
                                item.diffType = SchemaDiffItem::diffExtraInTarget;
                                item.description = wxString::Format(_("Column '%s' in target table '%s' does not exist in source"),
                                    tgtCol->getName_().c_str(), tgtTbl->getName_().c_str());
                                item.migrationSql = wxString::Format("ALTER TABLE %s DROP %s;\n",
                                    tgtTbl->getQuotedName().c_str(),
                                    tgtCol->getQuotedName().c_str());
                                diffs.push_back(item);
                            }
                        }
                    }
                }
            }
        }
    }

    // 5. Indices
    if (options.compareIndices)
    {
        TablesPtr tables = sourceDb->getTables();
        if (tables)
        {
            for (auto it = tables->begin(); it != tables->end(); ++it)
            {
                Table* srcTbl = dynamic_cast<Table*>((*it).get());
                if (!srcTbl || srcTbl->isSystem()) continue;
                srcTbl->ensureChildrenLoaded();

                std::vector<Index>* srcIndices = srcTbl->getIndices();
                if (!srcIndices) continue;

                Table* tgtTbl = dynamic_cast<Table*>(targetDb->findRelation(srcTbl->getName_()));
                if (!tgtTbl) continue;
                tgtTbl->ensureChildrenLoaded();

                std::vector<Index>* tgtIndices = tgtTbl->getIndices();

                for (auto& idx : *srcIndices)
                {
                    if (idx.isSystem()) continue;
                    bool found = false;
                    if (tgtIndices)
                    {
                        for (auto& ti : *tgtIndices)
                        {
                            if (ti.getName_() == idx.getName_())
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found)
                    {
                        SchemaDiffItem item;
                        item.objectType = "Index";
                        item.objectName = idx.getName_();
                        item.diffType = SchemaDiffItem::diffMissingInTarget;
                        item.description = wxString::Format(_("Index '%s' on table '%s' missing in target database"),
                            idx.getName_().c_str(), srcTbl->getName_().c_str());

                        CreateDDLVisitor cdv(0);
                        idx.acceptVisitor(&cdv);
                        item.migrationSql = cdv.getSql() + "\n";
                        diffs.push_back(item);
                    }
                }
            }
        }
    }

    // 6. Procedures
    if (options.compareProcedures)
    {
        ProceduresPtr procs = sourceDb->getProcedures();
        if (procs)
        {
            for (auto it = procs->begin(); it != procs->end(); ++it)
            {
                Procedure* p = dynamic_cast<Procedure*>((*it).get());
                if (!p || p->isSystem()) continue;

                Procedure* tgtP = dynamic_cast<Procedure*>(targetDb->findByName(p->getName_()));
                if (!tgtP)
                {
                    SchemaDiffItem item;
                    item.objectType = "Procedure";
                    item.objectName = p->getName_();
                    item.diffType = SchemaDiffItem::diffMissingInTarget;
                    item.description = wxString::Format(_("Procedure '%s' missing in target database"), p->getName_().c_str());

                    CreateDDLVisitor cdv(0);
                    p->acceptVisitor(&cdv);
                    wxString sql = cdv.getSql();
                    sql.Replace("CREATE PROCEDURE", "CREATE OR ALTER PROCEDURE");
                    item.migrationSql = sql + "\n";
                    diffs.push_back(item);
                }
            }
        }
    }

    // 7. Views
    if (options.compareViews)
    {
        ViewsPtr views = sourceDb->getViews();
        if (views)
        {
            for (auto it = views->begin(); it != views->end(); ++it)
            {
                View* v = dynamic_cast<View*>((*it).get());
                if (!v || v->isSystem()) continue;

                View* tgtV = dynamic_cast<View*>(targetDb->findRelation(v->getName_()));
                if (!tgtV)
                {
                    SchemaDiffItem item;
                    item.objectType = "View";
                    item.objectName = v->getName_();
                    item.diffType = SchemaDiffItem::diffMissingInTarget;
                    item.description = wxString::Format(_("View '%s' missing in target database"), v->getName_().c_str());

                    CreateDDLVisitor cdv(0);
                    v->acceptVisitor(&cdv);
                    wxString sql = cdv.getSql();
                    sql.Replace("CREATE VIEW", "CREATE OR ALTER VIEW");
                    item.migrationSql = sql + "\n";
                    diffs.push_back(item);
                }
            }
        }
    }

    // 8. Triggers
    if (options.compareTriggers)
    {
        DBTriggersPtr triggers = sourceDb->getDBTriggers();
        if (triggers)
        {
            for (auto it = triggers->begin(); it != triggers->end(); ++it)
            {
                Trigger* tr = dynamic_cast<Trigger*>((*it).get());
                if (!tr || tr->isSystem()) continue;

                Trigger* tgtTr = dynamic_cast<Trigger*>(targetDb->findByName(tr->getName_()));
                if (!tgtTr)
                {
                    SchemaDiffItem item;
                    item.objectType = "Trigger";
                    item.objectName = tr->getName_();
                    item.diffType = SchemaDiffItem::diffMissingInTarget;
                    item.description = wxString::Format(_("Trigger '%s' missing in target database"), tr->getName_().c_str());

                    CreateDDLVisitor cdv(0);
                    tr->acceptVisitor(&cdv);
                    wxString sql = cdv.getSql();
                    sql.Replace("CREATE TRIGGER", "CREATE OR ALTER TRIGGER");
                    item.migrationSql = sql + "\n";
                    diffs.push_back(item);
                }
            }
        }
    }

    return diffs;
}

wxString SchemaDiff::generateMigrationScript(
    const std::vector<SchemaDiffItem>& diffItems, Database* sourceDb, Database* targetDb)
{
    wxString script;
    script << "/* ====================================================================\n";
    script << "   FlameRobin Schema Migration Script\n";
    script << "   Source Database: " << (sourceDb ? sourceDb->getName_() : wxString("Unknown")) << "\n";
    script << "   Target Database: " << (targetDb ? targetDb->getName_() : wxString("Unknown")) << "\n";
    script << "   Generated: " << wxDateTime::Now().FormatISOCombined(' ') << "\n";
    script << "   Total Changes: " << diffItems.size() << "\n";
    script << "   ==================================================================== */\n\n";

    if (diffItems.empty())
    {
        script << "-- No schema differences detected between source and target databases.\n";
        return script;
    }

    script << "SET TERM ^ ;\n\n";
    for (const auto& item : diffItems)
    {
        script << "/* --- " << item.objectType << ": " << item.objectName << " --- */\n";
        script << "-- " << item.description << "\n";
        script << item.migrationSql << "\n";
    }
    script << "SET TERM ; ^\n";

    return script;
}
