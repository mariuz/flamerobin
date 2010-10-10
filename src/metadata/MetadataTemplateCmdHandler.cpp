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

#include "core/ProcessableObject.h"
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
        TemplateCmdParams cmdParams, ProcessableObject* object, wxString& processedText);
};
//-----------------------------------------------------------------------------
const MetadataTemplateCmdHandler MetadataTemplateCmdHandler::handlerInstance;
//-----------------------------------------------------------------------------
void MetadataTemplateCmdHandler::handleTemplateCmd(TemplateProcessor* tp,
    wxString cmdName, TemplateCmdParams cmdParams, ProcessableObject* object,
    wxString& processedText)
{
    struct Local
    {
        // Implements the body of all {%foreach%} loops.
        static void foreachIteration(bool& firstItem,
            TemplateProcessor* tp, wxString& processedText,
            wxString separator, wxString text, ProcessableObject* object)
        {
            wxString newText;
            tp->internalProcessTemplateText(newText, text, object);
            if ((!firstItem) && (!newText.IsEmpty()))
                processedText += tp->escapeChars(separator);
            if (!newText.IsEmpty())
                firstItem = false;
            processedText += newText;
        }
    };

    MetadataItem* metadataItem = dynamic_cast<MetadataItem*>(object);

    // {%parent:text%}
    // Switches the current object to the object's parent and
    // processes the specified text. If the object has no parent,
    // expands to a blank string.
    if ((cmdName == wxT("parent")) && metadataItem && metadataItem->getParent())
            tp->internalProcessTemplateText(processedText, cmdParams.all(),
                metadataItem->getParent());

    // {%object_handle%}
    // Expands to the current object's unique numeric handle.
    // Used to call FR's commands through URIs.
    else if ((cmdName == wxT("object_handle")) && metadataItem)
        processedText += wxString::Format(wxT("%d"), metadataItem->getHandle());

    // {%object_name%}
    // Expands to the current object's (non quoted) name.
    else if ((cmdName == wxT("object_name")) && metadataItem)
        processedText += metadataItem->getName_();

    // {%object_quoted_name%}
    // Expands to the current object's quoted name.
    else if ((cmdName == wxT("object_quoted_name")) && metadataItem)
        processedText += metadataItem->getQuotedName();

    // {%object_path%}
    // Expands to the current object's full path in the DBH.
    else if ((cmdName == wxT("object_path")) && metadataItem)
        processedText += metadataItem->getItemPath();

    // {%object_type%}
    // Expands to the current object's type name.
    else if ((cmdName == wxT("object_type")) && metadataItem)
        processedText += metadataItem->getTypeName();

    // {%is_system%}
    // Expands to "true" if the current object is a system object,
    // and to "false" otherwise.
    else if ((cmdName == wxT("is_system")) && metadataItem)
        processedText += getBooleanAsString(metadataItem->isSystem());

    // {%foreach:<collection>:<separator>:<text>%}
    // Repeats <text> once for each item in <collection>, pasting a <separator>
    // before each item except the first.
    else if ((cmdName == wxT("foreach")) && (cmdParams.Count() >= 3))
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
            for (ColumnPtrs::iterator it = r->begin(); it != r->end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(2), (*it).get());
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
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(2), &(*it));
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
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(2), &(*it));
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
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(2), &(*it));
            }
        }

        // {%foreach:index:<separator>:<text>%}
        // If the current object is a table, processes <text> for each index.
        else if (cmdParams[0] == wxT("index"))
        {
            Table* t = dynamic_cast<Table*>(object);
            if (!t)
                return;
            std::vector<Index>* ix = t->getIndices();
            if (!ix)
                return;
            bool firstItem = true;
            for (std::vector<Index>::iterator it = ix->begin(); it != ix->end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(2), &(*it));
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

            bool firstItem = true;
            for (std::vector<Trigger*>::iterator it = triggers.begin();
                it != triggers.end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(3), *it);
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
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(3), &(*it));
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
            bool firstItem = true;
            for (std::vector<Privilege>::iterator it = p->begin(); it != p->end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(2), &(*it));
            }
        }

        // {%foreach:depends_on:<separator>:<text>%}
        // Lists all objects on which the current object depends.
        // {%foreach:dependent:<separator>:<text>%}
        // Lists all objects that depend on the current object.
        else if (cmdParams[0] == wxT("depends_on") || cmdParams[0] == wxT("dependent"))
        {
            if (!metadataItem)
                return;

            std::vector<Dependency> deps;
            metadataItem->getDependencies(deps, cmdParams[0] == wxT("depends_on"));
            bool firstItem = true;
            for (std::vector<Dependency>::iterator it = deps.begin(); it != deps.end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(2), &(*it));
            }
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
            bool firstItem = true;
            for (ParameterPtrs::iterator it = p->begin(); it != p->end(); ++it)
            {
                if ((*it)->isOutputParameter() == isOut)
                {
                    Local::foreachIteration(firstItem, tp, processedText, sep,
                        cmdParams.all(3), (*it).get());
                }
            }
        }

        // {%foreach:user:<separator>:<text>%}
        // If the current object is a server, processes
        // the specified text once for each defined user,
        // switching each time the current object to the nth user.
        else if (cmdParams[0] == wxT("user"))
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

            bool firstItem = true;
            for (UserList::iterator it = usr->begin(); it != usr->end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.all(2), &(*it));
            }
        }
        // add more collections here.
        else
            return;
    }

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
    else if ((cmdName == wxT("object_description")) && metadataItem)
    {
        wxString desc;
        if (metadataItem->getDescription(desc))
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
            if (cmdParams[0] == wxT("name"))
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

    // {%generatorinfo:<property>%}
    // If the current object is a generator, expands to the generator's
    // requested property.
    else if ((cmdName == wxT("generatorinfo")) && (cmdParams.Count() >= 1))
    {
        Generator* g = dynamic_cast<Generator*>(object);
        if (!g)
            return;

        if (cmdParams[0] == wxT("value"))
            processedText << g->getValue();
    }

    // {%exceptioninfo:<property>%}
    // If the current object is an exception, expands to the exception's
    // requested property.
    else if ((cmdName == wxT("exceptioninfo")) && (cmdParams.Count() >= 1))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;

        if (cmdParams[0] == wxT("number"))
            processedText << e->getNumber();
        else if (cmdParams[0] == wxT("message"))
            processedText << e->getMessage();
    }

    // {%functioninfo:<property>%}
    // If the current object is a function, expands to the function's
    // requested property.
    else if (cmdName == wxT("functioninfo") && !cmdParams.IsEmpty())
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

    // {%indexinfo:<property>%}
    // If the current object is an index, expands to the index's
    // requested property.
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
        else if (cmdParams[0] == wxT("is_ascending"))
            processedText += getBooleanAsString(i->getIndexType() == Index::itAscending);
    }

    // {%object_ddl%}
    // Expands to the current object's DDL definition.
    else if ((cmdName == wxT("object_ddl")) && metadataItem)
    {
        ProgressIndicator* pi = tp->getProgressIndicator();
        if (pi)
        {
            pi->setProgressLevelCount(2);
            pi->initProgress(_("Extracting DDL Definitions"));
            pi->doShow();
        }
        CreateDDLVisitor cdv(pi);
        metadataItem->acceptVisitor(&cdv);
        processedText += tp->escapeChars(cdv.getSql(), false);
    }

    // {%dbinfo:<property>%}
    // If the current object is a database, expands to the database's
    // requested property.
    else if (cmdName == wxT("dbinfo") && !cmdParams.IsEmpty())
    {
        Database* db = dynamic_cast<Database*>(object);
        if (!db)
            return;

        if (cmdParams[0] == wxT("connection_string"))
            processedText += db->getConnectionString();
        else if (cmdParams[0] == wxT("ods_version"))
        {
            processedText += wxString() << db->getInfo().getODS();
            if (db->getInfo().getODSMinor())
            {
                processedText += wxT(".");
                processedText += wxString() << db->getInfo().getODSMinor();
            }
        }
        else if (cmdParams[0] == wxT("page_size"))
            processedText += wxString() << db->getInfo().getPageSize();
        else if (cmdParams[0] == wxT("pages"))
            processedText += wxString() << db->getInfo().getPages();
        else if (cmdParams[0] == wxT("size"))
        {
            int64_t size = db->getInfo().getSizeInBytes();
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
        else if (cmdParams[0] == wxT("page_buffers"))
            processedText += wxString() << db->getInfo().getBuffers();
        else if (cmdParams[0] == wxT("reserve_space"))
            processedText += getBooleanAsString(db->getInfo().getReserve());
        else if (cmdParams[0] == wxT("is_read_only"))
            processedText += getBooleanAsString(db->getInfo().getReadOnly());
        else if (cmdParams[0] == wxT("sql_dialect"))
            processedText += wxString() << db->getInfo().getDialect();
        else if (cmdParams[0] == wxT("default_charset"))
            processedText += db->getDatabaseCharset();
        else if (cmdParams[0] == wxT("sweep_interval"))
            processedText += wxString() << db->getInfo().getSweep();
        else if (cmdParams[0] == wxT("forced_writes"))
            processedText += getBooleanAsString(db->getInfo().getForcedWrites());
        else if (cmdParams[0] == wxT("oldest_transaction"))
            processedText += wxString() << db->getInfo().getOldestTransaction();
        else if (cmdParams[0] == wxT("next_transaction"))
            processedText += wxString() << db->getInfo().getNextTransaction();
        else if (cmdParams[0] == wxT("connected_users"))
        {
            wxArrayString users;
            db->getConnectedUsers(users);
            processedText += wxArrayToString(users, wxT(","));
        }
    }

    // {%privilegeinfo:<property>%}
    // If the current object is a privilege, expands to the privilege's
    // requested property.
    else if (cmdName == wxT("privilegeinfo") && (cmdParams.Count() > 0)) 
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
            
        if (cmdParams[0] == wxT("grantee_name"))
            processedText += tp->escapeChars(p->getGrantee());
    }

    // {%privilegeitemcount:<type>%}
    // If the current object is a privilege, expands to the count of
    // privilege items of the requested <type>.
    else if ((cmdName == wxT("privilegeitemcount")) && (cmdParams.Count() >= 1))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;

        PrivilegeItems list;
        p->getPrivilegeItems(cmdParams[0], list);
        processedText << list.size();
    }
    
    // {%privilegeiteminfo:<property>%}
    // If the current object is a privilege item, expands to the privilege
    // item's requested property.
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
