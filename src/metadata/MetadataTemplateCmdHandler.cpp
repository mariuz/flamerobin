/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <sstream>
#include <iomanip>

#include "core/StringUtils.h"
#include "core/TemplateProcessor.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/privilege.h"
#include "metadata/procedure.h"
#include "metadata/relation.h"
#include "metadata/role.h"
#include "metadata/server.h"
#include "metadata/User.h"

//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
class MetadataTemplateCmdHandler: public TemplateCmdHandler
{
private:
    static const MetadataTemplateCmdHandler handlerInstance; // singleton; registers itself on creation.
public:
    virtual void handleTemplateCmd(TemplateProcessor *tp, wxString cmdName,
        TemplateCmdParams cmdParams, MetadataItem* object, wxString& processedText);
};
//-----------------------------------------------------------------------------
const MetadataTemplateCmdHandler MetadataTemplateCmdHandler::handlerInstance;
//-----------------------------------------------------------------------------
void MetadataTemplateCmdHandler::handleTemplateCmd(TemplateProcessor *tp,
    wxString cmdName, TemplateCmdParams cmdParams, MetadataItem* object,
    wxString& processedText)
{
    // {%object_name%}
    // Expands to the current object's (non quoted) name.
    if (cmdName == wxT("object_name"))
        processedText += object->getName_();

    // {%object_quoted_name%}
    // Expands to the current object's quoted name.
    else if (cmdName == wxT("object_quoted_name"))
        processedText += object->getQuotedName();

    // {%object_path%}
    // Expands to the current object's full path in the DBH.
    else if (cmdName == wxT("object_path"))
        processedText += object->getItemPath();

    // {%object_type%}
    // Expands to the current object's type name.
    else if (cmdName == wxT("object_type"))
        processedText += object->getTypeName();

    // {%is_system%}
    // Expands to "true" if the current object is a system object,
    // and to "false" otherwise.
    else if (cmdName == wxT("is_system"))
        processedText += getBooleanAsString(object->isSystem());

    // {%owner_name%}
    // If the current object is a procedure, relation or role
    // expands to the owner's name.
    else if (cmdName == wxT("owner_name"))
    {
        wxString name;
        if (Relation* r = dynamic_cast<Relation*>(object))
            name = r->getOwner();
        else if (Procedure* p = dynamic_cast<Procedure*>(object))
            name = p->getOwner();
        else if (Role* r = dynamic_cast<Role*>(object))
            name = r->getOwner();
        if (!name.IsEmpty())
            processedText += tp->escapeChars(name);
    }

    // {%object_description%}
    // Expands to the current object's description, if available.
    // Otherwise expands to an empty string.
    else if (cmdName == wxT("object_description"))
    {
        wxString desc;
        if (object->getDescription(desc))
        {
            if (desc.IsEmpty())
                desc = _("No description");
            processedText += tp->escapeChars(desc);
        }
    }

    // {%dependencyinfo:property%}
    // If the current object is a dependency, expands to the requested
    // property of the dependency object.
    else if ((cmdName == wxT("dependencyinfo")) && (cmdParams.Count() >= 1)
        && (cmdParams[0] == wxT("fields")))
    {
        Dependency* d = dynamic_cast<Dependency*>(object);
        if (!d)
            return;
        processedText += d->getFields();
    }

    // {%primary_key:<text>%}
    // If the current object is a table, processes <text> after switching
    // the current object to the table's primary key.
    else if (cmdName == wxT("primary_key"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        PrimaryKeyConstraint* pk = t->getPrimaryKey();
        if (!pk)
            return;
        tp->internalProcessTemplateText(processedText, cmdParams.all(), pk);
    }

    // {%checkconstraintinfo:<property>%}
    // If the current object is a check constraint, expands to the constraint's
    // requested property.
    else if ((cmdName == wxT("checkconstraintinfo")) && (cmdParams.Count() >= 1))
    {
        CheckConstraint* c = dynamic_cast<CheckConstraint*>(object);
        if (!c)
            return;

        if (cmdParams[0] == wxT("source"))
            processedText += tp->escapeChars(c->getSource(), false);
    }

    // {%constraintinfo:columns%} (uses default separator and suffix)
    // {%constraintinfo:columns:separator%} (uses default suffix)
    // {%constraintinfo:columns:separator:suffix%}
    // If the current object is a column constraint (primary key, unique
    // constraint, foreign key, check constraint), expands to
    // a delimited list of constraint columns.
    else if ((cmdName == wxT("constraintinfo")) && (cmdParams.Count() >= 1)
        && (cmdParams[0] == wxT("columns")))
    {
        ColumnConstraint* c = dynamic_cast<ColumnConstraint*>(object);
        if (!c)
            return;

        if (cmdParams.Count() == 1)
            processedText += c->getColumnList();
        else if (cmdParams.Count() == 2)
            processedText += c->getColumnList(cmdParams[1]);
        else if (cmdParams.Count() >= 3)
            processedText += c->getColumnList(cmdParams[1], cmdParams.all(2));
    }

    // {%fkinfo:<property>%}
    // If the current object is a foreign key, expands to the foreign key's
    // requested property.
    else if ((cmdName == wxT("fkinfo")) && (cmdParams.Count() >= 1))
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;

        if (cmdParams[0] == wxT("ref_columns"))
            processedText += tp->escapeChars(fk->getReferencedColumnList());
        else if (cmdParams[0] == wxT("ref_table"))
            processedText += tp->escapeChars(fk->getReferencedTable());
        else if (cmdParams[0] == wxT("update_action"))
            processedText += tp->escapeChars(fk->getUpdateAction());
        else if (cmdParams[0] == wxT("delete_action"))
            processedText += tp->escapeChars(fk->getDeleteAction());
    }

    // {%columninfo:<property>%}
    // If the current object is a column, expands to the column's
    // requested property.
    else if ((cmdName == wxT("columninfo")) && (cmdParams.Count() >= 1))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (!c)
            return;

        if (cmdParams[0] == wxT("datatype"))
            processedText += tp->escapeChars(c->getDatatype());
        else if (cmdParams[0] == wxT("is_nullable"))
            processedText += tp->escapeChars(getBooleanAsString(c->isNullable()));
        else if (cmdParams[0] == wxT("null_option"))
            processedText += tp->escapeChars(c->isNullable() ? wxT("") : wxT("not null"));
        else if (cmdParams[0] == wxT("default_expression"))
        {
            wxString def(c->getDefault());
            def.Trim(false);
            if (def.Upper().StartsWith(wxT("DEFAULT")))
            {
                def.Remove(0, 7);
                def.Trim(false);
            }
            processedText += tp->escapeChars(def, false);
        }
    }

    // {%viewinfo:<property>%}
    // If the current object is a view, expands to the view's
    // requested property.
    else if ((cmdName == wxT("viewinfo")) && (cmdParams.Count() >= 1))
    {
        View* v = dynamic_cast<View*>(object);
        if (!v)
            return;
        if (cmdParams[0] == wxT("source"))
            processedText += tp->escapeChars(v->getSource(), false);
    }

    // {%procedureinfo:<property>%}
    // If the current object is a procedure, expands to the procedure's
    // requested property.
    else if ((cmdName == wxT("procedureinfo")) && (cmdParams.Count() >= 1))
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;
        if (cmdParams[0] == wxT("source"))
            processedText += tp->escapeChars(p->getSource(), false);
    }

    // {%triggerinfo:<property>%}
    // If the current object is a trigger, expands to the trigger's
    // requested property.
    else if ((cmdName == wxT("triggerinfo")) && (cmdParams.Count() >= 1))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        if (!t)
            return;
        if (cmdParams[0] == wxT("source"))
            processedText += tp->escapeChars(t->getSource(), false);
        else
        {
            wxString object, type;
            bool isActive, isDBTrigger;
            int position;
            t->getTriggerInfo(object, isActive, position, type, isDBTrigger);
            if (cmdParams[0] == wxT("object_name"))
                processedText += tp->escapeChars(object);
            else if (cmdParams[0] == wxT("is_active"))
                processedText += tp->escapeChars(getBooleanAsString(isActive));
            else if (cmdParams[0] == wxT("position"))
                processedText << position;
            else if (cmdParams[0] == wxT("type"))
                processedText += tp->escapeChars(type);
            else if (cmdParams[0] == wxT("is_db_trigger"))
                processedText += tp->escapeChars(getBooleanAsString(isDBTrigger));
        }
    }

    else if (cmdName == wxT("generator_value"))
    {
        Generator* g = dynamic_cast<Generator*>(object);
        if (!g)
            return;
        std::ostringstream ss;
        ss << g->getValue();
        processedText += tp->escapeChars(std2wx(ss.str()));
    }

    // TODO: switch to a single exceptioninfo command.
    else if (cmdName == wxT("exception_number"))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;
        wxString s;
        s << e->getNumber();
        processedText += tp->escapeChars(s);
    }

    else if (cmdName == wxT("exception_message"))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;
        processedText += tp->escapeChars(e->getMessage());
    }

    else if (cmdName == wxT("udfinfo") && !cmdParams.IsEmpty())
    {
        Function* f = dynamic_cast<Function*>(object);
        if (!f)
            return;

        if (cmdParams[0] == wxT("library"))
            processedText += tp->escapeChars(f->getLibraryName());
        else if (cmdParams[0] == wxT("entry_point"))
            processedText += tp->escapeChars(f->getEntryPoint());
        else if (cmdParams[0] == wxT("definition"))
            processedText += tp->escapeChars(f->getDefinition(), false);
    }

    else if (cmdName == wxT("indices"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<Index>* ix = t->getIndices();
        if (!ix)
            return;
        for (std::vector<Index>::iterator it = ix->begin(); it != ix->end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), &(*it));
    }

    else if (cmdName == wxT("object_ddl"))
    {
        ProgressIndicator* pi = tp->getProgressIndicator();
        if (pi)
        {
            pi->setProgressLevelCount(2);
            pi->initProgress(_("Extracting DDL Definitions"));
            pi->doShow();
        }
        CreateDDLVisitor cdv(pi);
        object->acceptVisitor(&cdv);
        processedText += tp->escapeChars(cdv.getSql(), false);
    }

    else if (cmdName == wxT("indexinfo") && !cmdParams.IsEmpty())
    {
        Index* i = dynamic_cast<Index*>(object);
        if (!i)
            return;
        if (cmdParams[0] == wxT("type"))
            processedText += (i->getIndexType() == Index::itAscending ? wxT("ASC") : wxT("DESC"));
        else if (cmdParams[0] == wxT("stats"))
        {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << i->getStatistics();
            processedText += std2wx(ss.str());
        }
        else if (cmdParams[0] == wxT("fields"))
            processedText += i->getFieldsAsString();
        else if (cmdParams[0] == wxT("is_active"))
            processedText += getBooleanAsString(i->isActive());
        else if (cmdParams[0] == wxT("is_unique"))
            processedText += getBooleanAsString(i->isUnique());
    }

    // TODO: group all database-related info inside a {%dbinfo:xxx%}?
    else if (cmdName == wxT("creation_date"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += d->getInfo().getCreated();
    }
    
    else if (cmdName == wxT("default_charset"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += d->getDatabaseCharset();
    }
    
    else if (cmdName == wxT("dialect"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += wxString() << d->getInfo().getDialect();
    }
    
    else if (cmdName == wxT("filesize"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        int64_t size = d->getInfo().getSizeInBytes();
        const double kilo = 1024;
        const double mega = kilo * kilo;
        const double giga = kilo * mega;
        if (size >= giga)
            processedText += wxString::Format(wxT("%0.2fGB"), size / giga);
        else if (size >= mega)
            processedText += wxString::Format(wxT("%0.2fMB"), size / mega);
        else
            processedText += wxString::Format(wxT("%0.2fkB"), size / kilo);
    }
    
    else if (cmdName == wxT("forced_writes"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += getBooleanAsString(d->getInfo().getForcedWrites());
    }
    
    else if (cmdName == wxT("fullpath"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += d->getConnectionString();
    }
    
    else if (cmdName == wxT("next_transaction"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += wxString() << d->getInfo().getNextTransaction();
    }
    
    else if (cmdName == wxT("ods_version"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += wxString() << d->getInfo().getODS();
        if (d->getInfo().getODSMinor())
        {
            processedText += wxT(".");
            processedText += wxString() << d->getInfo().getODSMinor();
        }
    }
    
    else if (cmdName == wxT("oldest_transaction"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += wxString() << d->getInfo().getOldestTransaction();
    }
    
    else if (cmdName == wxT("page_buffers"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += wxString() << d->getInfo().getBuffers();
    }
    
    else if (cmdName == wxT("page_size"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += wxString() << d->getInfo().getPageSize();
    }
    
    else if (cmdName == wxT("pages"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += wxString() << d->getInfo().getPages();
    }
    
    else if (cmdName == wxT("read_only"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += getBooleanAsString(d->getInfo().getReadOnly());
    }
    
    else if (cmdName == wxT("sweep_interval"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        processedText += wxString() << d->getInfo().getSweep();
    }

    // {%foreach:<collection>:<separator>:<text>%}
    // repeats <text> once for each item in <collection>, pasting a <separator>
    // before each item except the first.
    if ((cmdName == wxT("foreach")) && (cmdParams.Count() >= 3))
    {
        wxString sep;
        tp->internalProcessTemplateText(sep, cmdParams[1], object);
        
        // {%foreach:column:<separator>:<text>%}
        // If the current object is a relation, processes <text> for each column.
        if (cmdParams[0] == wxT("column"))
        {
            Relation* r = dynamic_cast<Relation*>(object);
            if (!r)
                return;
            r->ensureChildrenLoaded();
            bool firstItem = true;
            for (RelationColumns::iterator it = r->begin(); it != r->end(); ++it)
            {
                wxString newText;
                tp->internalProcessTemplateText(newText, cmdParams.all(2), (*it).get());
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }

        // {%foreach:foreign_key:<separator>:<text>%}
        // If the current object is a table, processes <text> for each foreign key.
        else if (cmdParams[0] == wxT("foreign_key"))
        {
            Table* t = dynamic_cast<Table*>(object);
            if (!t)
                return;
            std::vector<ForeignKey>* fks = t->getForeignKeys();
            if (!fks)
                return;
            bool firstItem = true;
            for (std::vector<ForeignKey>::iterator it = fks->begin(); it != fks->end(); ++it)
            {
                wxString newText;
                tp->internalProcessTemplateText(newText, cmdParams.all(2), &(*it));
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }

        // {%foreach:check_constraint:<separator>:<text>%}
        // If the current object is a table, processes <text> for each check constraint.
        else if (cmdParams[0] == wxT("check_constraint"))
        {
            Table* t = dynamic_cast<Table*>(object);
            if (!t)
                return;
            std::vector<CheckConstraint>* c = t->getCheckConstraints();
            if (!c)
                return;
            bool firstItem = true;
            for (std::vector<CheckConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            {
                wxString newText;
                tp->internalProcessTemplateText(newText, cmdParams.all(2), &(*it));
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }

        // {%foreach:unique_constraint:<separator>:<text>%}
        // If the current object is a table, processes <text> for each unique constraint.
        else if (cmdParams[0] == wxT("unique_constraint"))
        {
            Table* t = dynamic_cast<Table*>(object);
            if (!t)
                return;
            std::vector<UniqueConstraint>* c = t->getUniqueConstraints();
            if (!c)
                return;
            bool firstItem = true;
            for (std::vector<UniqueConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            {
                wxString newText;
                tp->internalProcessTemplateText(newText, cmdParams.all(2), &(*it));
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }

        // {%foreach:trigger:<separator>:<before|after>:<text>%}
        // If the current object is a relation, processes <text> for
        // each "before" or "after" trigger. If the current object is
        // a database, processes <text> for all database triggers and the
        // third param is ignored.
        else if ((cmdParams[0] == wxT("trigger")) && (cmdParams.Count() >= 4))
        {
            std::vector<Trigger*> triggers;
            
            Relation* r = dynamic_cast<Relation*>(object);
            if (r)
            {
                if (cmdParams[2] == wxT("after"))
                    r->getTriggers(triggers, Trigger::afterTrigger);
                else if (cmdParams[2] == wxT("before"))
                    r->getTriggers(triggers, Trigger::beforeTrigger);
            }
            else
            {
                Database* d = dynamic_cast<Database*>(object);
                if (d)
                    d->getDatabaseTriggers(triggers);
            }

            for (std::vector<Trigger*>::iterator it = triggers.begin();
                it != triggers.end(); ++it)
            {
                tp->internalProcessTemplateText(processedText, cmdParams.all(3), *it);
            }
        }

        // {%foreach:privilegeitem:<separator>:<type>:<text>%}
        // If the current object is a privilege, processes <text> for each privilege item
        // of the specified <type> (SELECT, INSERT, UPDATE, DELETE, EXECUTE, REFERENCES,
        // MEMBER OF).
        else if ((cmdParams[0] == wxT("privilegeitem")) && (cmdParams.Count() >= 4))
        {
            Privilege* p = dynamic_cast<Privilege*>(object);
            if (!p)
                return;
            PrivilegeItems list;
            p->getPrivilegeItems(cmdParams[2], list);
            bool firstItem = true;
            for (PrivilegeItems::iterator it = list.begin(); it != list.end(); ++it)
            {
                wxString newText;
                tp->internalProcessTemplateText(newText, cmdParams.all(3), &(*it));
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }

        // {%foreach:privilege:<separator>:<text>%}
        // If the current object is a relation, procedure or role,
        // processes <text> for each privilege.
        else if (cmdParams[0] == wxT("privilege"))
        {
            Relation* rel = dynamic_cast<Relation*>(object);
            Procedure* proc = dynamic_cast<Procedure*>(object);
            Role* role = dynamic_cast<Role*>(object);
            std::vector<Privilege>* p = 0;
            if (rel)
                p = rel->getPrivileges();
            if (proc)
                p = proc->getPrivileges();
            if (role)
                p = role->getPrivileges();
            if (!p)
                return;
            for (std::vector<Privilege>::iterator it = p->begin(); it != p->end(); ++it)
                tp->internalProcessTemplateText(processedText, cmdParams.all(2), &(*it));
        }

        // {%foreach:depends_on:<separator>:<text>%}
        // Lists all objects on which the current object depends.
        // {%foreach:dependent:<separator>:<text>%}
        // Lists all objects that depend on the current object.
        else if (cmdParams[0] == wxT("depends_on") || cmdParams[0] == wxT("dependent"))
        {
            MetadataItem* m = dynamic_cast<MetadataItem*>(object);
            if (!m)
                return;
            std::vector<Dependency> deps;
            m->getDependencies(deps, cmdParams[0] == wxT("depends_on"));
            for (std::vector<Dependency>::iterator it = deps.begin(); it != deps.end(); ++it)
                tp->internalProcessTemplateText(processedText, cmdParams.all(2), &(*it));
        }

        // {%foreach:parameter:<separator>:<input|output>:<text>%}
        // If the current object is a procedure, processes <text> for
        // each "input" or "output" parameter.
        else if ((cmdParams[0] == wxT("parameter")) && (cmdParams.Count() >= 4))
        {
            Procedure* p = dynamic_cast<Procedure*>(object);
            if (!p)
                return;

            SubjectLocker locker(p);
            p->ensureChildrenLoaded();
            bool isOut = (cmdParams[0] == wxT("output"));
            for (ProcedureParameters::iterator it = p->begin();
                it != p->end(); ++it)
            {
                if ((*it)->isOutputParameter() == isOut)
                    tp->internalProcessTemplateText(processedText, cmdParams.all(3), (*it).get());
            }
        }

        // add more collections here.
        else
            return;
    }

    else if (cmdName == wxT("privilegeinfo") && (cmdParams.Count() > 0)) 
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
            
        if (cmdParams[0] == wxT("grantee_name"))
            processedText += tp->escapeChars(p->getGrantee());
    }

    // {%privilegeitemcount:type%}
    else if ((cmdName == wxT("privilegeitemcount")) && (cmdParams.Count() >= 1))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
        PrivilegeItems list;
        p->getPrivilegeItems(cmdParams[0], list);
        processedText << list.size();
    }
    
    // {%privilegeiteminfo:property%}
    else if ((cmdName == wxT("privilegeiteminfo")) && (cmdParams.Count() >= 1))
    {
        PrivilegeItem* pi = dynamic_cast<PrivilegeItem*>(object);
        if (!pi)
            return;
        
        if (cmdParams[0] == wxT("grantor"))
            processedText += pi->grantor;
        else if (cmdParams[0] == wxT("grant_option"))
            processedText += getBooleanAsString(pi->grantOption);
        else if (cmdParams[0] == wxT("columns"))
        {
            for (vector<wxString>::iterator it = pi->columns.begin();
                it != pi->columns.end(); ++it)
            {
                if (it != pi->columns.begin())
                    processedText += wxT(",");
                processedText += (*it);
            }
        }
    }

    // {%users:text%}
    // Processes the specified text once for each defined user,
    // switching each time the current object to the nth user.
    else if (cmdName == wxT("users") && !cmdParams.IsEmpty())
    {
        Server* s = dynamic_cast<Server*>(object);
        if (!s)
            return;

        ProgressIndicator* pi = tp->getProgressIndicator();
        if (pi)
        {
            pi->initProgress(_("Connecting to Server..."));
            pi->doShow();
        }

        UserList* usr = s->getUsers(pi);
        if (!usr || !usr->size())
            return;

        for (UserList::iterator it = usr->begin(); it != usr->end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), &(*it));
    }

    // {%userinfo:property%}
    // If the current object  is a user, expands to various user properties.
    else if (cmdName == wxT("userinfo") && !cmdParams.IsEmpty())
    {
        User* u = dynamic_cast<User*>(object);
        if (!u)
            return;

        if (cmdParams[0] == wxT("username"))
            processedText += tp->escapeChars(u->usernameM);
        else if (cmdParams[0] == wxT("first_name"))
            processedText += tp->escapeChars(u->firstnameM);
        else if (cmdParams[0] == wxT("middle_name"))
            processedText += tp->escapeChars(u->middlenameM);
        else if (cmdParams[0] == wxT("last_name"))
            processedText += tp->escapeChars(u->lastnameM);
        else if (cmdParams[0] == wxT("unix_user"))
            processedText << u->useridM;
        else if (cmdParams[0] == wxT("unix_group"))
            processedText << u->groupidM;
    }
}
//-----------------------------------------------------------------------------
