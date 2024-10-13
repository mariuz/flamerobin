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

#include "config/Config.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/MetadataItemDescriptionVisitor.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/root.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"

void initializeLockCount(MetadataItem* item, unsigned count)
{
    if (item != 0 && count > 0)
    {
        for (unsigned i = 0; i < count; ++i)
            item->lockSubject();
    }
}

void initializeLockCount(MetadataItemPtr item, unsigned count)
{
    initializeLockCount(item.get(), count);
}

template<>
ObjectWithHandle<MetadataItem>::HandleMap ObjectWithHandle<MetadataItem>::handleMap = ObjectWithHandle<MetadataItem>::HandleMap();
template<>
ObjectWithHandle<MetadataItem>::Handle ObjectWithHandle<MetadataItem>::nextHandle = 0;

MetadataItem::MetadataItem()
    : Subject(), typeM(ntUnknown), parentM(0), metadataIdM(-1), childrenLoadedM(lsNotLoaded),
        descriptionLoadedM(lsNotLoaded), propertiesLoadedM(lsNotLoaded)
{
}

MetadataItem::MetadataItem(NodeType type, MetadataItem* parent,
        const wxString& name, int id)
   : Subject(), 
        typeM(type), parentM(parent), identifierM(name, getDatabase() != nullptr ? getDatabase()->getSqlDialect() : 3),
        metadataIdM(id),
        childrenLoadedM(lsNotLoaded), descriptionLoadedM(lsNotLoaded),
        propertiesLoadedM(lsNotLoaded)
{
}

MetadataItem::~MetadataItem()
{
}

const wxString MetadataItem::getTypeName() const
{
    return "";
}

const wxString MetadataItem::getItemPath() const
{
    wxString result = getTypeName() + "_" + getPathId();
    if (MetadataItem* parent = getParent())
    {
        wxString parentItemPath = parent->getItemPath();
        if (!parentItemPath.empty())
            result = parentItemPath + "/" + result;
    }
    return result;
}

const wxString MetadataItem::getPathId() const
{
    return getId();
}

const wxString MetadataItem::getId() const
{
    return getName_();
}

wxString getNameOfType(NodeType type)
{
    switch (type)
    {
        case ntTable:        return ("TABLE");
        case ntGTT:          return ("TABLEGTT");
        case ntView:         return ("VIEW");
        case ntProcedure:    return ("PROCEDURE");
        case ntDMLTrigger:   return ("TRIGGER");
        case ntGenerator:    return ("GENERATOR");
        case ntFunctionSQL:  return ("FUNCTIONSQL");
        case ntUDF:          return ("UDF");
        case ntDomain:       return ("DOMAIN");
        case ntRole:         return ("ROLE");
        case ntColumn:       return ("COLUMN");
        case ntException:    return ("EXCEPTION");
        case ntPackage:      return ("PACKAGE");
        case ntIndex:        return ("INDEX");
        case ntCharacterSet: return ("CHARACTERSET");
        case ntCollation:    return ("COLLATION");
        default:
            return "";
    }
}

NodeType getTypeByName(const wxString& name)
{
    if (name == "TABLE")
        return ntTable;
    else if (name == "TABLEGTT")
        return ntGTT;
    else if (name == "VIEW")
        return ntView;
    else if (name == "PROCEDURE")
        return ntProcedure;
    else if (name == "TRIGGER")
        return ntDMLTrigger;
    else if (name == "GENERATOR")
        return ntGenerator;
    else if (name == "FUNCTIONSQL")
        return ntFunctionSQL;
    else if (name == "FUNCTION")
        return ntFunctionSQL;
    else if (name == "UDF")
        return ntUDF;
    else if (name == "DOMAIN")
        return ntDomain;
    else if (name == "ROLE")
        return ntRole;
    else if (name == "COLUMN")
        return ntColumn;
    else if (name == "EXCEPTION")
        return ntException;
    else if (name == "PACKAGE")
        return ntPackage;
    else if (name == "INDEX")
        return ntIndex;
    else if (name == "CHARACTERSET")
        return ntCharacterSet;
    else if (name == "COLLATION")
        return ntCollation;
    else
        return ntUnknown;
}

void MetadataItem::invalidate()
{
    setChildrenLoaded(false);
    setPropertiesLoaded(false);
    notifyObservers();
}

void MetadataItem::loadPendingData()
{
    if (propertiesLoadedM == lsLoadPending)
        loadProperties();
    if (childrenLoadedM == lsLoadPending)
        loadChildren();
}

void MetadataItem::resetPendingLoadData()
{
    if (propertiesLoadedM == lsLoadPending)
        propertiesLoadedM = lsNotLoaded;
    if (childrenLoadedM == lsLoadPending)
        childrenLoadedM = lsNotLoaded;
}

void MetadataItem::ensurePropertiesLoaded()
{
    if (!propertiesLoaded())
        loadProperties();
}

bool MetadataItem::propertiesLoaded() const
{
    return propertiesLoadedM == lsLoaded;
}

void MetadataItem::loadProperties()
{
}

void MetadataItem::setPropertiesLoaded(bool loaded)
{
    if (loaded)
        propertiesLoadedM = lsLoaded;
    else
    {
        if (propertiesLoadedM == lsLoaded)
            propertiesLoadedM = lsLoadPending;
        else if (propertiesLoadedM != lsLoadPending)
            propertiesLoadedM = lsNotLoaded;
    }
}

bool MetadataItem::childrenLoaded() const
{
    return childrenLoadedM == lsLoaded;
}

void MetadataItem::doSetChildrenLoaded(bool /*loaded*/)
{
}

void MetadataItem::ensureChildrenLoaded()
{
    if (!childrenLoaded())
        loadChildren();
}

void MetadataItem::loadChildren()
{
}

void MetadataItem::setChildrenLoaded(bool loaded)
{
    if (loaded)
        childrenLoadedM = lsLoaded;
    else
    {
        if (childrenLoadedM == lsLoaded)
            childrenLoadedM = lsLoadPending;
        else if (childrenLoadedM != lsLoadPending)
            childrenLoadedM = lsNotLoaded;
    }
    doSetChildrenLoaded(loaded);
}

bool MetadataItem::getChildren(std::vector<MetadataItem*>& /*temp*/)
{
    return false;
}

DatabasePtr MetadataItem::getDatabase() const
{
    if (MetadataItem* m = getParent())
        return m->getDatabase();
    return DatabasePtr();
}

void MetadataItem::getDependencies(std::vector<Dependency>& list,
    bool ofObject, const wxString& field)
{
    std::vector<Dependency> tmp;
    getDependencies(tmp, ofObject);
    for (std::vector<Dependency>::iterator it = tmp.begin();
        it != tmp.end(); ++it)
    {
        if ((*it).hasField(field))
            list.push_back(*it);
    }
}

void MetadataItem::getDependenciesPivoted(std::vector<DependencyField>& list)
{
    std::vector<Dependency> deps;
    this->getDependencies(deps, true, true);
    this->getDependencies(deps, false, true);
    for (std::vector<Dependency>::iterator it = deps.begin(); it != deps.end(); ++it)
    {
        std::vector<DependencyField> fields;
        it->getFields(fields);
        for (std::vector<DependencyField>::iterator field = fields.begin(); field != fields.end(); ++field)
        {
            DependencyField* depField;
            auto itDep = find_if(list.begin(), list.end(), [&field](const DependencyField& obj) {return obj.getName_() == field->getName_(); });

            if (itDep == list.end()) {
                //TODO: determine a better way to do it, by now, it's only used here, but who knows in the future?
                depField = new DependencyField(field->getName_(), field->getPosition());
                //depField->setName_(*field);

                list.push_back(*depField);
                depField = &list.back();
            }
            else {
                depField = (&*itDep) ;

            }
            MetadataItem *current = getDatabase()->findByNameAndType(it->getType(), it->getName_());
            if (!current)
                continue;
            Dependency de(current, it->getAuxiliar());
            depField->addDependency(de);

        }
    }
}

//! ofObject = true   => returns list of objects this object depends on
//! ofObject = false  => returns list of objects that depend on this object
void MetadataItem::getDependencies(std::vector<Dependency>& list,
    bool ofObject, bool fieldsOnly)
{
    DatabasePtr d = getDatabase();

    int mytype = -1;            // map DBH type to RDB$DEPENDENT TYPE
    NodeType dep_types[] = {    ntTable,    ntView,     ntTrigger,  ntUnknown,  ntUnknown,
                                ntProcedure,ntUnknown,  ntException,ntUnknown,  ntUnknown,
                                ntUnknown,  ntUnknown,  ntUnknown,  ntUnknown,  ntGenerator,
                                ntFunctionSQL, ntUnknown,  ntUnknown,  ntUnknown,  ntPackage
    };
    const int type_count = sizeof(dep_types)/sizeof(NodeType);
    for (int i = 0; i < type_count; i++)
        if (typeM == dep_types[i])
            mytype = i;
    // system tables should be treated as tables
    if (typeM == ntSysTable)
        mytype = 0;
    if (typeM == ntDBTrigger || typeM == ntDDLTrigger || typeM == ntDMLTrigger)
        mytype = 2;
    if (typeM == ntFunctionSQL || typeM == ntUDF)
        mytype = 15;

    int mytype2 = mytype;
    // views count as relations(tables) when other object refer to them
    if (mytype == 1 && !ofObject)
        mytype2 = 0;
    // package header and package body
    if (mytype == ntPackage)
        mytype2 = 18;

    if (typeM == ntUnknown || mytype == -1)
        throw FRError(_("Unsupported type"));
    IBPP::Database& db = d->getIBPPDatabase();
    IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);

    wxString o1 = (ofObject ? "DEPENDENT" : "DEPENDED_ON");
    wxString o2 = (ofObject ? "DEPENDED_ON" : "DEPENDENT");
    wxString sql =
        "select t1.*, f2.rdb$field_position from ( \n "
        "select RDB$" + o2 + "_TYPE, RDB$" + o2 + "_NAME, RDB$FIELD_NAME \n "
        " from RDB$DEPENDENCIES \n "
        " where RDB$" + o1 + "_TYPE in (?,?) and RDB$" + o1 + "_NAME = ? \n ";
    int params = 1;
    if ((typeM == ntTable || typeM == ntSysTable || typeM == ntView) && ofObject)  // get deps for computed columns
    {                                                       // view needed to bind with generators
        sql += " union  \n"
            " SELECT DISTINCT d.rdb$depended_on_type, d.rdb$depended_on_name, d.rdb$field_name \n"
            " FROM rdb$relation_fields f \n"
            " LEFT JOIN rdb$dependencies d ON d.rdb$dependent_name = f.rdb$field_source \n"
            " WHERE d.rdb$dependent_type = 3 AND f.rdb$relation_name = ? \n";
        params++;
    }
    if (!ofObject) // find tables that have calculated columns based on "this" object
    {
        sql += "union  \n"
            " SELECT distinct cast(0 as smallint), f.rdb$relation_name, f.rdb$field_name \n"
            " from rdb$relation_fields f \n"
            " left join rdb$dependencies d on d.rdb$dependent_name = f.rdb$field_source \n"
            " where d.rdb$dependent_type = 3 and d.rdb$depended_on_name = ? ";
        params++;
    }
    // get the exact table and fields for views
    // rdb$dependencies covers deps. for WHERE clauses in SELECTs in VIEW body
    // but we also need mapping for column list in SELECT. These 2 queries cover it:
    if (ofObject && typeM == ntView)
    {
        sql += " union \n"
            " select distinct cast(0 as smallint), vr.RDB$RELATION_NAME, f.RDB$BASE_FIELD \n"
            " from RDB$RELATION_FIELDS f \n"
            " join RDB$VIEW_RELATIONS vr on f.RDB$VIEW_CONTEXT = vr.RDB$VIEW_CONTEXT \n"
            "   and f.RDB$RELATION_NAME = vr.RDB$VIEW_NAME \n"
            " where f.rdb$relation_name = ? \n";
        params++;
    }
    // views can depend on other views as well
    // we might need to add procedures here one day when Firebird gains support for it
    if (!ofObject && (typeM == ntView || typeM == ntTable || typeM == ntSysTable))
    {
        sql += " union \n"
            " select distinct cast(0 as smallint), f.RDB$RELATION_NAME, f.RDB$BASE_FIELD \n"
            " from RDB$RELATION_FIELDS f \n"
            " join RDB$VIEW_RELATIONS vr on f.RDB$VIEW_CONTEXT = vr.RDB$VIEW_CONTEXT \n"
            "   and f.RDB$RELATION_NAME = vr.RDB$VIEW_NAME \n"
            " where vr.rdb$relation_name = ? \n";
        params++;
    }
    sql += " ) t1 \n "
        "left join RDB$RELATION_FIELDS f2 \n "
        "    on f2.RDB$FIELD_NAME = t1.RDB$FIELD_NAME \n "
        "    and f2.RDB$RELATION_NAME = ? \n ";
    params++;

    sql += " order by 1, 2, 3";
    st1->Prepare(wx2std(sql, d->getCharsetConverter()));
    st1->Set(1, mytype);
    st1->Set(2, mytype2);
    for (int i = 0; i < params; i++)
        st1->Set(3 + i, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    MetadataItem* last = NULL;
    Dependency* dep = NULL;
    while (st1->Fetch())
    {
        int object_type;
        st1->Get(1, &object_type);
        if (object_type > type_count)   // some system object, not interesting for us
            continue;
        NodeType t = dep_types[object_type];
        if (t == ntUnknown)             // ditto
            continue;

        std::string objname_std;
        st1->Get(2, objname_std);
        wxString objname(std2wxIdentifier(objname_std,
            d->getCharsetConverter()));

        MetadataItem* current = d->findByNameAndType(t, objname);
        if (!current)
        {
            if (t == ntTable) {
                // maybe it's a view masked as table
                current = d->findByNameAndType(ntView, objname);
                // or possibly a system table
                if (!current)
                    current = d->findByNameAndType(ntSysTable, objname);
            }
            if (!ofObject && (t == ntDMLTrigger || t == ntDBTrigger || t == ntTrigger))
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
                st2->Set(1, objname_std);
                st2->Execute();
                if (st2->Fetch()) // table using that trigger found
                {
                    std::string s;
                    st2->Get(1, s);
                    wxString tablecheck(std2wxIdentifier(s, d->getCharsetConverter()));
                    if (getName_() != tablecheck)    // avoid self-reference
                        current = d->findByNameAndType(ntTable, tablecheck);
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
            std::string s;
            st1->Get(3, s);
            int pos;
            st1->Get(4, pos);

            dep->addField( DependencyField(std2wxIdentifier(s, d->getCharsetConverter()), pos));
        }
    }

    // TODO: perhaps this could be moved to Table?
    //       call MetadataItem::getDependencies() and then add this
    if ((typeM == ntTable || typeM == ntSysTable) && ofObject)   // foreign keys of this table + computed columns
    {
        Table *tab = dynamic_cast<Table *>(this);
        for (const auto iter : *(tab->getForeignKeys()))
        {
            MetadataItem *table = d->findByNameAndType(ntTable,
                iter.getReferencedTable());
            if (!table)
            {
                throw FRError(wxString::Format(_("Table %s not found."),
                    iter.getReferencedTable().c_str()));
            }
            MetadataItem* mi = new MetadataItem(iter);
            Dependency de(table, mi);
            de.setFields(iter.getReferencedColumns());
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
        st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
        st1->Execute();
        std::vector<Dependency> tempdep;
        while (st1->Fetch())
        {
            std::string s;
            st1->Get(1, s);
            DMLTrigger t(d->shared_from_this(),
                std2wxIdentifier(s, d->getCharsetConverter()));
            t.getDependencies(tempdep, true);
        }
        // remove duplicates, and self-references from "tempdep"
        while (true)
        {
            std::vector<Dependency>::iterator to_remove = tempdep.end();
            for (std::vector<Dependency>::iterator it = tempdep.begin();
                it != tempdep.end(); ++it)
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
            "select r1.rdb$relation_name, i.rdb$field_name, i.RDB$FIELD_POSITION, R1.RDB$CONSTRAINT_NAME, i2.RDB$FIELD_NAME "
            " from rdb$relation_constraints r1 "
            " join rdb$ref_constraints c on r1.rdb$constraint_name = c.rdb$constraint_name "
            " join rdb$relation_constraints r2 on c.RDB$CONST_NAME_UQ = r2.rdb$constraint_name "
            " join rdb$index_segments i on r1.rdb$index_name=i.rdb$index_name "
            " left join rdb$index_segments i2 on c.RDB$CONST_NAME_UQ = i2.rdb$index_name and i2.RDB$FIELD_POSITION = i.RDB$FIELD_POSITION "
            " where r2.rdb$relation_name=? "
            " and r1.rdb$constraint_type='FOREIGN KEY' "
        );
        st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
        st1->Execute();
        wxString lasttable;
        dep = NULL;
        while (st1->Fetch())
        {
            std::string s;
            st1->Get(1, s);
            wxString table_name(std2wxIdentifier(s, d->getCharsetConverter()));

            st1->Get(2, s);
            if (fieldsOnly)
            st1->Get(5, s);
            wxString field_name(std2wxIdentifier(s, d->getCharsetConverter()));
            int pos;
            st1->Get(3, pos);

            if (table_name != lasttable)    // new
            {
                MetadataItem* table = d->findByNameAndType(ntTable, table_name);

                if (!table)
                    continue;           // dummy check
                ForeignKey* fk = new ForeignKey();
                st1->Get(4, s);
                wxString fk_name(std2wxIdentifier(s, d->getCharsetConverter()));
                fk->setName_(fk_name);
                fk->setParent(table);

                Dependency de(table, fk);

                list.push_back(de);
                dep = &list.back();
                lasttable = table_name;
            }
            dep->addField(DependencyField(field_name, pos));
        }
    }

    tr1->Commit();
}

void MetadataItem::ensureDescriptionLoaded()
{
    if (descriptionLoadedM == lsNotLoaded)
        loadDescription();
}

wxString MetadataItem::getDescription()
{
    ensureDescriptionLoaded();
    return descriptionM;
}

bool MetadataItem::getDescription(wxString& description)
{
    ensureDescriptionLoaded();
    description = descriptionM;
    return descriptionLoadedM == lsLoaded;
}


void MetadataItem::invalidateDescription()
{
    if (descriptionLoadedM != lsNotLoaded)
    {
        descriptionLoadedM = lsNotLoaded;
        descriptionM = wxEmptyString;
        // call notifyObservers(), because this is only called after
        // the description has been changed by a committed SQL statement
        notifyObservers();
    }
}

void MetadataItem::loadDescription()
{
    LoadDescriptionVisitor ldv;
    acceptVisitor(&ldv);
    // don't call notifyObservers() !
    // since descriptions are loaded on-demand, doing so would result in
    // additional activity at best, and crashes or infinite loops at worst
    if (ldv.descriptionAvailable())
    {
        descriptionLoadedM = lsLoaded;
        descriptionM = ldv.getDescription();
    }
    else
    {
        descriptionLoadedM = lsNotAvailable;
        descriptionM = wxEmptyString;
    }
}

void MetadataItem::saveDescription(const wxString& WXUNUSED(description))
{
    throw FRError(wxString::Format(
        "Objects of type %s do not support descriptions",
        getTypeName().c_str()));
}

void MetadataItem::setDescription(const wxString& description)
{
    if (getDescription() != description)
    {
        SaveDescriptionVisitor sdv(description);
        acceptVisitor(&sdv);
        // if previous statement didn't throw the description has been saved
        descriptionLoadedM = lsLoaded;
        descriptionM = description;
        // call notifyObservers(), because this is only called after
        // the description has been edited by the user
        notifyObservers();
    }
}

void MetadataItem::setDescriptionIsEmpty()
{
    descriptionLoadedM = lsLoaded;
    descriptionM = wxEmptyString;
}

MetadataItem* MetadataItem::getParent() const
{
    return parentM;
}

void MetadataItem::setParent(MetadataItem* parent)
{
    parentM = parent;
}

wxString MetadataItem::getName_() const
{
    return identifierM.get();
}

wxString MetadataItem::getQuotedName() const
{
    return identifierM.getQuoted();
}

Identifier MetadataItem::getIdentifier() const
{
    return identifierM;
}

void MetadataItem::setName_(const wxString& name)
{
    identifierM.setText(name);
    notifyObservers();
}

NodeType MetadataItem::getType() const
{
    return typeM;
}

void MetadataItem::setType(NodeType type)
{
    typeM = type;
}

int MetadataItem::getMetadataId()
{
    return metadataIdM;
}

void MetadataItem::setMetadataId(int id)
{
    metadataIdM = id;
}

bool MetadataItem::isSystem() const
{
    return hasSystemPrefix(getName_());
}

/*static*/
bool MetadataItem::hasSystemPrefix(const wxString& name)
{
    wxString prefix(name.substr(0, 4));
    return prefix == "RDB$" || prefix == "MON$" || prefix == "SEC$";
}

wxString MetadataItem::getDropSqlStatement() const
{
    return "DROP " + getTypeName() + " " + getQuotedName() + ";";
}

void MetadataItem::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitMetadataItem(*this);
}

void MetadataItem::lockChildren()
{
// NOTE: getChildren() can not be used here, because we want to lock the
//       MetadataCollection objects as well.  That means we have to override
//       this method in all descendant classes - oh well...
}

void MetadataItem::lockSubject()
{
    Subject::lockSubject();
    lockChildren();
}

void MetadataItem::unlockChildren()
{
// NOTE: getChildren() can not be used here, because we want to lock the
//       MetadataCollection objects as well.  That means we have to override
//       this method in all descendant classes - oh well...
}

void MetadataItem::unlockSubject()
{
    Subject::unlockSubject();
    unlockChildren();
}

MetadataItem *Dependency::getParent() const
{
    return objectM->getParent();
}

wxString Dependency::getName_() const
{
    return objectM->getName_();
}

NodeType Dependency::getType() const
{
    return objectM->getType();
}

const wxString Dependency::getTypeName() const
{
    return objectM->getTypeName();
}

MetadataItem *Dependency::getDependentObject() const
{
    return objectM;
}

MetadataItem * Dependency::getAuxiliar() const
{
    return auxiliarM;
}

Dependency::Dependency(MetadataItem *object, MetadataItem *auxiliar)
{
    objectM = object;
    auxiliarM = auxiliar;
}

void Dependency::getFields(std::vector<DependencyField>& fields) const
{
    for (std::vector<DependencyField>::const_iterator it = fieldsM.begin();
        it != fieldsM.end(); ++it)
    {
        fields.push_back(*it);
    }
}

wxString Dependency::getFields() const
{
    wxString temp;
    for (std::vector<DependencyField>::const_iterator it = fieldsM.begin(); it != fieldsM.end(); ++it)
    {
        if (it != fieldsM.begin())
            temp += ", ";
        temp += (*it).getName_();
    }
    return temp;
}

void Dependency::addField(const DependencyField& name)
{
    if (fieldsM.end() == std::find(fieldsM.begin(), fieldsM.end(), name))
    {
        std::vector<DependencyField>::const_iterator it = std::lower_bound(fieldsM.begin(), fieldsM.end(), name); // find proper position in descending order
        this->fieldsM.insert(it, name); // insert before iterator it
        //fieldsM.push_back(name);
    }
}

void Dependency::setFields(const std::vector<DependencyField>& fields)
{
    fieldsM = fields;
}
void Dependency::setFields(const std::vector<wxString>& fields)
{
    fieldsM = std::vector<DependencyField>();
    for (std::vector<wxString>::const_iterator it = fields.begin(); it != fields.end(); ++it)
    {
        //fieldsM.push_back(DependencyField(*it, 0));
        this->addField(DependencyField(*it, 0));
    }
}

bool Dependency::hasField(const DependencyField& name) const
{
    return fieldsM.end() != std::find(fieldsM.begin(), fieldsM.end(), name);
}

bool Dependency::operator== (const Dependency& other) const
{
    return (objectM == other.getDependentObject() && getFields() == other.getFields());
}

bool Dependency::operator!= (const Dependency& other) const
{
    return (objectM != other.getDependentObject() || getFields() != other.getFields());
}

bool Dependency::operator<(const Dependency & other) const
{
    if (this->getType() == other.getType())
        return this->getName_() < other.getName_();
    return  this->getType() < other.getType();
}


void Dependency::acceptVisitor(MetadataItemVisitor* visitor)
{
    if (objectM)
        objectM->acceptVisitor(visitor);
}

DependencyField::DependencyField(wxString name, int position)
    :positionM(position)
{
    setName_(name);
}
int DependencyField::getPosition()
{
    return this->positionM;
}

void DependencyField::getDependencies(std::vector<Dependency>& list) const
{
    for (std::vector<Dependency>::const_iterator it = objectsM_.begin();
        it != objectsM_.end(); ++it)
    {
        list.push_back(*it);
    }
}

void DependencyField::addDependency(const Dependency & other)
{
    std::vector<Dependency>::const_iterator it = std::lower_bound(objectsM_.begin(), objectsM_.end(), other); // find proper position in descending order
    this->objectsM_.insert(it, other); // insert before iterator it
    //this->objectsM_.push_back(other);
}

bool DependencyField::operator==(const DependencyField & other) const
{
    //TODO: verify correctly if the object is the same
    return other.getName_()==this->getName_();
}

bool DependencyField::operator<(const DependencyField & other) const
{
    if (this->positionM == other.positionM)
        return this->getName_() < other.getName_();
    return this->positionM < other.positionM;//TODO: enable after fix position in every possible place
}
