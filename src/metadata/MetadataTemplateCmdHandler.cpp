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

#include "core/ProcessableObject.h"
#include "core/StringUtils.h"
#include "core/TemplateProcessor.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/parameter.h"
#include "metadata/privilege.h"
#include "metadata/procedure.h"
#include "metadata/relation.h"
#include "metadata/role.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/User.h"
#include "metadata/view.h"
#include "metadata/package.h"


class MetadataTemplateCmdHandler: public TemplateCmdHandler
{
private:
    static const MetadataTemplateCmdHandler handlerInstance; // singleton; registers itself on creation.
public:
    MetadataTemplateCmdHandler() {};
    virtual void handleTemplateCmd(TemplateProcessor *tp,
        const wxString& cmdName, const TemplateCmdParams& cmdParams,
        ProcessableObject* object, wxString& processedText);
};

const MetadataTemplateCmdHandler MetadataTemplateCmdHandler::handlerInstance;

void MetadataTemplateCmdHandler::handleTemplateCmd(TemplateProcessor *tp,
    const wxString& cmdName, const TemplateCmdParams& cmdParams,
    ProcessableObject* object, wxString& processedText)
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
                processedText += separator;
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
    if ((cmdName == "parent") && metadataItem && metadataItem->getParent())
        tp->internalProcessTemplateText(processedText, cmdParams.all(),
            metadataItem->getParent());

    // {%database:text%}
    // Switches the current object to the object's database, if any, and
    // processes the specified text. If the object has no database,
    // expands to a blank string.
    if ((cmdName == "database") && metadataItem && metadataItem->getDatabase())
        tp->internalProcessTemplateText(processedText, cmdParams.all(),
            metadataItem->getDatabase().get());

    // {%object_handle%}
    // Expands to the current object's unique numeric handle.
    // Used to call FR's commands through URIs.
    else if ((cmdName == "object_handle") && metadataItem)
        processedText += wxString::Format("%lu", metadataItem->getHandle());

    // {%object_name%}
    // Expands to the current object's (non quoted) name.
    else if ((cmdName == "object_name") && metadataItem)
        processedText += metadataItem->getName_();

    // {%object_quoted_name%}
    // Expands to the current object's quoted name.
    else if ((cmdName == "object_quoted_name") && metadataItem)
        processedText += metadataItem->getQuotedName();

    // {%object_path%}
    // Expands to the current object's full path in the DBH.
    else if ((cmdName == "object_path") && metadataItem)
        processedText += metadataItem->getItemPath();

    // {%object_type%}
    // Expands to the current object's type name.
    else if ((cmdName == "object_type") && metadataItem)
        processedText += metadataItem->getTypeName();

    // {%is_system%}
    // Expands to "true" if the current object is a system object,
    // and to "false" otherwise.
    else if ((cmdName == "is_system") && metadataItem)
        processedText += getBooleanAsString(metadataItem->isSystem());

    // {%foreach:<collection>:<separator>:<text>%}
    // Repeats <text> once for each item in <collection>, pasting a <separator>
    // before each item except the first.
    else if ((cmdName == "foreach") && (cmdParams.Count() >= 3))
    {
        wxString sep;
        tp->internalProcessTemplateText(sep, cmdParams[1], object);
        
        // {%foreach:column:<separator>:<text>%}
        // If the current object is a relation, processes <text> for each column.
        if (cmdParams[0] == "column")
        {
            Relation* r = dynamic_cast<Relation*>(object);
            if (!r)
                return;
            r->ensureChildrenLoaded();
            bool firstItem = true;
            for (ColumnPtrs::iterator it = r->begin(); it != r->end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.from(2), (*it).get());
            }
        }

        // {%foreach:foreign_key:<separator>:<text>%}
        // If the current object is a table, processes <text> for each foreign key.
        else if (cmdParams[0] == "foreign_key")
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
                    cmdParams.from(2), &(*it));
            }
        }

        // {%foreach:check_constraint:<separator>:<text>%}
        // If the current object is a table, processes <text> for each check constraint.
        else if (cmdParams[0] == "check_constraint")
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
                    cmdParams.from(2), &(*it));
            }
        }

        // {%foreach:unique_constraint:<separator>:<text>%}
        // If the current object is a table, processes <text> for each unique constraint.
        else if (cmdParams[0] == "unique_constraint")
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
                    cmdParams.from(2), &(*it));
            }
        }

        // {%foreach:index:<separator>:<text>%}
        // If the current object is a table, processes <text> for each index.
        else if (cmdParams[0] == "index")
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
                    cmdParams.from(2), &(*it));
            }
        }

        // {%foreach:trigger:<separator>:<before|after>:<text>%}
        // If the current object is a relation, processes <text> for
        // each "before" or "after" trigger. If the current object is
        // a database, processes <text> for all database triggers and the
        // third param is ignored.
        else if ((cmdParams[0] == "trigger") && (cmdParams.Count() >= 4))
        {
            std::vector<Trigger*> triggers;
            
            Relation* r = dynamic_cast<Relation*>(object);
            if (r)
            {
                if (cmdParams[2] == "after")
                    r->getTriggers(triggers, Trigger::afterIUD);
                else if (cmdParams[2] == "before")
                    r->getTriggers(triggers, Trigger::beforeIUD);
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
                    cmdParams.from(3), *it);
            }
        }

        // {%foreach:privilegeitem:<separator>:<type>:<text>%}
        // If the current object is a privilege, processes <text> for each privilege item
        // of the specified <type> (SELECT, INSERT, UPDATE, DELETE, EXECUTE, REFERENCES,
        // MEMBER OF).
        else if ((cmdParams[0] == "privilegeitem") && (cmdParams.Count() >= 4))
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
                    cmdParams.from(3), &(*it));
            }
        }

        // {%foreach:privilege:<separator>:<text>%}
        // If the current object is a relation, procedure or role,
        // processes <text> for each privilege.
        else if (cmdParams[0] == "privilege")
        {
            
            Relation* rel = dynamic_cast<Relation*>(object);
            Procedure* proc = dynamic_cast<Procedure*>(object);
            Role* role = dynamic_cast<Role*>(object);
            Function* func = dynamic_cast<Function*>(object);
            Package* pack = dynamic_cast<Package*>(object);
            Generator* gen = dynamic_cast<Generator*>(object);
            Exception* exc = dynamic_cast<Exception*>(object);
            Domain* dom = dynamic_cast<Domain*>(object);
            std::vector<Privilege>* p = 0;
            if (rel)
                p = rel->getPrivileges();
            if (proc)
                p = proc->getPrivileges();
            if (role)
                p = role->getPrivileges();
            if (func)
                p = func->getPrivileges();
            if (pack)
                p = pack->getPrivileges();
            if (gen)
                p = gen->getPrivileges();
            if (exc)
                p = exc->getPrivileges();
            if (dom)
                p = dom->getPrivileges();
            if (!p)
                return;
            bool firstItem = true;
            for (std::vector<Privilege>::iterator it = p->begin(); it != p->end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.from(2), &(*it));
            }
        }
        // {%foreach:depends_on:<separator>:<text>%}
        // Lists all objects on which the current object depends.
        // {%foreach:dependent:<separator>:<text>%}
        // Lists all objects that depend on the current object.
        else if (cmdParams[0] == "depends_on" || cmdParams[0] == "dependent")
        {
            if (!metadataItem)
                return;

            std::vector<Dependency> deps;
            metadataItem->getDependencies(deps, cmdParams[0] == "depends_on");
            bool firstItem = true;
            for (std::vector<Dependency>::iterator it = deps.begin(); it != deps.end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.from(2), &(*it));
            }
        }
        else if (cmdParams[0] == "fieldDependencyInfo") {
            if (!metadataItem)
                return;

            std::vector<DependencyField> depFields;
            metadataItem->getDependenciesPivoted(depFields);
            bool firstItem = true;
            for (std::vector<DependencyField>::iterator it = depFields.begin(); it != depFields.end(); ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.from(2), &(*it));
            }
        }

        // {%foreach:fieldDependencyObjects:separator:text%}
        // If the current object is a dependency, expands to the requested
        // property of the dependency object.
        else if (cmdParams[0] == "fieldDependencyObjects")
        {
            DependencyField* cb = dynamic_cast<DependencyField*>(object);
            if (!cb)
                return;

            if (!metadataItem)
                return;
            DependencyField* mt = dynamic_cast<DependencyField*>(metadataItem);

            std::vector<Dependency> deps;
            mt->getDependencies(deps);
            bool firstItem = true;
            for (std::vector<Dependency>::iterator it = deps.begin(); it != deps.end(); ++it)
            {
                    Local::foreachIteration(firstItem, tp, processedText, sep,
                        cmdParams.from(2), &(*it));
            }
        }

        // {%foreach:parameter:<separator>:<input|output>:<text>%}
        // If the current object is a procedure, processes <text> for
        // each "input" or "output" parameter.
        else if ((cmdParams[0] == "parameter") && (cmdParams.Count() >= 4))
        {
            Procedure* p = dynamic_cast<Procedure*>(object);
            Function* f = dynamic_cast<Function*>(object);
            if (p) {
                SubjectLocker locker(p);
                p->ensureChildrenLoaded();
                bool isOut = (cmdParams[2] == "output");
                bool firstItem = true;
                for (ParameterPtrs::iterator it = p->begin(); it != p->end(); ++it)
                {
                    if ((*it)->isOutputParameter() == isOut)
                    {
                        Local::foreachIteration(firstItem, tp, processedText, sep,
                            cmdParams.from(3), (*it).get());
                    }
                }
            }
            if (f) {
                SubjectLocker locker(f);
                f->ensureChildrenLoaded();
                bool isOut = (cmdParams[2] == "output");
                bool firstItem = true;
                for (ParameterPtrs::iterator it = f->begin(); it != f->end(); ++it)
                {
                    if ((*it)->isOutputParameter() == isOut)
                    {
                        Local::foreachIteration(firstItem, tp, processedText, sep,
                            cmdParams.from(3), (*it).get());
                    }
                }
            }
        }
        // {%foreach:method:<separator>:<function|procedure>:<text>%}
        // If the current object is a package, processes <text> for
        // each "function" or "procedure" method.
        else if ((cmdParams[0] == "method") && (cmdParams.Count() >= 4))
        {
            Package* p = dynamic_cast<Package*>(object);
            if (p) {
                SubjectLocker locker(p);
                p->ensureChildrenLoaded();
                //bool isFunction = (cmdParams[2] == "function");
                bool firstItem = true;
                for (MethodPtrs::iterator it = p->begin(); it != p->end(); ++it)
                {
                    //if ((*it)->isFunction() == isFunction)
                    {
                        Local::foreachIteration(firstItem, tp, processedText, sep,
                            cmdParams.from(3), (*it).get());
                    }
                }
            }
        }
        // {%foreach:user:<separator>:<text>%}
        // If the current object is a server, processes
        // the specified text once for each defined user,
        // switching each time the current object to the nth user.
        else if (cmdParams[0] == "user")
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

            UserPtrs users = s->getUsers(pi);
            if (users.empty())
                return;

            bool firstItem = true;
            for (UserPtrs::iterator it = users.begin(); it != users.end();
                ++it)
            {
                Local::foreachIteration(firstItem, tp, processedText, sep,
                    cmdParams.from(2), (*it).get());
            }
        }
        // add more collections here.
        else
            return;
    }

    // {%owner_name%}
    // If the current object is a procedure, relation or role
    // expands to the owner's name.
    else if (cmdName == "owner_name")
    {
        wxString name;
        if (Relation* r = dynamic_cast<Relation*>(object))
            name = r->getOwner();
        else if (Procedure* p = dynamic_cast<Procedure*>(object))
            name = p->getOwner();
        else if (Role* role = dynamic_cast<Role*>(object))
            name = role->getOwner();
        else if (Function* f = dynamic_cast<Function*>(object))
            name = f->getOwner();
        else if (Package* pk = dynamic_cast<Package*>(object))
            name = pk->getOwner();
        if (!name.IsEmpty())
            processedText += tp->escapeChars(name);
    }

    // {%object_description%}
    // Expands to the current object's description, if available.
    // Otherwise expands to an empty string.
    else if ((cmdName == "object_description") && metadataItem)
    {
        wxString desc;
        if (metadataItem->getDescription(desc))
        {
            if (desc.empty())
                desc = _("No description");
            processedText += tp->escapeChars(desc);
        }
    }

    // {%dependencyinfo:property%}
    // If the current object is a dependency, expands to the requested
    // property of the dependency object.
    else if ((cmdName == "dependencyinfo") && (cmdParams.Count() >= 1)
        && (cmdParams[0] == "fields"))
    {
        Dependency* d = dynamic_cast<Dependency*>(object);
        if (!d)
            return;

        processedText += d->getFields();
    }
    else if (cmdName == "auxiliar")
    {
        Dependency* d = dynamic_cast<Dependency*>(object);
        if (!d)
            return;
        if ((cmdParams.Count() >= 1) && (cmdParams[0] == "isnull"))
          processedText += tp->escapeChars(getBooleanAsString(d->getAuxiliar()==0));
        else if(d->getAuxiliar())
            processedText += tp->escapeChars(d->getAuxiliar()->getName_());
    }

    // {%primary_key:<text>%}
    // If the current object is a table, processes <text> after switching
    // the current object to the table's primary key.
    else if (cmdName == "primary_key")
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;

        PrimaryKeyConstraint* pk = t->getPrimaryKey();
        if (!pk)
            return;

        tp->internalProcessTemplateText(processedText, cmdParams.all(), pk);
    }

    // {%no_pk_or_unique%}
    // Expands to true if the current object is a table without a primary
    // key or at least one unique constraint, false otherwise.
    else if (cmdName == "no_pk_or_unique")
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;

        if (!t->getPrimaryKey() && t->getUniqueConstraints()->size() == 0)
            processedText += getBooleanAsString(true);
        else
            processedText += getBooleanAsString(false);
    }

    // {%checkconstraintinfo:<property>%}
    // If the current object is a check constraint, expands to the constraint's
    // requested property.
    else if ((cmdName == "checkconstraintinfo") && (cmdParams.Count() >= 1))
    {
        CheckConstraint* c = dynamic_cast<CheckConstraint*>(object);
        if (!c)
            return;

        if (cmdParams[0] == "source")
            processedText += tp->escapeChars(c->getSource(), false);
    }

    // {%constraintinfo:columns%} (uses default separator and suffix)
    // {%constraintinfo:columns:separator%} (uses default suffix)
    // {%constraintinfo:columns:separator:suffix%}
    // If the current object is a column constraint (primary key, unique
    // constraint, foreign key, check constraint), expands to
    // a delimited list of constraint columns.
    else if ((cmdName == "constraintinfo") && (cmdParams.Count() >= 1)
        && (cmdParams[0] == "columns"))
    {
        ColumnConstraint* c = dynamic_cast<ColumnConstraint*>(object);
        if (!c)
            return;

        if (cmdParams.Count() == 1)
            processedText += c->getColumnList();
        else if (cmdParams.Count() == 2)
            processedText += c->getColumnList(cmdParams[1]);
        else if (cmdParams.Count() >= 3)
            processedText += c->getColumnList(cmdParams[1], cmdParams.from(2));
    }

    // {%fkinfo:<property>%}
    // If the current object is a foreign key, expands to the foreign key's
    // requested property.
    else if ((cmdName == "fkinfo") && (cmdParams.Count() >= 1))
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;

        if (cmdParams[0] == "ref_columns")
            processedText += tp->escapeChars(fk->getReferencedColumnList());
        else if (cmdParams[0] == "ref_table")
            processedText += tp->escapeChars(fk->getReferencedTable());
        else if (cmdParams[0] == "update_action")
            processedText += tp->escapeChars(fk->getUpdateAction());
        else if (cmdParams[0] == "delete_action")
            processedText += tp->escapeChars(fk->getDeleteAction());
    }

    // {%columninfo:<property>%}
    // If the current object is a column, expands to the column's
    // requested property.
    else if ((cmdName == "columninfo") && (cmdParams.Count() >= 1))
    {
        ColumnBase* cb = dynamic_cast<ColumnBase*>(object);
        if (!cb)
            return;

        if (cmdParams[0] == "datatype")
            processedText += tp->escapeChars(cb->getDatatype());
        else if(cmdParams[0] == "typeof") {
            processedText += "TYPE OF COLUMN " + cb->getParent()->getQuotedName() + "." + cb->getQuotedName();
        }
        else if (cmdParams[0] == "source") {
            processedText += tp->escapeChars(cb->getSource());
        }
        else if (cmdParams[0] == "is_nullable")
        {
            processedText += tp->escapeChars(getBooleanAsString(
                cb->isNullable(CheckDomainNullability)));
        }
        else if (cmdParams[0] == "null_option")
        {
            if (!cb->isNullable(CheckDomainNullability))
                processedText += tp->escapeChars("not null");
        }
        else if (cmdParams[0] == "default_expression")
        {
            wxString defaultValue;
            if (cb->getDefault(ReturnDomainDefault, defaultValue))
            {
                defaultValue = Domain::trimDefaultValue(defaultValue);
                processedText += tp->escapeChars(defaultValue, false);
            }
        }
    }

    // {%viewinfo:<property>%}
    // If the current object is a view, expands to the view's
    // requested property.
    else if ((cmdName == "viewinfo") && (cmdParams.Count() >= 1))
    {
        View* v = dynamic_cast<View*>(object);
        if (!v)
            return;

        if (cmdParams[0] == "source")
            processedText += tp->escapeChars(v->getSource(), false);
    }

    // {%procedureinfo:<property>%}
    // If the current object is a procedure, expands to the procedure's
    // requested property.
    else if ((cmdName == "procedureinfo") && (cmdParams.Count() >= 1))
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;

        if (cmdParams[0] == "source")
            processedText += tp->escapeChars(p->getSource(), false);
        // {%procedureinfo:paramcount[:input|output]%}
        // Expands to the number of input or output parameters.
        // If no argument is specified, expands to the total number of
        // parameters.
        else if (cmdParams[0] == "paramcount")
        {
            size_t paramCount(0);
            p->ensureChildrenLoaded();
            if (cmdParams.Count() >= 2)
            {
                bool isOut = (cmdParams[1] == "output");
                for (ParameterPtrs::iterator it = p->begin(); it != p->end(); ++it)
                {
                    if ((*it)->isOutputParameter() == isOut)
                        paramCount++;
                }
            }
            else
                paramCount = p->getParamCount();

            processedText += tp->escapeChars(
                wxString::Format("%zu", paramCount), false);
        }
    }

    // {%triggerinfo:<property>%}
    // If the current object is a trigger, expands to the trigger's
    // requested property.
    else if ((cmdName == "triggerinfo") && (cmdParams.Count() >= 1))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        if (!t)
            return;

        if (cmdParams[0] == "name")
            processedText += tp->escapeChars(t->getRelationName());
        else if (cmdParams[0] == "source")
            processedText += tp->escapeChars(t->getSource(), false);
        else if (cmdParams[0] == "position")
            processedText << t->getPosition();
        else if (cmdParams[0] == "type")
            processedText += tp->escapeChars(t->getFiringEvent());
        else if (cmdParams[0] == "is_active")
            processedText += tp->escapeChars(getBooleanAsString(t->getActive()));
        else if (cmdParams[0] == "is_db_trigger")
            processedText += tp->escapeChars(getBooleanAsString(t->isDBTrigger()));
    }

    // {%generatorinfo:<property>%}
    // If the current object is a generator, expands to the generator's
    // requested property.
    else if ((cmdName == "generatorinfo") && (cmdParams.Count() >= 1))
    {
        Generator* g = dynamic_cast<Generator*>(object);
        if (!g)
            return;

        if (cmdParams[0] == "value")
            processedText << g->getValue();
        else if (cmdParams[0] == "source")
            processedText << g->getSource();
    }

    // {%exceptioninfo:<property>%}
    // If the current object is an exception, expands to the exception's
    // requested property.
    else if ((cmdName == "exceptioninfo") && (cmdParams.Count() >= 1))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;

        if (cmdParams[0] == "number")
            processedText << e->getNumber();
        else if (cmdParams[0] == "message")
            processedText << e->getMessage();
    }

    // {%udfinfo:<property>%}
    // If the current object is a function, expands to the function's
    // requested property.
    else if (cmdName == "udfinfo" && !cmdParams.IsEmpty())
    {
        UDF* f = dynamic_cast<UDF*>(object);
        if (!f)
            return;

        if (cmdParams[0] == "library")
            processedText += tp->escapeChars(f->getLibraryName());
        else if (cmdParams[0] == "entry_point")
            processedText += tp->escapeChars(f->getEntryPoint());
        else if (cmdParams[0] == "definition")
            processedText += tp->escapeChars(f->getDefinition(), false);
        else if (cmdParams[0] == "source")
            processedText += tp->escapeChars(f->getSource(), false);
    }
    // {%functioninfo:<property>%}
    // If the current object is a function, expands to the function's
    // requested property.
    else if (cmdName == "functioninfo" && !cmdParams.IsEmpty())
    {
        FunctionSQL* f = dynamic_cast<FunctionSQL*>(object);
        if (!f)
            return;

        if (cmdParams[0] == "definition")
            processedText += tp->escapeChars(f->getDefinition(), false);
        else if (cmdParams[0] == "source")
            processedText += tp->escapeChars(f->getSource(), false);
    }

    // {%indexinfo:<property>%}
    // If the current object is an index, expands to the index's
    // requested property.
    else if (cmdName == "indexinfo" && !cmdParams.IsEmpty())
    {
        Index* i = dynamic_cast<Index*>(object);
        if (!i)
            return;

        if (cmdParams[0] == "type")
            processedText += (i->getIndexType() == Index::itAscending ? "ASC" : "DESC");
        else if (cmdParams[0] == "stats")
            processedText += wxString::Format("%.6f", i->getStatistics());
        else if (cmdParams[0] == "fields")
            processedText += i->getFieldsAsString();
        else if (cmdParams[0] == "is_active")
            processedText += getBooleanAsString(i->isActive());
        else if (cmdParams[0] == "is_unique")
            processedText += getBooleanAsString(i->isUnique());
        else if (cmdParams[0] == "is_ascending")
            processedText += getBooleanAsString(i->getIndexType() == Index::itAscending);
    }

    // {%object_ddl%}
    // Expands to the current object's DDL definition.
    else if ((cmdName == "object_ddl") && metadataItem)
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
    else if (cmdName == "dbinfo" && !cmdParams.IsEmpty())
    {
        Database* db = dynamic_cast<Database*>(object);
        if (!db)
            return;

        if (cmdParams[0] == "connection_string")
            processedText += db->getConnectionString();
        else if (cmdParams[0] == "ods_version")
        {
            processedText += wxString() << db->getInfo().getODS();
            if (db->getInfo().getODSMinor())
            {
                processedText += ".";
                processedText += wxString() << db->getInfo().getODSMinor();
            }
        }
        else if (cmdParams[0] == "page_size")
            processedText += wxString() << db->getInfo().getPageSize();
        else if (cmdParams[0] == "pages")
            processedText += wxString() << db->getInfo().getPages();
        else if (cmdParams[0] == "size")
        {
            int64_t size = db->getInfo().getSizeInBytes();
            const double kilo = 1024;
            const double mega = kilo * kilo;
            const double giga = kilo * mega;
            if (size >= giga)
                processedText += wxString::Format("%0.2fGB", size / giga);
            else if (size >= mega)
                processedText += wxString::Format("%0.2fMB", size / mega);
            else
                processedText += wxString::Format("%0.2fkB", size / kilo);
        }
        else if (cmdParams[0] == "page_buffers")
            processedText += wxString() << db->getInfo().getBuffers();
        else if (cmdParams[0] == "reserve_space")
            processedText += getBooleanAsString(db->getInfo().getReserve());
        else if (cmdParams[0] == "is_read_only")
            processedText += getBooleanAsString(db->getInfo().getReadOnly());
        else if (cmdParams[0] == "sql_dialect")
            processedText += wxString() << db->getSqlDialect();
        else if (cmdParams[0] == "default_charset")
            processedText += db->getDatabaseCharset();
        else if (cmdParams[0] == "sweep_interval")
            processedText += wxString() << db->getInfo().getSweep();
        else if (cmdParams[0] == "forced_writes")
            processedText += getBooleanAsString(db->getInfo().getForcedWrites());
        else if (cmdParams[0] == "oldest_transaction")
            processedText += wxString() << db->getInfo().getOldestTransaction();
        else if (cmdParams[0] == "oldest_active_transaction")
            processedText += wxString() << db->getInfo().getOldestActiveTransaction();
        else if (cmdParams[0] == "oldest_snapshot")
            processedText += wxString() << db->getInfo().getOldestSnapshot();
        else if (cmdParams[0] == "next_transaction")
            processedText += wxString() << db->getInfo().getNextTransaction();
        else if (cmdParams[0] == "connected_users")
        {
            wxArrayString users;
            db->getConnectedUsers(users);
            processedText += wxArrayToString(users, ",");
        }
        else if (cmdParams[0] == "linger")
            processedText += wxString() << db->getLinger();
        else if (cmdParams[0] == "sql_security")
            processedText += wxString() << db->getSqlSecurity();
    }

    // {%privilegeinfo:<property>%}
    // If the current object is a privilege, expands to the privilege's
    // requested property.
    else if (cmdName == "privilegeinfo" && (cmdParams.Count() > 0))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
            
        if (cmdParams[0] == "grantee_name")
            processedText += tp->escapeChars(p->getGrantee());
    }

    // {%privilegeitemcount:<type>%}
    // If the current object is a privilege, expands to the count of
    // privilege items of the requested <type>.
    else if ((cmdName == "privilegeitemcount") && (cmdParams.Count() >= 1))
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
    else if ((cmdName == "privilegeiteminfo") && (cmdParams.Count() >= 1))
    {
        PrivilegeItem* pi = dynamic_cast<PrivilegeItem*>(object);
        if (!pi)
            return;
        
        if (cmdParams[0] == "grantor")
            processedText += pi->grantor;
        else if (cmdParams[0] == "grant_option")
            processedText += getBooleanAsString(pi->grantOption);
        else if (cmdParams[0] == "columns")
        {
            for (std::vector<wxString>::iterator it = pi->columns.begin();
                it != pi->columns.end(); ++it)
            {
                if (it != pi->columns.begin())
                    processedText += ",";
                processedText += (*it);
            }
        }
    }

    // {%userinfo:property%}
    // If the current object  is a user, expands to various user properties.
    else if (cmdName == "userinfo" && !cmdParams.IsEmpty())
    {
        User* u = dynamic_cast<User*>(object);
        if (!u)
            return;

        if (cmdParams[0] == "username")
            processedText += tp->escapeChars(u->getUsername());
        else if (cmdParams[0] == "first_name")
            processedText += tp->escapeChars(u->getFirstName());
        else if (cmdParams[0] == "middle_name")
            processedText += tp->escapeChars(u->getMiddleName());
        else if (cmdParams[0] == "last_name")
            processedText += tp->escapeChars(u->getLastName());
        else if (cmdParams[0] == "unix_user")
            processedText << u->getUserId();
        else if (cmdParams[0] == "unix_group")
            processedText << u->getGroupId();
    }
    // {%sql_security%}
    // If the current object is a data base, procedure, relation, 
    // function or trigger expands to the SQL Security.
    else if (cmdName == "sql_security")
    {
    wxString name;
    if (Relation* r = dynamic_cast<Relation*>(object))
        name = r->getSqlSecurity();
    else if (Procedure* p = dynamic_cast<Procedure*>(object))
        name = p->getSqlSecurity();
    else if (Function* f = dynamic_cast<Function*>(object))
        name = f->getSqlSecurity();
    else if (Trigger* t = dynamic_cast<Trigger*>(object))
        name = t->getSqlSecurity();
    else if (Package* pk = dynamic_cast<Package*>(object))
        name = pk->getSqlSecurity();
    if (!name.IsEmpty())
        processedText += tp->escapeChars(name);
    }
    // {%packageinfo:<property>%}
    // If the current object is a package, expands to the package's
    // requested property.
    else if ((cmdName == "packageinfo") && (cmdParams.Count() >= 1))
    {
        Package* p = dynamic_cast<Package*>(object);
        if (!p)
            return;
        if (cmdParams[0] == "definition")
            processedText += tp->escapeChars(p->getDefinition(), false);
        if (cmdParams[0] == "source")
            processedText += tp->escapeChars(p->getSource(), false);
    }

}

