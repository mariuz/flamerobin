/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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

#include <wx/encconv.h>
#include <wx/fontmap.h>
#include <sstream>
#include <iomanip>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "frutils.h"
#include "gui/ProgressDialog.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/server.h"
//-----------------------------------------------------------------------------
//! converts wxString to std::string
std::string wx2std(const wxString& input, wxMBConv* conv)
{
    if (input.empty())
        return "";
    if (!conv)
        conv = wxConvCurrent;
    const wxWX2MBbuf buf(input.mb_str(*conv));
    // conversion may fail and return 0, which isn't a safe value to pass
    // to std:string constructor
    if (!buf)
        return "";
    return std::string(buf);
}
//-----------------------------------------------------------------------------
//! converts std:string to wxString
wxString std2wx(const std::string& input, wxMBConv* conv)
{
    if (input.empty())
        return wxEmptyString;
    if (!conv)
        conv = wxConvCurrent;
    return wxString(input.c_str(), *conv);
}
//-----------------------------------------------------------------------------
wxString std2wxIdentifier(const std::string& input, wxMBConv* conv)
{
    if (input.empty())
        return wxEmptyString;
    if (!conv)
        conv = wxConvCurrent;
    // trim trailing whitespace
    size_t last = input.find_last_not_of(" ");
    return wxString(input.c_str(), *conv,
        (last == std::string::npos) ? std::string::npos : last + 1);
}
//-----------------------------------------------------------------------------
//! returns string suitable for HTML META charset tag (used only if no
//  conversion to UTF-8 is available, i.e. in non-Unicode build
wxString getHtmlCharset()
{
#if !wxUSE_UNICODE
    struct CharsetMapping {
        wxFontEncoding encoding;
        const wxChar* htmlCS;
    };
    static const CharsetMapping mappings[] = {
        { wxFONTENCODING_ISO8859_1, wxT("ISO-8859-1") },
        { wxFONTENCODING_ISO8859_2, wxT("ISO-8859-2") },
        { wxFONTENCODING_ISO8859_3, wxT("ISO-8859-3") },
        { wxFONTENCODING_ISO8859_4, wxT("ISO-8859-4") },
        { wxFONTENCODING_ISO8859_5, wxT("ISO-8859-5") },
        { wxFONTENCODING_ISO8859_6, wxT("ISO-8859-6") },
        { wxFONTENCODING_ISO8859_7, wxT("ISO-8859-7") },
        { wxFONTENCODING_ISO8859_8, wxT("ISO-8859-8") },
        { wxFONTENCODING_ISO8859_9, wxT("ISO-8859-9") },
        { wxFONTENCODING_ISO8859_10, wxT("ISO-8859-10") },
        { wxFONTENCODING_ISO8859_11, wxT("ISO-8859-11") },
        { wxFONTENCODING_ISO8859_12, wxT("ISO-8859-12") },
        { wxFONTENCODING_ISO8859_13, wxT("ISO-8859-13") },
        { wxFONTENCODING_ISO8859_14, wxT("ISO-8859-14") },
        { wxFONTENCODING_ISO8859_15, wxT("ISO-8859-15") },

        { wxFONTENCODING_CP1250, wxT("windows-1250") },
        { wxFONTENCODING_CP1251, wxT("windows-1251") },
        { wxFONTENCODING_CP1252, wxT("windows-1252") },
        { wxFONTENCODING_CP1253, wxT("windows-1253") },
        { wxFONTENCODING_CP1254, wxT("windows-1254") },
        { wxFONTENCODING_CP1255, wxT("windows-1255") },
        { wxFONTENCODING_CP1256, wxT("windows-1256") },
        { wxFONTENCODING_CP1257, wxT("windows-1257") }
    };
    int mappingCount = sizeof(mappings) / sizeof(CharsetMapping);

    wxFontEncoding enc = wxLocale::GetSystemEncoding();
    for (int i = 0; i < mappingCount; i++)
    {
        if (mappings[i].encoding == enc)
            return mappings[i].htmlCS;
    }
#endif
    return wxT("UTF-8");
}
//-----------------------------------------------------------------------------
TemplateEngine::TemplateEngine(MetadataItem *m,
    std::vector<MetadataItem *> *allowedObjects)
    :objectM(m), flagNextM(false), plainTextM(false)
{
    if (allowedObjects) // optional vector of objects that should be processed
    {
        allowedObjectsM = *allowedObjects;
    }
}
//-----------------------------------------------------------------------------
void TemplateEngine::setPlainText(bool plain)
{
    plainTextM = plain;
}
//-----------------------------------------------------------------------------
//! processes commands found in HTML template
//
//! command is in format:   {%action:data%}
//! data field can be empty
void TemplateEngine::processCommand(wxString cmd, MetadataItem *object,
    wxString& htmlpage, wxWindow *window, bool first)
{
    wxString::size_type pos = cmd.find(':');
    wxString suffix;
    if (pos != wxString::npos)
    {
        suffix = cmd.substr(pos+1);
        cmd = cmd.substr(0, pos);
    }

    // TODO: change this to getURLencodedName()
    if (cmd == wxT("object_name"))
        htmlpage += object->getName_();

    else if (cmd == wxT("parent"))
    {
        if (object->getParent())
            processHtmlCode(htmlpage, suffix, object->getParent(), window);
    }

    else if (cmd == wxT("object_type"))
        htmlpage += object->getTypeName();

    else if (cmd == wxT("object_address"))
        htmlpage += wxString::Format(wxT("%ld"), (uintptr_t)object);

    else if (cmd == wxT("parent_window"))
        htmlpage += wxString::Format(wxT("%ld"), (uintptr_t)window);

    else if (cmd == wxT("fr_home"))
        htmlpage += config().getHomePath();

    else if (cmd == wxT("separator"))
    {
        if (!first)
            htmlpage += suffix;
    }

    else if (cmd == wxT("NotSystem"))
    {
        if (!object->isSystem())
            processHtmlCode(htmlpage, suffix, object, window);
    }

    else if (cmd == wxT("ParentNotSystem"))
    {
        if (object->getParent() && !object->getParent()->isSystem())
            processHtmlCode(htmlpage, suffix, object, window);
    }

    else if (cmd == wxT("header"))  // include another file
    {
        std::vector<wxString> pages;            // pages this object has
        pages.push_back(wxT("Summary"));
        if (object->getType() == ntRole)        // special case, roles
            pages.push_back(wxT("Privileges")); // don't have dependencies
        if (object->getType() == ntDatabase)
            pages.push_back(wxT("Triggers"));   // FB 2.1
        switch (object->getType())
        {
            case ntSysTable:
            case ntTable:
                pages.push_back(wxT("Constraints"));
                pages.push_back(wxT("Indices"));
            case ntView:
                pages.push_back(wxT("Triggers"));
            case ntProcedure:
                pages.push_back(wxT("Privileges"));
            case ntTrigger:
            case ntException:
            case ntFunction:
            case ntGenerator:
                pages.push_back(wxT("Dependencies"));
            case ntDatabase:
            case ntRole:
                pages.push_back(wxT("DDL"));
        };
        wxString page = loadEntireFile(config().getHtmlTemplatesPath()
            + wxT("header.html"));
        bool first = true;
        while (!page.Strip().IsEmpty())
        {
            wxString::size_type p2 = page.find('|');
            wxString part(page);
            if (p2 != wxString::npos)
            {
                part = page.substr(0, p2);
                page.Remove(0, p2+1);
            }
            else
                page.Clear();
            for (std::vector<wxString>::iterator it = pages.begin(); it !=
                pages.end(); ++it)
            {
                if (part.Find(wxT(">")+(*it)+wxT("<")) == -1)
                    continue;
                if (first)
                    first = false;
                else
                    htmlpage += wxT(" | ");
                if (part.Find(wxT(">")+suffix+wxT("<")) != -1)
                    htmlpage += suffix;
                else
                    processHtmlCode(htmlpage, part, object, window);
            }
        }
    }

    else if (cmd == wxT("users"))
    {
        Server* s = dynamic_cast<Server*>(object);
        if (!s)
            return;

        ProgressDialog pd(window->GetParent(), _("Connecting to Server..."), 1);
        UserList* usr = s->getUsers(&pd);
        if (!usr || !usr->size())
        {
            window->Close();
            return;
        }
        for (UserList::iterator it = usr->begin(); it != usr->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it), window);
    }

    else if (cmd == wxT("userinfo"))
    {
        User* u = dynamic_cast<User*>(object);
        if (!u)
            return;
        if (suffix == wxT("username"))
            htmlpage += escapeHtmlChars(u->usernameM);
        if (suffix == wxT("first_name"))
            htmlpage += escapeHtmlChars(u->firstnameM);
        if (suffix == wxT("middle_name"))
            htmlpage += escapeHtmlChars(u->middlenameM);
        if (suffix == wxT("last_name"))
            htmlpage += escapeHtmlChars(u->lastnameM);
        if (suffix == wxT("unix_user"))
            htmlpage << u->useridM;
        if (suffix == wxT("unix_group"))
            htmlpage << u->groupidM;
    }

    else if (cmd == wxT("owner_name"))
    {
        wxString name;
        if (Relation* r = dynamic_cast<Relation*>(object))
            name = r->getOwner();
        else if (Procedure* p = dynamic_cast<Procedure*>(object))
            name = p->getOwner();
        else if (Role* r = dynamic_cast<Role*>(object))
            name = r->getOwner();
        if (!name.empty())
            htmlpage += escapeHtmlChars(name, false);
    }

    else if (cmd == wxT("object_description"))
    {
        wxString desc;
        if (object->getDescription(desc))
        {
            if (desc.empty())
                desc = _("No description");
            htmlpage += escapeHtmlChars(desc);
            if (!suffix.IsEmpty())
                processHtmlCode(htmlpage, suffix, object, window);
        }
    }

    else if (cmd == wxT("show_if_config"))
    {
        wxString::size_type poscolon = suffix.find(':');
        if (poscolon != wxString::npos)
        {
            wxString configKey = suffix.substr(0, poscolon);
            suffix = suffix.substr(poscolon+1);
            wxString::size_type pos2 = suffix.find(':');
            if (pos2 != wxString::npos)
            {
                wxString flag = suffix.substr(0, pos2);
                if (config().get(configKey, (flag == wxT("true"))))
                    processHtmlCode(htmlpage, suffix.substr(pos2+1), object, window);
            }
        }
    }

    else if (cmd == wxT("columns"))  // table and view columns
    {
        Relation* r = dynamic_cast<Relation*>(object);
        if (!r)
            return;
        r->ensureChildrenLoaded();
        for (RelationColumns::iterator it = r->begin(); it != r->end(); ++it)
            processHtmlCode(htmlpage, suffix, (*it).get(), window, it == r->begin());
    }

    // table triggers,  triggers:after or triggers:befor  <- not a typo
    else if (cmd == wxT("triggers"))
    {
        Relation* r = dynamic_cast<Relation*>(object);
        Database* d = dynamic_cast<Database*>(object);
        if (!r && !d)
            return;
        std::vector<Trigger*> tmp;
        if (r)
        {
            if (suffix.substr(0, 5) == wxT("after"))
                r->getTriggers(tmp, Trigger::afterTrigger);
            else
                r->getTriggers(tmp, Trigger::beforeTrigger);
            suffix.erase(0, 5);
        }
        else // d
            d->getDatabaseTriggers(tmp);
        for (std::vector<Trigger*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
            processHtmlCode(htmlpage, suffix, *it, window, it == tmp.begin());
    }

    else if (cmd == wxT("depends_on") || cmd == wxT("depend_of"))
    {
        MetadataItem* m = dynamic_cast<MetadataItem*>(object);
        if (!m)
            return;
        std::vector<Dependency> tmp;
        m->getDependencies(tmp, cmd == wxT("depends_on"));
        for (std::vector<Dependency>::iterator it = tmp.begin();
            it != tmp.end(); ++it)
        {
            processHtmlCode(htmlpage, suffix, &(*it), window);
        }
    }

    else if (cmd == wxT("dependency_columns"))
    {
        Dependency* d = dynamic_cast<Dependency*>(object);
        if (!d)
            return;
        htmlpage += d->getFields();
    }

    else if (cmd == wxT("primary_key"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        PrimaryKeyConstraint* pk = t->getPrimaryKey();
        if (!pk)
            return;
        processHtmlCode(htmlpage, suffix, pk, window);
    }

    else if (cmd == wxT("foreign_keys"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<ForeignKey>* fk = t->getForeignKeys();
        if (!fk)
            return;
        for (std::vector<ForeignKey>::iterator it = fk->begin(); it != fk->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it), window, it == fk->begin());
    }

    else if (cmd == wxT("check_constraints"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<CheckConstraint>* c = t->getCheckConstraints();
        if (!c)
            return;
        for (std::vector<CheckConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it), window, it == c->begin());
    }

    else if (cmd == wxT("unique_constraints"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<UniqueConstraint>* c = t->getUniqueConstraints();
        if (!c)
            return;
        for (std::vector<UniqueConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it), window, it == c->begin());
    }

    else if (cmd == wxT("privileges"))
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
        for (std::vector<Privilege>::iterator it = p->begin();
            it != p->end(); ++it)
        {
            processHtmlCode(htmlpage, suffix, &(*it), window, it == p->begin());
        }
    }

    else if (cmd == wxT("grantee_name"))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
        htmlpage += escapeHtmlChars(p->getGrantee());
    }

    else if (cmd == wxT("privilege"))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
        wxString okimage = wxT("<img src=\"") +
            config().getHtmlTemplatesPath() + wxT("ok.png\">");
        wxString ok2image = wxT("<img src=\"") +
            config().getHtmlTemplatesPath() + wxT("ok2.png\">");
        // see which type
        std::vector<PrivilegeItem> list;
        p->getPrivileges(suffix, list);
        if (list.size())
        {
            bool brnext = false;
            for (std::vector<PrivilegeItem>::iterator it = list.begin(); it !=
                list.end(); ++it)
            {
                if (brnext)
                {
                    htmlpage += wxT("<br>");
                    brnext = false;
                }

                // wxHTML doesn't support TITLE or ALT property of IMG tag so
                // we use our custom 'link hower' handler to show it
                htmlpage += wxT("<a href=\"info://Granted by ") + (*it).grantor
                    + wxT("\">");

                htmlpage += wxT("<img src=\"") +
                    config().getHtmlTemplatesPath();
                if ((*it).grantOption)
                    htmlpage += wxT("ok2.png\"");
                else
                    htmlpage += wxT("ok.png\"");
                htmlpage += wxT("\"></a>");

                if ((*it).columns.size())
                {
                    htmlpage += wxT(" <font size=-1>");
                    for (std::vector<wxString>::iterator i =
                        (*it).columns.begin(); i != (*it).columns.end(); ++i)
                    {
                        if (i != (*it).columns.begin())
                            htmlpage += wxT(",");
                        htmlpage += (*i);
                    }
                    htmlpage += wxT("</font>");
                    brnext = true;
                }
            }
        }
        else
        {
            htmlpage += wxT("<img src=\"") + config().getHtmlTemplatesPath()
                + wxT("redx.png\">");
        }
    }

    else if (cmd == wxT("check_source"))
    {
        CheckConstraint* c = dynamic_cast<CheckConstraint*>(object);
        if (!c)
            return;
        htmlpage += escapeHtmlChars(c->sourceM);
    }

    else if (cmd == wxT("constraint_columns"))
    {
        ColumnConstraint* c = dynamic_cast<ColumnConstraint*>(object);
        if (!c)
            return;
        pos = suffix.find(':');
        if (pos == wxString::npos)
            htmlpage += c->getColumnList();
        else
            htmlpage += c->getColumnList(suffix.substr(0, pos), suffix.substr(pos + 1));
    }

    else if (cmd == wxT("fk_referenced_columns") || cmd == wxT("fk_table"))
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;
        if (cmd == wxT("fk_table"))
            htmlpage += fk->referencedTableM;
        else
            htmlpage += fk->getReferencedColumnList();
    }

    else if (cmd == wxT("fk_update") || cmd == wxT("fk_delete"))  // table and view columns
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;
        if (cmd == wxT("fk_update"))
            htmlpage += fk->updateActionM;
        else
            htmlpage += fk->deleteActionM;
    }

    else if (cmd == wxT("column_datatype"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
        {   // needs newlines escaped for computed column source
            htmlpage += escapeHtmlChars(c->getDatatype());
            // TODO: make the domain name (if any) a link to the domain's property page?
        }
    }

    else if (cmd == wxT("column_nulloption"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
            htmlpage += (c->isNullable() ? wxT("") : wxT("not null"));
    }

    else if (cmd == wxT("column_default"))
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
            htmlpage += def;
        }
    }

    else if (cmd == wxT("input_parameters") || cmd == wxT("output_parameters"))     // SP params
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;

        SubjectLocker locker(p);
        p->ensureChildrenLoaded();
        bool parOut = (cmd == wxT("output_parameters"));
        for (ProcedureParameters::iterator it = p->begin();
            it != p->end(); ++it)
        {
            if ((*it)->isOutputParameter() == parOut)
                processHtmlCode(htmlpage, suffix, (*it).get(), window, it == p->begin());
        }
    }

    else if (cmd == wxT("view_source"))
    {
        View* v = dynamic_cast<View*>(object);
        if (!v)
            return;
        htmlpage += escapeHtmlChars(v->getSource(), false);
    }

    else if (cmd == wxT("procedure_source"))
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;
        htmlpage += escapeHtmlChars(p->getSource(), false);
    }

    else if (cmd == wxT("trigger_source"))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        if (!t)
            return;
        htmlpage += escapeHtmlChars(t->getSource(), false);
    }

    else if (cmd == wxT("trigger_info"))
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
        htmlpage += escapeHtmlChars(s, false);
    }

    else if (cmd == wxT("generator_value"))
    {
        Generator* g = dynamic_cast<Generator*>(object);
        if (!g)
            return;
        std::ostringstream ss;
        ss << g->getValue();
        htmlpage += escapeHtmlChars(std2wx(ss.str()), false);
    }

    else if (cmd == wxT("exception_number"))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;
        wxString s;
        s << e->getNumber();
        htmlpage += escapeHtmlChars(s, false);
    }

    else if (cmd == wxT("exception_message"))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;
        htmlpage += escapeHtmlChars(e->getMessage(), false);
    }

    else if (cmd == wxT("udf_info"))
    {
        Function* f = dynamic_cast<Function*>(object);
        if (!f)
            return;
        wxString src = f->getDefinition();
        htmlpage += f->getHtmlHeader() + escapeHtmlChars(src, false);
    }

    else if (cmd == wxT("varcolor"))
    {
        static bool first = false;
        first = !first;
        pos = suffix.find('/');
        if (first)
            htmlpage += suffix.substr(0, pos);
        else
            htmlpage += suffix.substr(pos + 1);
    }

    else if (cmd == wxT("indices"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<Index>* ix = t->getIndices();
        if (!ix)
            return;
        for (std::vector<Index>::iterator it = ix->begin(); it != ix->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it), window, it == ix->begin());
    }

    else if (cmd == wxT("object_ddl"))
    {
        ProgressDialog pd(window->GetParent(), _("Extracting DDL Definitions"), 2);

        CreateDDLVisitor cdv(&pd);
        object->acceptVisitor(&cdv);
        htmlpage += escapeHtmlChars(cdv.getSql(), false);
    }

    else if (cmd.substr(0, 5) == wxT("index"))
    {
        wxString okimage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("ok.png\">");
        wxString ximage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("redx.png\">");
        Index* i = dynamic_cast<Index*>(object);
        if (!i)
            return;
        if (cmd == wxT("index_type"))
            htmlpage += (i->getIndexType() == Index::itAscending ? wxT("ASC") : wxT("DESC"));
        if (cmd == wxT("index_active"))
            htmlpage += (i->isActive() ? okimage : ximage);
        if (cmd == wxT("index_unique") && i->isUnique())
            htmlpage += okimage;
        else if (cmd == wxT("index_stats"))
        {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << i->getStatistics();
            htmlpage += std2wx(ss.str());
        }
        else if (cmd == wxT("index_fields"))
            htmlpage += i->getFieldsAsString();
    }

    else if (cmd == wxT("creation_date"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += d->getInfo().getCreated();
    }
    else if (cmd == wxT("default_charset"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += d->getDatabaseCharset();
    }
    else if (cmd == wxT("dialect"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getDialect();
    }
    else if (cmd == wxT("filesize"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        int64_t size = d->getInfo().getSizeInBytes();
        const double kilo = 1024;
        const double mega = kilo * kilo;
        const double giga = kilo * mega;
        if (size >= giga)
            htmlpage += wxString::Format(wxT("%0.2fGB"), size / giga);
        else if (size >= mega)
            htmlpage += wxString::Format(wxT("%0.2fMB"), size / mega);
        else
            htmlpage += wxString::Format(wxT("%0.2fkB"), size / kilo);
    }
    else if (cmd == wxT("forced_writes"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        wxString okimage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("ok.png\">");
        wxString ximage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("redx.png\">");

        htmlpage += (d->getInfo().getForcedWrites() ? okimage : ximage);
    }
    else if (cmd == wxT("fullpath"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += d->getConnectionString();
    }
    else if (cmd == wxT("next_transaction"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getNextTransaction();
    }
    else if (cmd == wxT("ods_version"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getODS();
        if (d->getInfo().getODSMinor())
        {
            htmlpage += wxT(".");
            htmlpage += wxString() << d->getInfo().getODSMinor();
        }
    }
    else if (cmd == wxT("oldest_transaction"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getOldestTransaction();
    }
    else if (cmd == wxT("page_buffers"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getBuffers();
    }
    else if (cmd == wxT("page_size"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getPageSize();
    }
    else if (cmd == wxT("pages"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getPages();
    }
    else if (cmd == wxT("read_only"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        if (d->getInfo().getReadOnly())
            htmlpage += wxT("true");
    else
            htmlpage += wxT("false");
    }
    else if (cmd == wxT("sweep_interval"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getSweep();
    }
}
//-----------------------------------------------------------------------------
//! processes html template code given in the htmlsource wxString
void TemplateEngine::processHtmlCode(wxString& htmlpage, wxString htmlsource,
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

    using namespace std;
    wxString::size_type pos = 0, oldpos = 0, endpos = 0;
    while (true)
    {
        pos = htmlsource.find(wxT("{%"), pos);
        if (pos == wxString::npos)
        {
            htmlpage += htmlsource.substr(oldpos);
            break;
        }

        wxString::size_type check, startpos = pos;
        int cnt = 1;
        while (cnt > 0)
        {
            endpos = htmlsource.find(wxT("%}"), startpos+1);
            if (endpos == wxString::npos)
                break;

            check = htmlsource.find(wxT("{%"), startpos+1);
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

        htmlpage += htmlsource.substr(oldpos, pos - oldpos);
        wxString cmd = htmlsource.substr(pos + 2, endpos - pos - 2); // 2 = start_marker_len = end_marker_len
        processCommand(cmd, object, htmlpage, window, first);
        oldpos = pos = endpos + 2;
    }
}
//-----------------------------------------------------------------------------
//! converts chars that have special meaning in HTML, so they get displayed
wxString TemplateEngine::escapeHtmlChars(const wxString& input, bool processNewlines)
{
    if (input.empty() || plainTextM)
        return input;
    wxString result;
    wxString::const_iterator start = input.begin();
    while (start != input.end())
    {
        wxString::const_iterator stop = start;
        while (stop != input.end())
        {
            const wxChar c = *stop;
            if (c == '&' || c == '<' || c == '>' || c == '"'
                || (processNewlines && (c == '\r' || c == '\n')))
            {
                if (stop > start)
                    result += wxString(start, stop);
                if (c == '&')
                    result += wxT("&amp;");
                else if (c == '<')
                    result += wxT("&lt;");
                else if (c == '>')
                    result += wxT("&gt;");
                else if (c == '"')
                    result += wxT("&quot;");
                else if (c == '\n')
                    result += wxT("<BR>");
                else if (c == '\r')
                    /* swallow silently */;
                else
                    wxASSERT_MSG(false, wxT("escape not handled"));
                // start processing *after* the replaced character
                ++stop;
                start = stop;
                break;
            }
            ++stop;
        }
        if (stop > start)
            result += wxString(start, stop);
        start = stop;
    }
    return result;
}
//-----------------------------------------------------------------------------
