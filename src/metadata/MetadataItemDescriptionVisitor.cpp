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

#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/Index.h"
#include "metadata/metadataitem.h"
#include "metadata/MetadataItemDescriptionVisitor.h"
#include "metadata/parameter.h"
#include "metadata/package.h"
#include "metadata/procedure.h"
#include "metadata/relation.h"
#include "metadata/role.h"
#include "metadata/trigger.h"


// class LoadDescriptionVisitor
LoadDescriptionVisitor::LoadDescriptionVisitor()
    : MetadataItemVisitor(), availableM(false)
{
}

bool LoadDescriptionVisitor::descriptionAvailable() const
{
    return availableM;
}

wxString LoadDescriptionVisitor::getDescription() const
{
    return descriptionM;
}

void LoadDescriptionVisitor::loadDescription(MetadataItem* object,
    const std::string& statement)
{
    loadDescription(object, 0, statement);
}

void LoadDescriptionVisitor::loadDescription(MetadataItem* object,
    MetadataItem* parent, const std::string& statement)
{
    descriptionM = wxEmptyString;
    availableM = false;

    wxASSERT(object);
    DatabasePtr db = object->getDatabase();
    wxMBConv* csConverter = db->getCharsetConverter();

    try
    {
        MetadataLoader* loader = db->getMetadataLoader();
        MetadataLoaderTransaction tr(loader);
        IBPP::Statement& st1 = loader->getStatement(statement);

        st1->Set(1, wx2std(object->getName_(), csConverter));
        // relation column or SP parameter?
        if (parent)
            st1->Set(2, wx2std(parent->getName_(), csConverter));
        st1->Execute();
        st1->Fetch();

        std::string value;
        if (!st1->IsNull(1))
        {
            IBPP::Blob b = loader->createBlob();
            st1->Get(1, b);
            b->Load(value);
            descriptionM = wxString(value.c_str(), *csConverter);
        }
        else
            descriptionM = wxEmptyString;
        availableM = true;
    }
    catch (IBPP::SQLException &e)
    {
        // FB 2.0 supports descriptions for some objects that previous
        // FB versions don't
        if (e.SqlCode() != -206) // column does not belong to referenced table.
            throw;
    }
}

void LoadDescriptionVisitor::visitColumn(Column& column)
{
    // TODO: use Column::GetRelation() / Column::getProcedure() instead
    loadDescription(&column, column.getParent(),
        "select RDB$DESCRIPTION from RDB$RELATION_FIELDS "
        "where RDB$FIELD_NAME = ? and RDB$RELATION_NAME = ?");
}

void LoadDescriptionVisitor::visitDomain(Domain& domain)
{
    loadDescription(&domain,
        "select RDB$DESCRIPTION from RDB$FIELDS "
        "where RDB$FIELD_NAME = ?");
}

void LoadDescriptionVisitor::visitException(Exception& exception)
{
    loadDescription(&exception,
        "select RDB$DESCRIPTION from RDB$EXCEPTIONS "
        "where RDB$EXCEPTION_NAME = ?");
}

void LoadDescriptionVisitor::visitFunctionSQL(FunctionSQL& function)
{
    loadDescription(&function,
        "select RDB$DESCRIPTION from RDB$FUNCTIONS "
        "where RDB$FUNCTION_NAME = ?");
}

void LoadDescriptionVisitor::visitUDF(UDF& function)
{
    loadDescription(&function,
        "select RDB$DESCRIPTION from RDB$FUNCTIONS "
        "where RDB$FUNCTION_NAME = ?");
}

void LoadDescriptionVisitor::visitGenerator(Generator& generator)
{
    loadDescription(&generator,
        "select RDB$DESCRIPTION from RDB$GENERATORS "
        "where RDB$GENERATOR_NAME = ?");
}

void LoadDescriptionVisitor::visitIndex(Index& index)
{
    loadDescription(&index,
        "select RDB$DESCRIPTION from RDB$INDICES "
        "where RDB$INDEX_NAME = ?");
}

void LoadDescriptionVisitor::visitParameter(Parameter& parameter)
{
    // TODO: use Parameter::getProcedure() instead
    loadDescription(&parameter, parameter.getParent(),
        "select RDB$DESCRIPTION from RDB$PROCEDURE_PARAMETERS "
        "where RDB$PARAMETER_NAME = ? and RDB$PROCEDURE_NAME = ?");
}

void LoadDescriptionVisitor::visitPackage(Package& package)
{
    loadDescription(&package,
        "select RDB$DESCRIPTION from RDB$PACKAGES "
        "where RDB$PACLAGE_NAME = ?");
}

void LoadDescriptionVisitor::visitProcedure(Procedure& procedure)
{
    loadDescription(&procedure,
        "select RDB$DESCRIPTION from RDB$PROCEDURES "
        "where RDB$PROCEDURE_NAME = ?");
}

void LoadDescriptionVisitor::visitRelation(Relation& relation)
{
    loadDescription(&relation,
        "select RDB$DESCRIPTION from RDB$RELATIONS "
        "where RDB$RELATION_NAME = ?");
}

void LoadDescriptionVisitor::visitRole(Role& role)
{
    loadDescription(&role,
        "select RDB$DESCRIPTION from RDB$ROLES "
        "where RDB$ROLE_NAME = ?");
}

void LoadDescriptionVisitor::visitTrigger(Trigger& trigger)
{
    loadDescription(&trigger,
        "select RDB$DESCRIPTION from RDB$TRIGGERS "
        "where RDB$TRIGGER_NAME = ?");
}

// class SaveDescriptionVisitor
SaveDescriptionVisitor::SaveDescriptionVisitor(wxString description)
    : MetadataItemVisitor(), descriptionM(description)
{
}

void SaveDescriptionVisitor::saveDescription(MetadataItem* object,
    const std::string& statement)
{
    saveDescription(object, 0, statement);
}

void SaveDescriptionVisitor::saveDescription(MetadataItem* object,
    MetadataItem* parent, const std::string& statement)
{
    wxASSERT(object);
    DatabasePtr d = object->getDatabase();
    wxMBConv* csConverter = d->getCharsetConverter();

    IBPP::Database& db = d->getIBPPDatabase();
    IBPP::Transaction tr1 = IBPP::TransactionFactory(db);
    tr1->Start();

    IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
	if (d->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) {
		descriptionM.Replace("'", "''");
		wxString s;
		if (parent)
		  s = s.Format(wxString(statement), parent->getQuotedName(), object->getQuotedName(), descriptionM);
		else
			s = s.Format(wxString(statement), object->getQuotedName(), descriptionM);

		st1->Prepare(wx2std(s, csConverter));
	}
	else {
		st1->Prepare(statement);

		if (!descriptionM.empty())
		{
			IBPP::Blob b = IBPP::BlobFactory(db, tr1);
			b->Save(wx2std(descriptionM, csConverter));
			st1->Set(1, b);
		}
		else
			st1->SetNull(1);
		st1->Set(2, wx2std(object->getName_(), csConverter));
		// relation column or SP parameter?
		if (parent)
			st1->Set(3, wx2std(parent->getName_(), csConverter));
	}

    st1->Execute();
    tr1->Commit();
}

void SaveDescriptionVisitor::visitColumn(Column& column)
{
	if (column.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11,1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&column, column.getParent(), "comment on column %s.%s is '%s'");
		return;
	}
    // TODO: use Column::GetRelation() / Column::getProcedure() instead
    saveDescription(&column, column.getParent(),
        "update RDB$RELATION_FIELDS set RDB$DESCRIPTION = ? "
        "where RDB$FIELD_NAME = ? and RDB$RELATION_NAME = ?");
}

void SaveDescriptionVisitor::visitDomain(Domain& domain)
{
	if (domain.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&domain, "comment on domain %s is '%s'");
		return;
	}
    saveDescription(&domain,
        "update RDB$FIELDS set RDB$DESCRIPTION = ? "
        "where RDB$FIELD_NAME = ?");
}

void SaveDescriptionVisitor::visitException(Exception& exception)
{
	if (exception.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&exception, "comment on exception %s is '%s'");
		return;
	}
    saveDescription(&exception,
        "update RDB$EXCEPTIONS set RDB$DESCRIPTION = ? "
        "where RDB$EXCEPTION_NAME = ?");
}

void SaveDescriptionVisitor::visitFunctionSQL(FunctionSQL& function)
{
    saveDescription(&function,
        "update RDB$FUNCTIONS set rdb$description = ? "
        "where RDB$FUNCTION_NAME = ?");
}

void SaveDescriptionVisitor::visitUDF(UDF& function)
{
	if (function.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&function, "comment on function %s is '%s'");
		return;
	}
    saveDescription(&function,
        "update RDB$FUNCTIONS set rdb$description = ? "
        "where RDB$FUNCTION_NAME = ?");
}

void SaveDescriptionVisitor::visitGenerator(Generator& generator)
{
	if (generator.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&generator, "comment on generator %s is '%s'");
		return;
	}
    saveDescription(&generator,
        "update RDB$GENERATORS set RDB$DESCRIPTION = ? "
        "where RDB$GENERATOR_NAME = ?");
}

void SaveDescriptionVisitor::visitIndex(Index& index)
{
	if (index.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&index, "comment on index %s is '%s'");
		return;
	}
    saveDescription(&index,
        "update RDB$INDICES set RDB$DESCRIPTION = ? "
        "where RDB$INDEX_NAME = ?");
}

void SaveDescriptionVisitor::visitParameter(Parameter& parameter)
{
	if (parameter.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&parameter, parameter.getParent(), "comment on parameter %s.%s is '%s'");
		return;
	}
    // TODO: use Parameter::getProcedure() instead
    saveDescription(&parameter, parameter.getParent(),
        "update RDB$PROCEDURE_PARAMETERS set RDB$DESCRIPTION = ? "
        "where RDB$PARAMETER_NAME = ? and RDB$PROCEDURE_NAME = ?");
}

void SaveDescriptionVisitor::visitProcedure(Procedure& procedure)
{
	if (procedure.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&procedure, "comment on procedure %s is '%s'");
		return;
	}
    saveDescription(&procedure,
        "update RDB$PROCEDURES set RDB$DESCRIPTION = ? "
        "where RDB$PROCEDURE_NAME = ?");
}

void SaveDescriptionVisitor::visitRelation(Relation& relation)
{
	if (relation.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		if (relation.getRelationType() == 1)
			saveDescription(&relation, "comment on view %s is '%s'");
		else
			saveDescription(&relation, "comment on table %s is '%s'");
		return;
	}
    saveDescription(&relation,
        "update RDB$RELATIONS set RDB$DESCRIPTION = ? "
        "where RDB$RELATION_NAME = ?");
}

void SaveDescriptionVisitor::visitRole(Role& role)
{
	if (role.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&role, "comment on role %s is '%s'");
		return;
	}
    saveDescription(&role,
        "update RDB$ROLES set RDB$DESCRIPTION = ? "
        "where RDB$ROLE_NAME = ?");
}

void SaveDescriptionVisitor::visitTrigger(Trigger& trigger)
{
	if (trigger.getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(11, 1)) { //Its available since FB 2.0, ODS 11.0 but I like to use "new" resources for safety
		saveDescription(&trigger, "comment on trigger %s is '%s'");
		return;
	}
    saveDescription(&trigger,
        "update RDB$TRIGGERS set RDB$DESCRIPTION = ? "
        "where RDB$TRIGGER_NAME = ?");
}

