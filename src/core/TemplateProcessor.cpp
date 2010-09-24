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
#include <vector>
#include <map>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "frutils.h"
#include "core/ProgressIndicator.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/metadataitem.h"
#include "metadata/server.h"
#include "TemplateProcessor.h"
//-----------------------------------------------------------------------------
TemplateProcessor::TemplateProcessor(MetadataItem *m,
    std::vector<MetadataItem *> *allowedObjects)
    : objectM(m), flagNextM(false)
{
    if (allowedObjects) // optional vector of objects that should be processed
    {
        allowedObjectsM = *allowedObjects;
    }
}
//-----------------------------------------------------------------------------
wxString getBooleanAsString(bool value)
{
    return (value) ? wxT("true") : wxT("false");
}
//-----------------------------------------------------------------------------
void TemplateProcessor::processCommand(wxString cmdName, TemplateCmdParams cmdParams,
    MetadataItem *object, wxString& processedText, wxWindow *window, bool first)
{
    if (cmdName == wxT("template_root"))
        processedText += getTemplatePath();

    else if (cmdName == wxT("getvar") && !cmdParams.empty())
        processedText += getVar(cmdParams.all());

    else if (cmdName == wxT("setvar") && !cmdParams.empty())
    {
        if (cmdParams.Count() == 1)
            clearVar(cmdParams[0]);
        else
            setVar(cmdParams[0], cmdParams[1]);
    }

    else if (cmdName == wxT("clearvar") && !cmdParams.empty())
        clearVar(cmdParams.all());

    else if (cmdName == wxT("clearvars"))
        clearVars();

    else if (cmdName == wxT("getconf") && !cmdParams.empty())
        processedText += configM.get(cmdParams.all(), wxString(wxT("")));

    else if (cmdName == wxT("setconf") && !cmdParams.empty())
    {
        if (cmdParams.Count() == 1)
            configM.setValue(cmdParams[0], wxString(wxT("")));
        else
            configM.setValue(cmdParams[0], cmdParams[1]);
    }

    else if (cmdName == wxT("object_name"))
        processedText += object->getName_();

    else if (cmdName == wxT("parent"))
    {
        if (object->getParent())
            internalProcessTemplateText(processedText, cmdParams.all(), object->getParent(), window);
    }

    else if (cmdName == wxT("object_type"))
        processedText += object->getTypeName();

    else if (cmdName == wxT("object_address"))
        processedText += wxString::Format(wxT("%ld"), (uintptr_t)object);

    else if (cmdName == wxT("parent_window"))
        processedText += wxString::Format(wxT("%ld"), (uintptr_t)window);

    else if (cmdName == wxT("separator") && !cmdParams.empty())
    {
        if (!first)
            processedText += cmdParams.all();
    }

    else if (cmdName == wxT("is_system"))
        processedText += getBooleanAsString(object->isSystem());

    else if (cmdName == wxT("users") && !cmdParams.empty())
    {
        Server* s = dynamic_cast<Server*>(object);
        if (!s)
            return;

        ProgressIndicator* pi = getProgressIndicator();
        if (pi)
        {
            pi->initProgress(_("Connecting to Server..."));
            pi->doShow();
        }
        UserList* usr = s->getUsers(pi);
        if (!usr || !usr->size())
        {
            window->Close();
            return;
        }
        for (UserList::iterator it = usr->begin(); it != usr->end(); ++it)
            internalProcessTemplateText(processedText, cmdParams.all(), &(*it), window);
    }

    else if (cmdName == wxT("userinfo") && !cmdParams.empty())
    {
        User* u = dynamic_cast<User*>(object);
        if (!u)
            return;
        if (cmdParams[0] == wxT("username"))
            processedText += escapeChars(u->usernameM);
        else if (cmdParams[0] == wxT("first_name"))
            processedText += escapeChars(u->firstnameM);
        else if (cmdParams[0] == wxT("middle_name"))
            processedText += escapeChars(u->middlenameM);
        else if (cmdParams[0] == wxT("last_name"))
            processedText += escapeChars(u->lastnameM);
        else if (cmdParams[0] == wxT("unix_user"))
            processedText << u->useridM;
        else if (cmdParams[0] == wxT("unix_group"))
            processedText << u->groupidM;
    }

    else if (cmdName == wxT("owner_name"))
    {
        wxString name;
        if (Relation* r = dynamic_cast<Relation*>(object))
            name = r->getOwner();
        else if (Procedure* p = dynamic_cast<Procedure*>(object))
            name = p->getOwner();
        else if (Role* r = dynamic_cast<Role*>(object))
            name = r->getOwner();
        if (!name.empty())
            processedText += escapeChars(name, false);
    }

    else if (cmdName == wxT("object_description"))
    {
        wxString desc;
        if (object->getDescription(desc))
        {
            if (desc.empty())
                desc = _("No description");
            processedText += escapeChars(desc);
            if (!cmdParams.empty())
                internalProcessTemplateText(processedText, cmdParams.all(), object, window);
        }
    }

    // TODO: replace with vars and ifeq?
    else if (cmdName == wxT("show_if_config") && !cmdParams.empty())
    {
        if (cmdParams.Count() >= 2)
        {
            if (config().get(cmdParams[0], (cmdParams[1] == wxT("true"))))
            {
                cmdParams.RemoveAt(0, 2);
                internalProcessTemplateText(processedText, cmdParams.all(), object, window);
            }
        }
    }

    else if (cmdName == wxT("ifeq") && (cmdParams.Count() >= 3))
    {
        wxString val1;
        internalProcessTemplateText(val1, cmdParams[0], object, window);
        wxString val2;
        internalProcessTemplateText(val2, cmdParams[1], object, window);
        wxString trueText;
        internalProcessTemplateText(trueText, cmdParams[2], object, window);
        wxString falseText;
        if (cmdParams.Count() >= 4)
        {
            cmdParams.RemoveAt(0, 3);
            internalProcessTemplateText(falseText, cmdParams.all(), object, window);
        }
        if (val1 == val2)
            processedText += trueText;
        else
            processedText += falseText;
    }

    else if (cmdName == wxT("columns") && !cmdParams.empty())  // table and view columns
    {
        Relation* r = dynamic_cast<Relation*>(object);
        if (!r)
            return;
        r->ensureChildrenLoaded();
        for (RelationColumns::iterator it = r->begin(); it != r->end(); ++it)
            internalProcessTemplateText(processedText, cmdParams.all(), (*it).get(), window, it == r->begin());
    }

    else if (cmdName == wxT("relation_triggers") && !cmdParams.empty())
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
            internalProcessTemplateText(processedText, cmdParams.all(), *it, window, it == tmp.begin());
    }

    else if (cmdName == wxT("db_triggers") && !cmdParams.empty())
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        std::vector<Trigger*> tmp;
        d->getDatabaseTriggers(tmp);
        for (std::vector<Trigger*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
            internalProcessTemplateText(processedText, cmdParams.all(), *it, window, it == tmp.begin());
    }

    else if (cmdName == wxT("depends_on") || cmdName == wxT("depend_of"))
    {
        MetadataItem* m = dynamic_cast<MetadataItem*>(object);
        if (!m)
            return;
        std::vector<Dependency> tmp;
        m->getDependencies(tmp, cmdName == wxT("depends_on"));
        for (std::vector<Dependency>::iterator it = tmp.begin(); it != tmp.end(); ++it)
            internalProcessTemplateText(processedText, cmdParams.all(), &(*it), window);
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
        internalProcessTemplateText(processedText, cmdParams.all(), pk, window);
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
            internalProcessTemplateText(processedText, cmdParams.all(), &(*it), window, it == fk->begin());
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
            internalProcessTemplateText(processedText, cmdParams.all(), &(*it), window, it == c->begin());
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
            internalProcessTemplateText(processedText, cmdParams.all(), &(*it), window, it == c->begin());
    }

    else if (cmdName == wxT("privileges"))
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
            internalProcessTemplateText(processedText, cmdParams.all(), &(*it), window, it == p->begin());
    }

    else if (cmdName == wxT("grantee_name"))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
        processedText += escapeChars(p->getGrantee());
    }

    else if (cmdName == wxT("check_source"))
    {
        CheckConstraint* c = dynamic_cast<CheckConstraint*>(object);
        if (!c)
            return;
        processedText += escapeChars(c->sourceM);
    }

    // {%constraint_columns%} (uses default separator and suffix)
    // {%constraint_columns:separator%} (uses default suffix)
    // {%constraint_columns:separator:suffix%}
    else if (cmdName == wxT("constraint_columns"))
    {
        ColumnConstraint* c = dynamic_cast<ColumnConstraint*>(object);
        if (!c)
            return;

        if (cmdParams.empty())
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

    else if (cmdName == wxT("column_datatype"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
        {   // needs newlines escaped for computed column source
            processedText += escapeChars(c->getDatatype());
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
                internalProcessTemplateText(processedText, cmdParams.all(), (*it).get(), window, it == p->begin());
        }
    }

    else if (cmdName == wxT("view_source"))
    {
        View* v = dynamic_cast<View*>(object);
        if (!v)
            return;
        processedText += escapeChars(v->getSource(), false);
    }

    else if (cmdName == wxT("procedure_source"))
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;
        processedText += escapeChars(p->getSource(), false);
    }

    else if (cmdName == wxT("trigger_source"))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        if (!t)
            return;
        processedText += escapeChars(t->getSource(), false);
    }

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
        processedText += escapeChars(s, false);
    }

    else if (cmdName == wxT("generator_value"))
    {
        Generator* g = dynamic_cast<Generator*>(object);
        if (!g)
            return;
        std::ostringstream ss;
        ss << g->getValue();
        processedText += escapeChars(std2wx(ss.str()), false);
    }

    else if (cmdName == wxT("exception_number"))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;
        wxString s;
        s << e->getNumber();
        processedText += escapeChars(s, false);
    }

    else if (cmdName == wxT("exception_message"))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;
        processedText += escapeChars(e->getMessage(), false);
    }

    else if (cmdName == wxT("udfinfo") && !cmdParams.empty())
    {
        Function* f = dynamic_cast<Function*>(object);
        if (!f)
            return;

        if (cmdParams[0] == wxT("library"))
            processedText += escapeChars(f->getLibraryName());
        else if (cmdParams[0] == wxT("entry_point"))
            processedText += escapeChars(f->getEntryPoint());
        else if (cmdParams[0] == wxT("definition"))
            processedText += escapeChars(f->getDefinition(), false);
    }

    else if (cmdName == wxT("varcolor") && (cmdParams.Count() >= 2))
    {
        static bool first = false;
        first = !first;
        if (first)
            processedText += cmdParams[0];
        else
            processedText += cmdParams[1];
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
            internalProcessTemplateText(processedText, cmdParams.all(), &(*it), window, it == ix->begin());
    }

    else if (cmdName == wxT("object_ddl"))
    {
        ProgressIndicator* pi = getProgressIndicator();
        if (pi)
        {
            pi->setProgressLevelCount(2);
            pi->initProgress(_("Extracting DDL Definitions"));
            pi->doShow();
        }
        CreateDDLVisitor cdv(pi);
        object->acceptVisitor(&cdv);
        processedText += escapeChars(cdv.getSql(), false);
    }

    else if (cmdName == wxT("indexinfo") && !cmdParams.empty())
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
}
//-----------------------------------------------------------------------------
void TemplateProcessor::internalProcessTemplateText(wxString& processedText, wxString inputText,
    MetadataItem *object, wxWindow *window, bool first)
{
    if (object == 0)
        object = objectM;

    if (flagNextM)  // reuse the first "flag" from skipped object (see below)
        first = true;
    flagNextM = false;

    if (!allowedObjectsM.empty() && std::find(allowedObjectsM.begin(),
        allowedObjectsM.end(), object) == allowedObjectsM.end())
    {
        if (first)  // keep the "first" flag in case current gets skipped
            flagNextM = true;
        return;
    }
    
    // parse commands
    wxString::size_type pos = 0, oldpos = 0, endpos = 0;
    while (true)
    {
        pos = inputText.find(wxT("{%"), pos);
        if (pos == wxString::npos)
        {
            processedText += inputText.substr(oldpos);
            break;
        }

        wxString::size_type check, startpos = pos;
        int cnt = 1;
        while (cnt > 0)
        {
            endpos = inputText.find(wxT("%}"), startpos+1);
            if (endpos == wxString::npos)
                break;

            check = inputText.find(wxT("{%"), startpos+1);
            if (check == wxString::npos)
                startpos = endpos;
            else
            {
                startpos = (check < endpos ? check : endpos);
                if (startpos == check)
                    cnt++;
            }
            if (startpos == endpos)
                cnt--;
            startpos++;
        }

        if (cnt > 0)    // no matching closing %}
            break;

        processedText += inputText.substr(oldpos, pos - oldpos);
        wxString cmd = inputText.substr(pos + 2, endpos - pos - 2); // 2 = start_marker_len = end_marker_len
        
        // parse command name and params.
        wxString cmdName;
        TemplateCmdParams cmdParams;
        
        enum TemplateCmdState
        {
            inText,
            inString1,
            inString2
        };
        TemplateCmdState state = inText;
        wxString buffer;
        unsigned int nestLevel = 0;
        for (wxString::size_type i = 0; i < cmd.Length(); i++)
        {
            wxChar c = cmd[i];
            
            if (c == wxT(':'))
            {
                if ((nestLevel == 0) && (state == inText))
                {
                    cmdParams.Add(buffer);
                    buffer.Clear();
                    continue;
                }
            }
            buffer += c;
            
            if ((c == wxT('{')) && (i < cmd.Length() - 1) && (cmd[i + 1] == wxT('%')))
                nestLevel++;
            else if ((c == wxT('}')) && (i > 0) && (cmd[i - 1] == wxT('%')))
                nestLevel--;
            else if (c == wxT('\''))
                state == inString1 ? state = inText : state = inString1;
            else if (c == wxT('"'))
                state == inString2 ? state = inText : state = inString2;
        }
        if (buffer.Length() > 0)
            cmdParams.Add(buffer);

        if (cmdParams.Count() > 0)
        {
            cmdName = cmdParams[0];
            cmdParams.RemoveAt(0);
            if (!cmdName.empty())
            {
                // process internal commands.
                processCommand(cmdName, cmdParams, object, processedText, window, first);
                // call external command handlers.
                getTemplateCmdHandlerRepository().handleTemplateCmd(this, cmdName, cmdParams,
                    object, processedText, window, first);
            }
        }
        oldpos = pos = endpos + 2;
    }
}
//-----------------------------------------------------------------------------
void TemplateProcessor::processTemplateFile(wxString& processedText, wxFileName inputFileName,
    MetadataItem* object, wxWindow *window, bool first, ProgressIndicator* progressIndicator)
{
    fileNameM = inputFileName;
    inputFileName.SetExt(wxT("conf"));
    configM.setConfigFileName(inputFileName);
    progressIndicatorM = progressIndicator;
    internalProcessTemplateText(processedText, loadEntireFile(fileNameM), object, window, first);
}
//-----------------------------------------------------------------------------
void TemplateProcessor::processTemplateText(wxString& processedText, wxString inputText,
    MetadataItem *object, wxWindow *window, bool first, ProgressIndicator* progressIndicator)
{
    fileNameM.Clear();
    configM.setConfigFileName(wxFileName(wxEmptyString));
    progressIndicatorM = progressIndicator;
    internalProcessTemplateText(processedText, inputText, object, window, first);
}
//-----------------------------------------------------------------------------
void TemplateProcessor::setVar(wxString varName, wxString varValue)
{
    varsM[varName] = varValue;
}
//-----------------------------------------------------------------------------
wxString TemplateProcessor::getVar(wxString varName)
{
    return varsM[varName];
}
//-----------------------------------------------------------------------------
void TemplateProcessor::clearVar(wxString varName)
{
    varsM.erase(varName);
}
//-----------------------------------------------------------------------------
void TemplateProcessor::clearVars()
{
    varsM.clear();
}
//-----------------------------------------------------------------------------
wxString TemplateProcessor::getTemplatePath()
{
    return fileNameM.GetPathWithSep();
}
//-----------------------------------------------------------------------------
const wxChar TemplateCmdParams::SEPARATOR = wxT(':');
//-----------------------------------------------------------------------------
wxString TemplateCmdParams::all() const
{
    wxString result;
    for (size_t i = 0; i < Count(); i++)
    {
        if (i == 0)
          result = Item(i);
        else
          result += wxT(':') + Item(i);
    }
    return result;
}
//-----------------------------------------------------------------------------
TemplateCmdHandlerRepository& getTemplateCmdHandlerRepository()
{
    static TemplateCmdHandlerRepository repository;
    return repository;
}
//-----------------------------------------------------------------------------
//! needed to disallow instantiation
TemplateCmdHandlerRepository::TemplateCmdHandlerRepository()
    : handlerListSortedM(false)
{
}
//-----------------------------------------------------------------------------
TemplateCmdHandlerRepository::~TemplateCmdHandlerRepository()
{
    while (!handlersM.empty())
        removeHandler(handlersM.front());
}
//-----------------------------------------------------------------------------
//! needed in checkHandlerListSorted() to sort on objects instead of pointers
bool templateCmdHandlerPointerLT(const TemplateCmdHandler* left, const TemplateCmdHandler* right)
{
    return *left < *right;
}
//-----------------------------------------------------------------------------
void TemplateCmdHandlerRepository::checkHandlerListSorted()
{
    if (!handlerListSortedM)
    {
        handlersM.sort(templateCmdHandlerPointerLT);
        handlerListSortedM = true;
    }
}
//-----------------------------------------------------------------------------
//! returns false if no suitable handler found
void TemplateCmdHandlerRepository::handleTemplateCmd(TemplateProcessor *tp,
    wxString cmdName, TemplateCmdParams cmdParams, MetadataItem* object,
    wxString& processedText, wxWindow *window, bool first)
{
    checkHandlerListSorted();
    for (std::list<TemplateCmdHandler*>::iterator it = handlersM.begin(); it != handlersM.end(); ++it)
        (*it)->handleTemplateCmd(tp, cmdName, cmdParams, object, processedText, window, first);
}
//-----------------------------------------------------------------------------
void TemplateCmdHandlerRepository::addHandler(TemplateCmdHandler* handler)
{
    // can't do ordered insert here, since the getPosition() function that
    // serves TemplateCmdHandler::operator< is virtual, and this function (addHandler)
    // is called in the constructor of TemplateCmdHandler.
    // The list will be sorted on demand (see checkHandlerListSorted()).
    handlersM.push_back(handler);
    handler->setRepository(this);
    handlerListSortedM = false;
}
//-----------------------------------------------------------------------------
void TemplateCmdHandlerRepository::removeHandler(TemplateCmdHandler* handler)
{
    handlersM.erase(std::find(handlersM.begin(), handlersM.end(), handler));
    handler->setRepository(0);
}
//-----------------------------------------------------------------------------
TemplateCmdHandler::TemplateCmdHandler() :
    repositoryM(0)
{
    getTemplateCmdHandlerRepository().addHandler(this);
}
//-----------------------------------------------------------------------------
void TemplateCmdHandler::setRepository(TemplateCmdHandlerRepository* const repository)
{
    repositoryM = repository;
}
//-----------------------------------------------------------------------------
//! this is currently only called on program termination
TemplateCmdHandler::~TemplateCmdHandler()
{
    if (repositoryM)
        repositoryM->removeHandler(this);
}
//-----------------------------------------------------------------------------
