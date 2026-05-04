#include <iostream>
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include "sql/SqlStatement.h"
#include "sql/Identifier.h"
#include "metadata/database.h"
#include "metadata/relation.h"
#include "metadata/procedure.h"
#include "metadata/table.h"
#include "metadata/view.h"
#include "frutils.h"

// --- MINIMAL STUBS TO ALLOW LINKING WITHOUT FULL METADATA ENGINE ---

MetadataItem::MetadataItem() : parentM(0), typeM(ntUnknown), metadataIdM(-1) {}
MetadataItem::MetadataItem(NodeType type, MetadataItem* parent, const wxString& name, int id) 
    : parentM(parent), typeM(type), identifierM(name), metadataIdM(id) {}
MetadataItem::~MetadataItem() {}
wxString MetadataItem::getName_() const { return identifierM.get(); }
NodeType MetadataItem::getType() const { return typeM; }
void MetadataItem::ensureChildrenLoaded() {}

Relation::Relation(NodeType type, DatabasePtr database, const wxString& name) 
    : MetadataItem(type, (MetadataItem*)database.get(), name, 0) {}
ColumnPtr Relation::findColumn(const wxString&) const { return ColumnPtr(); }

Table::Table(DatabasePtr database, const wxString& name) 
    : Relation(ntTable, database, name) {}

View::View(DatabasePtr database, const wxString& name) 
    : Relation(ntView, database, name) {}

Procedure::Procedure(DatabasePtr database, const wxString& name) 
    : MetadataItem(ntProcedure, (MetadataItem*)database.get(), name, 0) {}
ParameterPtr Procedure::findParameter(const wxString&) const { return ParameterPtr(); }

MetadataItem* Database::findByNameAndType(NodeType, const wxString&) { return nullptr; }
Relation* Database::findRelation(const Identifier&) { return nullptr; }
CharacterSetPtr Database::getCharsetById(int) { return CharacterSetPtr(); }

wxString unquote(const wxString& s, const wxString&) { return s; }

// --- TEST CODE ---

namespace
{
bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << "\n";
        return true;
    }
    std::cerr << "[FAIL] " << testName << "\n";
    return false;
}
}

int main()
{
    bool ok = true;
    
    std::cout << "Testing Firebird 5.0 MERGE statement recognition...\n";

    // Test 1: Standard MERGE syntax
    {
        wxString sql = "MERGE INTO target t USING source s ON t.id = s.id WHEN MATCHED THEN UPDATE SET t.val = s.val";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "target", "Standard MERGE: target name recognition") && ok;
        ok = check(stm.getAction() == actMERGE, "Standard MERGE: identified as actMERGE") && ok;
        ok = check(!stm.isDDL(), "Standard MERGE: not identified as DDL") && ok;
    }

    // Test 2: MERGE without INTO (Firebird shorthand)
    {
        wxString sql = "MERGE target t USING source s ON t.id = s.id WHEN MATCHED THEN DELETE";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "target", "MERGE without INTO: target name recognition") && ok;
        ok = check(stm.getAction() == actMERGE, "MERGE without INTO: identified as actMERGE") && ok;
    }

    // Test 3: Firebird 5.0 extended syntax (WHEN NOT MATCHED BY SOURCE)
    {
        wxString sql = "MERGE target t USING source s ON t.id = s.id WHEN NOT MATCHED BY SOURCE THEN DELETE";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "target", "MERGE 5.0: target name recognition") && ok;
        ok = check(stm.getAction() == actMERGE, "MERGE 5.0: identified as actMERGE") && ok;
    }

    // Test 4: Full MERGE with all clause types
    {
        wxString sql = "MERGE INTO target t USING source s ON t.id = s.id "
                       "WHEN MATCHED AND t.val IS DISTINCT FROM s.val THEN UPDATE SET t.val = s.val "
                       "WHEN NOT MATCHED BY TARGET THEN INSERT (id, val) VALUES (s.id, s.val) "
                       "WHEN NOT MATCHED BY SOURCE THEN DELETE";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "target", "Full MERGE: target name recognition") && ok;
        ok = check(stm.getAction() == actMERGE, "Full MERGE: identified as actMERGE") && ok;
    }

    // Test 5: MERGE where target is a keyword (e.g., "SOURCE" used as table name)
    {
        wxString sql = "MERGE INTO SOURCE s USING source_table src ON s.id = src.id WHEN MATCHED THEN DELETE";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "SOURCE", "MERGE keyword target (INTO): name recognition") && ok;
        ok = check(stm.getAction() == actMERGE, "MERGE keyword target (INTO): identified as actMERGE") && ok;
    }

    // Test 6: MERGE without INTO where target is a keyword
    {
        wxString sql = "MERGE SOURCE s USING source_table src ON s.id = src.id WHEN MATCHED THEN DELETE";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "SOURCE", "MERGE keyword target (no INTO): name recognition") && ok;
        ok = check(stm.getAction() == actMERGE, "MERGE keyword target (no INTO): identified as actMERGE") && ok;
    }

    if (ok)
        std::cout << "\nAll MERGE recognition tests passed.\n";
    else
        std::cout << "\nSome tests failed.\n";

    return ok ? 0 : 1;
}

