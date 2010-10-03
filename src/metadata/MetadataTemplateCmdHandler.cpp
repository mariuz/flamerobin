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
#include "metadata/user.h"

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

    else if (cmdName == wxT("object_description"))
    {
        wxString desc;
        if (object->getDescription(desc))
        {
            if (desc.IsEmpty())
                desc = _("No description");
            processedText += tp->escapeChars(desc);
            if (!cmdParams.IsEmpty())
                tp->internalProcessTemplateText(processedText, cmdParams.all(), object);
        }
    }

    else if (cmdName == wxT("relation_columns") && !cmdParams.IsEmpty())
    {
        Relation* r = dynamic_cast<Relation*>(object);
        if (!r)
            return;
        r->ensureChildrenLoaded();
        for (RelationColumns::iterator it = r->begin(); it != r->end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), (*it).get());
    }

    else if (cmdName == wxT("relation_triggers") && !cmdParams.IsEmpty())
    {
        Relation* r = dynamic_cast<Relation*>(object);
        if (!r)
            return;
        std::vector<Trigger*> tmp;
        if (cmdParams[0] == wxT("after"))
            r->getTriggers(tmp, Trigger::afterTrigger);
        else if (cmdParams[0] == wxT("before"))
            r->getTriggers(tmp, Trigger::beforeTrigger);
        cmdParams.RemoveAt(0);

        for (std::vector<Trigger*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), *it);
    }

    else if (cmdName == wxT("db_triggers") && !cmdParams.IsEmpty())
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        std::vector<Trigger*> tmp;
        d->getDatabaseTriggers(tmp);
        for (std::vector<Trigger*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), *it);
    }

    else if (cmdName == wxT("depends_on") || cmdName == wxT("depend_of"))
    {
        MetadataItem* m = dynamic_cast<MetadataItem*>(object);
        if (!m)
            return;
        std::vector<Dependency> tmp;
        m->getDependencies(tmp, cmdName == wxT("depends_on"));
        for (std::vector<Dependency>::iterator it = tmp.begin(); it != tmp.end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), &(*it));
    }

    else if (cmdName == wxT("dependency_columns"))
    {
        Dependency* d = dynamic_cast<Dependency*>(object);
        if (!d)
            return;
        processedText += d->getFields();
    }

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

    else if (cmdName == wxT("foreign_keys"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<ForeignKey>* fk = t->getForeignKeys();
        if (!fk)
            return;
        for (std::vector<ForeignKey>::iterator it = fk->begin(); it != fk->end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), &(*it));
    }

    else if (cmdName == wxT("check_constraints"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<CheckConstraint>* c = t->getCheckConstraints();
        if (!c)
            return;
        for (std::vector<CheckConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), &(*it));
    }

    else if (cmdName == wxT("unique_constraints"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<UniqueConstraint>* c = t->getUniqueConstraints();
        if (!c)
            return;
        for (std::vector<UniqueConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            tp->internalProcessTemplateText(processedText, cmdParams.all(), &(*it));
    }

    else if (cmdName == wxT("check_source"))
    {
        CheckConstraint* c = dynamic_cast<CheckConstraint*>(object);
        if (!c)
            return;
        processedText += tp->escapeChars(c->sourceM);
    }

    // {%constraint_columns%} (uses default separator and suffix)
    // {%constraint_columns:separator%} (uses default suffix)
    // {%constraint_columns:separator:suffix%}
    else if (cmdName == wxT("constraint_columns"))
    {
        ColumnConstraint* c = dynamic_cast<ColumnConstraint*>(object);
        if (!c)
            return;

        if (cmdParams.IsEmpty())
            processedText += c->getColumnList();
        else if (cmdParams.Count() == 1)
            processedText += c->getColumnList(cmdParams[0]);
        else if (cmdParams.Count() >= 2)
            processedText += c->getColumnList(cmdParams[0], cmdParams[1]);
    }

    else if (cmdName == wxT("fk_referenced_columns"))
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;
        processedText += fk->getReferencedColumnList();
    }

    else if (cmdName == wxT("fk_table"))
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;
        processedText += fk->referencedTableM;
    }

    else if (cmdName == wxT("fk_update"))
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;
        processedText += fk->updateActionM;
    }

    else if (cmdName == wxT("fk_delete"))
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;
        processedText += fk->deleteActionM;
    }

    // TODO: change to a single columninfo command.
    else if (cmdName == wxT("column_datatype"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
        {   // needs newlines escaped for computed column source
            processedText += tp->escapeChars(c->getDatatype());
            // TODO: make the domain name (if any) a link to the domain's property page?
        }
    }

    else if (cmdName == wxT("column_nulloption"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
            processedText += (c->isNullable() ? wxT("") : wxT("not null"));
    }

    else if (cmdName == wxT("column_default"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
        {
            wxString def(c->getDefault());
            def.Trim(false);    // left trim
            if (def.Upper().StartsWith(wxT("DEFAULT")))
            {
                def.Remove(0, 7);
                def.Trim(false);
            }
            processedText += def;
        }
    }

    else if (cmdName == wxT("input_parameters") || cmdName == wxT("output_parameters"))     // SP params
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;

        SubjectLocker locker(p);
        p->ensureChildrenLoaded();
        bool parOut = (cmdName == wxT("output_parameters"));
        for (ProcedureParameters::iterator it = p->begin();
            it != p->end(); ++it)
        {
            if ((*it)->isOutputParameter() == parOut)
                tp->internalProcessTemplateText(processedText, cmdParams.all(), (*it).get());
        }
    }

    else if (cmdName == wxT("view_source"))
    {
        View* v = dynamic_cast<View*>(object);
        if (!v)
            return;
        processedText += tp->escapeChars(v->getSource(), false);
    }

    else if (cmdName == wxT("procedure_source"))
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;
        processedText += tp->escapeChars(p->getSource(), false);
    }

    else if (cmdName == wxT("trigger_source"))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        if (!t)
            return;
        processedText += tp->escapeChars(t->getSource(), false);
    }

    // TODO: switch to separate values for a nicer grid in the property page.
    else if (cmdName == wxT("trigger_info"))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        wxString object, type;
        bool active, isDBtrigger;
        int position;
        if (!t)
            return;
        t->getTriggerInfo(object, active, position, type, isDBtrigger);
        wxString s(active ? wxT("Active ") : wxT("Inactive "));
        s << type << wxT(" trigger ");
        if (!isDBtrigger)
            s << wxT("for ") << object;
        s << wxT(" at position ") << position;
        processedText += tp->escapeChars(s);
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
                tp->internalProcessTemplateText(newText, cmdParams[2], (*it).get());
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }

        // {%foreach:privilegeitem:<separator>:<type>:<text>%}
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
                tp->internalProcessTemplateText(newText, cmdParams[3], &(*it));
                if ((!firstItem) && (!newText.IsEmpty()))
                    processedText += sep;                    
                if (!newText.IsEmpty())
                    firstItem = false;
                processedText += newText;
            }
        }

        // {%foreach:privilege:<separator>:<text>%}
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
