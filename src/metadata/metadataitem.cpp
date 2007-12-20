/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <sstream>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/root.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
MetadataItem::MetadataItem()
    : Subject()
{
    parentM = 0;
    typeM = ntUnknown;
    descriptionLoadedM = dsNotLoaded;
}
//-----------------------------------------------------------------------------
MetadataItem::~MetadataItem()
{
}
//-----------------------------------------------------------------------------
const wxString MetadataItem::getTypeName() const
{
    return wxT("");
}
//-----------------------------------------------------------------------------
const wxString MetadataItem::getItemPath() const
{
    wxString result = getTypeName() + wxT("_") + getPathId();
    if (parentM)
    {
        wxString parentItemPath = parentM->getItemPath();
        if (!parentItemPath.empty())
            result = parentItemPath + wxT("/") + result;
    }
    return result;
}
//-----------------------------------------------------------------------------
const wxString MetadataItem::getPathId() const
{
    return getId();
}
//-----------------------------------------------------------------------------
const wxString MetadataItem::getId() const
{
    return getName_();
}
//-----------------------------------------------------------------------------
wxString getNameOfType(NodeType type)
{
    switch (type)
    {
        case ntTable:       return (wxT("TABLE"));
        case ntView:        return (wxT("VIEW"));
        case ntProcedure:   return (wxT("PROCEDURE"));
        case ntTrigger:     return (wxT("TRIGGER"));
        case ntGenerator:   return (wxT("GENERATOR"));
        case ntFunction:    return (wxT("FUNCTION"));
        case ntDomain:      return (wxT("DOMAIN"));
        case ntRole:        return (wxT("ROLE"));
        case ntColumn:      return (wxT("COLUMN"));
        case ntException:   return (wxT("EXCEPTION"));
        default:
            return wxT("");
    }
}
//-----------------------------------------------------------------------------
NodeType getTypeByName(wxString name)
{
    if (name == wxT("TABLE"))
        return ntTable;
    else if (name == wxT("VIEW"))
        return ntView;
    else if (name == wxT("PROCEDURE"))
        return ntProcedure;
    else if (name == wxT("TRIGGER"))
        return ntTrigger;
    else if (name == wxT("GENERATOR"))
        return ntGenerator;
    else if (name == wxT("FUNCTION"))
        return ntFunction;
    else if (name == wxT("DOMAIN"))
        return ntDomain;
    else if (name == wxT("ROLE"))
        return ntRole;
    else if (name == wxT("COLUMN"))
        return ntColumn;
    else if (name == wxT("EXCEPTION"))
        return ntException;
    else
        return ntUnknown;
}
//-----------------------------------------------------------------------------
bool MetadataItem::getChildren(vector<MetadataItem*>& /*temp*/)
{
    return false;
}
//-----------------------------------------------------------------------------
//! removes its children (by calling drop() for each) and notifies its parent
void MetadataItem::drop()
{
    vector<MetadataItem* >temp;
    if (getChildren(temp))
        for (vector<MetadataItem*>::iterator it = temp.begin(); it != temp.end(); ++it)
            (*it)->drop();

    // TODO: perhaps the whole DBH needs to be reconsidered
    // we could write: if (parentM) parentM->remove(this);
    // but we can't, since parent might not be a collection!
    // ie. currently it is a Database object
}
//-----------------------------------------------------------------------------
MetadataItem* MetadataItem::getParentObjectOfType(NodeType type) const
{
    MetadataItem* m = const_cast<MetadataItem*>(this);
    while (m && m->getType() != type)
        m = m->getParent();
    return m;
}
//-----------------------------------------------------------------------------
Database* MetadataItem::findDatabase() const
{
    return dynamic_cast<Database*>(getParentObjectOfType(ntDatabase));
}
//-----------------------------------------------------------------------------
Database* MetadataItem::getDatabase(const wxString& callingMethod) const
{
    Database* database = findDatabase();
    if (!database)
    {
        throw FRError(wxString::Format(_("%s - no database assigned"),
            callingMethod.c_str()));
    }
    return database;
}
//-----------------------------------------------------------------------------
Root* MetadataItem::getRoot() const
{
    return dynamic_cast<Root*>(getParentObjectOfType(ntRoot));
}
//-----------------------------------------------------------------------------
//! ofObject = true   => returns list of objects this object depends on
//! ofObject = false  => returns list of objects that depend on this object
void MetadataItem::getDependencies(vector<Dependency>& list, bool ofObject)
{
    Database* d = getDatabase(wxT("MetadataItem::getDependencies"));

    int mytype = -1;            // map DBH type to RDB$DEPENDENT TYPE
    NodeType dep_types[] = {    ntTable,    ntView,     ntTrigger,  ntUnknown,  ntUnknown,
                                ntProcedure,ntUnknown,  ntException,ntUnknown,  ntUnknown,
                                ntUnknown,  ntUnknown,  ntUnknown,  ntUnknown,  ntGenerator,
                                ntFunction
    };
    int type_count = sizeof(dep_types)/sizeof(NodeType);
    for (int i = 0; i < type_count; i++)
        if (typeM == dep_types[i])
            mytype = i;

    // views count as relations(tables) when other object refer to them
    if (mytype == 1 && !ofObject)
        mytype = 0;
    // system tables should be treated as tables
    if (typeM == ntSysTable)
        mytype = 0;

    if (typeM == ntUnknown || mytype == -1)
        throw FRError(_("Unsupported type"));
    IBPP::Database& db = d->getIBPPDatabase();
    IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);

    wxString o1 = (ofObject ? wxT("DEPENDENT") : wxT("DEPENDED_ON"));
    wxString o2 = (ofObject ? wxT("DEPENDED_ON") : wxT("DEPENDENT"));
    wxString sql =
        wxT("select RDB$") + o2 + wxT("_TYPE, RDB$") + o2 + wxT("_NAME, RDB$FIELD_NAME \n ")
        wxT(" from RDB$DEPENDENCIES \n ")
        wxT(" where RDB$") + o1 + wxT("_TYPE = ? and RDB$") + o1 + wxT("_NAME = ? \n ");
    int params = 1;
    if ((typeM == ntTable || typeM == ntSysTable || typeM == ntView) && ofObject)  // get deps for computed columns
    {                                                       // view needed to bind with generators
        sql += wxT(" union  \n")
            wxT(" SELECT DISTINCT d.rdb$depended_on_type, d.rdb$depended_on_name, d.rdb$field_name \n")
            wxT(" FROM rdb$relation_fields f \n")
            wxT(" LEFT JOIN rdb$dependencies d ON d.rdb$dependent_name = f.rdb$field_source \n")
            wxT(" WHERE d.rdb$dependent_type = 3 AND f.rdb$relation_name = ? \n");
        params++;
    }
    if (!ofObject) // find tables that have calculated columns based on "this" object
    {
        sql += wxT("union  \n")
            wxT(" SELECT distinct cast(0 as smallint), f.rdb$relation_name, f.rdb$field_name \n")
            wxT(" from rdb$relation_fields f \n")
            wxT(" left join rdb$dependencies d on d.rdb$dependent_name = f.rdb$field_source \n")
            wxT(" where d.rdb$dependent_type = 3 and d.rdb$depended_on_name = ? ");
        params++;
    }
    // get the exact table and fields for views
    // rdb$dependencies covers deps. for WHERE clauses in SELECTs in VIEW body
    // but we also need mapping for column list in SELECT. These 2 queries cover it:
    if (ofObject && typeM == ntView)
    {
		sql += wxT(" union \n")
			wxT(" select distinct cast(0 as smallint), vr.RDB$RELATION_NAME, f.RDB$BASE_FIELD \n")
			wxT(" from RDB$RELATION_FIELDS f \n")
			wxT(" join RDB$VIEW_RELATIONS vr on f.RDB$VIEW_CONTEXT = vr.RDB$VIEW_CONTEXT \n")
			wxT("   and f.RDB$RELATION_NAME = vr.RDB$VIEW_NAME \n")
			wxT(" where f.rdb$relation_name = ? \n");
		params++;
    }
    // views can depend on other views as well
    // we might need to add procedures here one day when Firebird gains support for it
    if (!ofObject && (typeM == ntView || typeM == ntTable || typeM == ntSysTable))
    {
		sql += wxT(" union \n")
			wxT(" select distinct cast(0 as smallint), f.RDB$RELATION_NAME, f.RDB$BASE_FIELD \n")
			wxT(" from RDB$RELATION_FIELDS f \n")
			wxT(" join RDB$VIEW_RELATIONS vr on f.RDB$VIEW_CONTEXT = vr.RDB$VIEW_CONTEXT \n")
			wxT("   and f.RDB$RELATION_NAME = vr.RDB$VIEW_NAME \n")
			wxT(" where vr.rdb$relation_name = ? \n");
		params++;
    }

    sql += wxT(" order by 1, 2, 3");
    st1->Prepare(wx2std(sql));
    st1->Set(1, mytype);
    for (int i = 0; i < params; i++)
		st1->Set(2+i, wx2std(getName_()));
    st1->Execute();
    MetadataItem* last = 0;
    Dependency* dep = 0;
    while (st1->Fetch())
    {
        std::string object_name, field_name;
        int object_type;
        st1->Get(1, &object_type);
        st1->Get(2, object_name);
        object_name.erase(object_name.find_last_not_of(" ") + 1);     // trim

        if (object_type > type_count)   // some system object, not interesting for us
            continue;
        NodeType t = dep_types[object_type];
        if (t == ntUnknown)             // ditto
            continue;
        MetadataItem* current = d->findByNameAndType(t, std2wx(object_name));
        if (!current)
        {
            if (t == ntTable) {
                // maybe it's a view masked as table
                current = d->findByNameAndType(ntView, std2wx(object_name));
                // or possibly a system table
                if (!current)
                    current = d->findByNameAndType(ntSysTable, std2wx(object_name));
            }
            if (!ofObject && t == ntTrigger)
            {
                // system trigger dependent of this object indicates possible check constraint on a table
                // that references this object. So, let's check if this trigger is used for check constraint
                // and get that table's name
                IBPP::Statement st2 = IBPP::StatementFactory(db, tr1);
                st2->Prepare(
                    "select r.rdb$relation_name from rdb$relation_constraints r "
                    " join rdb$check_constraints c on r.rdb$constraint_name=c.rdb$constraint_name "
                    " and r.rdb$constraint_type = 'CHECK' where c.rdb$trigger_name = ? "
                );
                st2->Set(1, object_name);
                st2->Execute();
                if (st2->Fetch()) // table using that trigger found
                {
                    std::string tablecheck;
                    st2->Get(1, tablecheck);
                    tablecheck.erase(tablecheck.find_last_not_of(" ")+1);
                    if (getName_() != std2wx(tablecheck))    // avoid self-reference
                        current = d->findByNameAndType(ntTable, std2wx(tablecheck));
                }
            }
            if (!current)
                continue;
        }
        if (current != last)            // new object
        {
            Dependency de(current);
            list.push_back(de);
            dep = &list.back();
            last = current;
        }
        if (!st1->IsNull(3))
        {
            st1->Get(3, field_name);
            field_name.erase(field_name.find_last_not_of(" ") + 1);       // trim
            dep->addField(std2wx(field_name));
        }
    }

    // TODO: perhaps this could be moved to Table?
    //       call MetadataItem::getDependencies() and then add this
    if ((typeM == ntTable || typeM == ntSysTable) && ofObject)   // foreign keys of this table + computed columns
    {
        Table *t = dynamic_cast<Table *>(this);
        vector<ForeignKey> *f = t->getForeignKeys();
        for (vector<ForeignKey>::const_iterator it = f->begin(); it != f->end(); ++it)
        {
            MetadataItem *table = d->findByNameAndType(ntTable,
                (*it).referencedTableM);
            if (!table)
            {
                throw FRError(wxString::Format(_("Table %s not found."),
                    (*it).referencedTableM.c_str()));
            }
            Dependency de(table);
            de.setFields((*it).referencedColumnsM);
            list.push_back(de);
        }

        // Add check constraints here (CHECKS are checked via system triggers), example:
        // table1::check( table1.field1 > select max(field2) from table2 )
        // So, table vs any object from this ^^^ select
        // Algorithm: 1.find all system triggers bound to that CHECK constraint
        //            2.find dependencies for those system triggers
        //            3.display those dependencies as deps. of this table
        st1->Prepare("select distinct c.rdb$trigger_name from rdb$relation_constraints r "
            " join rdb$check_constraints c on r.rdb$constraint_name=c.rdb$constraint_name "
            " and r.rdb$constraint_type = 'CHECK' where r.rdb$relation_name= ? "
        );
        st1->Set(1, wx2std(getName_()));
        st1->Execute();
        vector<Dependency> tempdep;
        while (st1->Fetch())
        {
            std::string s;
            st1->Get(1, s);
            s.erase(s.find_last_not_of(" ")+1);
            Trigger t;
            t.setName_(std2wx(s));
            t.setParent(d);
            t.getDependencies(tempdep, true);
        }
        // remove duplicates, and self-references from "tempdep"
        while (true)
        {
            std::vector<Dependency>::iterator to_remove = tempdep.end();
            for (std::vector<Dependency>::iterator it = tempdep.begin(); it != tempdep.end(); ++it)
            {
                if ((*it).getDependentObject() == this)
                {
                    to_remove = it;
                    break;
                }
                to_remove = std::find(it + 1, tempdep.end(), (*it));
                if (to_remove != tempdep.end())
                    break;
            }
            if (to_remove == tempdep.end())
                break;
            else
                tempdep.erase(to_remove);
        }
        list.insert(list.end(), tempdep.begin(), tempdep.end());
    }

    // TODO: perhaps this could be moved to Table?
    if ((typeM == ntTable || typeM == ntSysTable) && !ofObject)  // foreign keys of other tables
    {
        st1->Prepare(
            "select r1.rdb$relation_name, i.rdb$field_name "
            " from rdb$relation_constraints r1 "
            " join rdb$ref_constraints c on r1.rdb$constraint_name = c.rdb$constraint_name "
            " join rdb$relation_constraints r2 on c.RDB$CONST_NAME_UQ = r2.rdb$constraint_name "
            " join rdb$index_segments i on r1.rdb$index_name=i.rdb$index_name "
            " where r2.rdb$relation_name=? "
            " and r1.rdb$constraint_type='FOREIGN KEY' "
        );
        st1->Set(1, wx2std(getName_()));
        st1->Execute();
        std::string lasttable;
        Dependency *dep = 0;
        while (st1->Fetch())
        {
            std::string table_name, field_name;
            st1->Get(1, table_name);
            st1->Get(2, field_name);
            table_name.erase(table_name.find_last_not_of(" ")+1);       // trim
            field_name.erase(field_name.find_last_not_of(" ")+1);       // trim
            if (table_name != lasttable)    // new
            {
                MetadataItem* table = d->findByNameAndType(ntTable, std2wx(table_name));
                if (!table)
                    continue;           // dummy check
                Dependency de(table);
                list.push_back(de);
                dep = &list.back();
                lasttable = table_name;
            }
            dep->addField(std2wx(field_name));
        }
    }

    tr1->Commit();
}
//-----------------------------------------------------------------------------
bool MetadataItem::isDescriptionAvailable()
{
    if (descriptionLoadedM == dsNotLoaded)
        loadDescription();
    return descriptionLoadedM != dsNotAvailable;
}
//-----------------------------------------------------------------------------
wxString MetadataItem::getDescription(bool force)
{
    if (force || descriptionLoadedM == dsNotLoaded)
        loadDescription();
    return descriptionM;
}
//-----------------------------------------------------------------------------
void MetadataItem::loadDescription()
{
    setDescriptionM(wxEmptyString);
}
//-----------------------------------------------------------------------------
void MetadataItem::loadDescription(wxString loadStatement)
{
    Database* d = getDatabase(wxT("MetadataItem::loadDescription"));

    wxString desc;
    try
    {
        MetadataLoader* loader = d->getMetadataLoader();
        MetadataLoaderTransaction tr(loader);
        IBPP::Statement& st1 = loader->getStatement(wx2std(loadStatement));
        st1->Set(1, wx2std(getName_()));
        if (st1->Parameters() > 1)
            st1->Set(2, wx2std(getParent()->getName_())); // table/view/SP name
        st1->Execute();
        st1->Fetch();

        std::string value;
        if (!st1->IsNull(1))
        {
            IBPP::Blob b = loader->createBlob();
            st1->Get(1, b);
            b->Load(value);
            desc = std2wx(value);
        }
        descriptionLoadedM = dsLoaded;
    }
    catch (IBPP::SQLException &e)
    {
        // FB 2.0 supports descriptions for some objects that previous
        // FB versions don't
        if (e.SqlCode() != -206) // column does not belong to referenced table.
            throw;
        else
            descriptionLoadedM = dsNotAvailable;
    }

    // set value, notify observers
    setDescriptionM(desc);
}
//-----------------------------------------------------------------------------
void MetadataItem::saveDescription(wxString WXUNUSED(description))
{
    throw FRError(wxString::Format(
        wxT("Objects of type %s do not support descriptions"),
        getTypeName().c_str()));
}
//-----------------------------------------------------------------------------
void MetadataItem::saveDescription(wxString saveStatement,
    wxString description)
{
    Database* d = getDatabase(wxT("MetadataItem::saveDescription"));

    IBPP::Database& db = d->getIBPPDatabase();
    IBPP::Transaction tr1 = IBPP::TransactionFactory(db);
    tr1->Start();

    IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
    st1->Prepare(wx2std(saveStatement));

    if (!description.empty())
    {
        IBPP::Blob b = IBPP::BlobFactory(db, tr1);
        b->Save(wx2std(description));
        st1->Set(1, b);
    }
    else
        st1->SetNull(1);
    st1->Set(2, wx2std(getName_()));
    if (st1->Parameters() > 2)
        st1->Set(3, wx2std(getParent()->getName_()));
    st1->Execute();
    tr1->Commit();
    // set value, notify observers
    setDescriptionM(description);
}
//-----------------------------------------------------------------------------
void MetadataItem::setDescription(wxString description)
{
    if (description.compare(getDescription()) != 0)
        saveDescription(description);
}
//-----------------------------------------------------------------------------
void MetadataItem::setDescriptionM(wxString description)
{
    if (descriptionLoadedM == dsNotLoaded || descriptionLoadedM == dsLoaded &&
        (descriptionM.compare(description) != 0))
    {
        descriptionM = description;
        descriptionLoadedM = dsLoaded;
        // FIXME: This is correct, but leads to reentrancy problems with the
        //        current code.  Working on it...
        // notifyObservers();
    }
}
//-----------------------------------------------------------------------------
MetadataItem* MetadataItem::getParent() const
{
    return parentM;
}
//-----------------------------------------------------------------------------
void MetadataItem::setParent(MetadataItem* parent)
{
    parentM = parent;
}
//-----------------------------------------------------------------------------
wxString MetadataItem::getPrintableName()
{
    wxString printableName(getName_());
    size_t n = getChildrenCount();
    if (n)
        printableName << wxT(" (") << n << wxT(")");
    return printableName;
}
//-----------------------------------------------------------------------------
wxString MetadataItem::getName_() const
{
    return identifierM.get();
}
//-----------------------------------------------------------------------------
wxString MetadataItem::getQuotedName() const
{
    return identifierM.getQuoted();
}
//-----------------------------------------------------------------------------
Identifier MetadataItem::getIdentifier() const
{
    return identifierM;
}
//-----------------------------------------------------------------------------
void MetadataItem::setName_(wxString name)
{
    identifierM.setText(name);
    notifyObservers();
}
//-----------------------------------------------------------------------------
NodeType MetadataItem::getType() const
{
    return typeM;
}
//-----------------------------------------------------------------------------
void MetadataItem::setType(NodeType type)
{
    typeM = type;
}
//-----------------------------------------------------------------------------
bool MetadataItem::isSystem() const
{
    return getName_().substr(0, 4) == wxT("RDB$");
}
//-----------------------------------------------------------------------------
wxString MetadataItem::getDropSqlStatement() const
{
    return wxT("DROP ") + getTypeName() + wxT(" ") + getQuotedName() + wxT(";");
}
//-----------------------------------------------------------------------------
void MetadataItem::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitMetadataItem(*this);
}
//-----------------------------------------------------------------------------
void MetadataItem::lockChildren()
{
// NOTE: getChildren() can not be used here, because we want to lock the
//       MetadataCollection objects as well.  That means we have to override
//       this method in all descendant classes - oh well...
}
//-----------------------------------------------------------------------------
void MetadataItem::lockSubject()
{
    Subject::lockSubject();
    lockChildren();
}
//-----------------------------------------------------------------------------
void MetadataItem::unlockChildren()
{
// NOTE: getChildren() can not be used here, because we want to lock the
//       MetadataCollection objects as well.  That means we have to override
//       this method in all descendant classes - oh well...
}
//-----------------------------------------------------------------------------
void MetadataItem::unlockSubject()
{
    Subject::unlockSubject();
    unlockChildren();
}
//-----------------------------------------------------------------------------
MetadataItem *Dependency::getParent() const
{
    return objectM->getParent();
}
//-----------------------------------------------------------------------------
wxString Dependency::getName_() const
{
    return objectM->getName_();
}
//-----------------------------------------------------------------------------
NodeType Dependency::getType() const
{
    return objectM->getType();
}
//-----------------------------------------------------------------------------
const wxString Dependency::getTypeName() const
{
    return objectM->getTypeName();
}
//-----------------------------------------------------------------------------
MetadataItem *Dependency::getDependentObject() const
{
    return objectM;
}
//-----------------------------------------------------------------------------
Dependency::Dependency(MetadataItem *object)
{
    objectM = object;
}
//-----------------------------------------------------------------------------
void Dependency::getFields(std::vector<wxString>& fields) const
{
    for (std::vector<wxString>::const_iterator it = fieldsM.begin();
        it != fieldsM.end(); ++it)
    {
        fields.push_back(*it);
    }
}
//-----------------------------------------------------------------------------
wxString Dependency::getFields() const
{
    wxString temp;
    for (std::vector<wxString>::const_iterator it = fieldsM.begin(); it != fieldsM.end(); ++it)
    {
        if (it != fieldsM.begin())
            temp += wxT(", ");
        temp += (*it);
    }
    return temp;
}
//-----------------------------------------------------------------------------
void Dependency::addField(const wxString& name)
{
    if (fieldsM.end() == std::find(fieldsM.begin(), fieldsM.end(), name))
        fieldsM.push_back(name);
}
//-----------------------------------------------------------------------------
void Dependency::setFields(const std::vector<wxString>& fields)
{
    fieldsM = fields;
}
//-----------------------------------------------------------------------------
bool Dependency::operator== (const Dependency& other) const
{
    return (objectM == other.getDependentObject() && getFields() == other.getFields());
}
//-----------------------------------------------------------------------------
bool Dependency::operator!= (const Dependency& other) const
{
    return (objectM != other.getDependentObject() || getFields() != other.getFields());
}
//-----------------------------------------------------------------------------
void Dependency::acceptVisitor(MetadataItemVisitor* visitor)
{
    if (objectM)
        objectM->acceptVisitor(visitor);
}
//-----------------------------------------------------------------------------
