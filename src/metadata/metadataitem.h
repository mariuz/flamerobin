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


#ifndef FR_METADATAITEM_H
#define FR_METADATAITEM_H

#include <wx/string.h>

#include <algorithm>
#include <vector>

#include "core/ObjectWithHandle.h"
#include "core/ProcessableObject.h"
#include "core/Subject.h"
#include "metadata/MetadataClasses.h"
#include "sql/Identifier.h"

class Dependency;
class DependencyField;
class MetadataItemVisitor;

typedef enum { ntUnknown, ntRoot, ntServer, ntDatabase,
    // each item type and (if applicable) its parent type
    ntTable, ntTables, 
    ntGTT, ntGTTs,
    ntView, ntViews, 
    ntProcedure, ntProcedures,
    ntTrigger, ntTriggers, 
    ntGenerator, ntGenerators, 
    ntFunction, ntFunctions, 
    ntFunctionSQL, ntFunctionSQLs, 
    ntUDF,  ntUDFs,
    ntSysTable, ntSysTables, 
    ntException, ntExceptions,
    ntDomain, ntDomains, 
    ntSysDomain, ntSysDomains,
    ntRole, ntRoles, 
    ntSysRole, ntSysRoles, 
    ntColumn, ntParameter, 
    ntIndex, ntIndices, 
    ntSysIndices, ntUsrIndices,
    ntPackage, ntPackages, 
    ntSysPackage, ntSysPackages,
    ntDMLTrigger, ntDMLTriggers,
    ntDBTrigger, ntDBTriggers, 
    ntDDLTrigger, ntDDLTriggers,
    ntMethod,
    ntUser, ntUsers,
    ntSystem, 
    ntCharacterSet, ntChartersets,
    ntSysCollation, ntSysCollations, 
    ntCollation, ntCollations,
    ntLastType
} NodeType;

NodeType getTypeByName(const wxString& name);
wxString getNameOfType(NodeType type);

void initializeLockCount(MetadataItem* item, unsigned count);
void initializeLockCount(MetadataItemPtr item, unsigned count);

class MetadataItem: public Subject, public ObjectWithHandle<MetadataItem>,
    public ProcessableObject
{
private:
    MetadataItem* parentM;
    NodeType typeM;
    Identifier identifierM;
    int metadataIdM;

    enum LoadState { lsNotLoaded, lsLoadPending, lsLoaded, lsNotAvailable };
    LoadState childrenLoadedM;
    LoadState descriptionLoadedM;
    LoadState propertiesLoadedM;

    wxString descriptionM;
    void ensureDescriptionLoaded();

protected:
    virtual void loadDescription();
    virtual void saveDescription(const wxString& description);
    void saveDescription(const wxString& saveStatement,
        const wxString& description);
    // used internally if rdb$description column (a blob) is null
    void setDescriptionIsEmpty();

    virtual void loadProperties();
    void setPropertiesLoaded(bool loaded);

    virtual void doSetChildrenLoaded(bool loaded);
    virtual void loadChildren();
    virtual void lockChildren();
    virtual void unlockChildren();

    void resetPendingLoadData();

public:
    MetadataItem();
    MetadataItem(NodeType type, MetadataItem* parent = 0,
        const wxString& name = wxEmptyString, int id = -1);
    virtual ~MetadataItem();

    virtual void lockSubject();
    virtual void unlockSubject();

    void getDependencies(std::vector<Dependency>& list, bool ofObject, bool fieldsOnly=false);  // load from db
    void getDependencies(std::vector<Dependency>& list, bool ofObject,
        const wxString& field);  // load from db
    void getDependenciesPivoted(std::vector<DependencyField>& list);


    // returned shared ptr may be unassigned
    virtual DatabasePtr getDatabase() const;

    virtual void invalidate();

    // items description (in database)
    wxString getDescription();
    bool getDescription(wxString& description);
    void invalidateDescription();
    void setDescription(const wxString& description);

    bool childrenLoaded() const;
    void ensureChildrenLoaded();
    void ensurePropertiesLoaded();
    void loadPendingData();
    bool propertiesLoaded() const;
    void setChildrenLoaded(bool loaded);

    virtual bool getChildren(std::vector<MetadataItem *>& temp);
    virtual size_t getChildrenCount() const { return 0; };

    // returns complete DROP SQL statement
    virtual wxString getDropSqlStatement() const;

    // getters/setters
    virtual MetadataItem* getParent() const;
    void setParent(MetadataItem* parent);
    virtual wxString getName_() const;
    virtual wxString getQuotedName() const;
    virtual Identifier getIdentifier() const;
    virtual void setName_(const wxString& name);
    virtual NodeType getType() const;
    void setType(NodeType type);
    virtual int getMetadataId();
    virtual void setMetadataId(int id);

    // returns the name of the data type (f. ex. TABLE)
    virtual const wxString getTypeName() const;

    // returns the item path, currently only used to store settings in config().
    // It could also be used to locate items in the DBH tree.
    virtual const wxString getItemPath() const;

    // returns the id wxString of the metadata item that contributes to the path. The
    // predefined implementation just returns getId().
    virtual const wxString getPathId() const;
    // returns the id of the item (to be saved in config files, etc.).
    // The predefined implementation just returns getName_().
    virtual const wxString getId() const;

    // returns true if the metadata item is a system (as opposed to user-defined) item.
    virtual bool isSystem() const;
    static bool hasSystemPrefix(const wxString& name);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

class DependencyField : public MetadataItem
{
private:
std::vector<Dependency> objectsM_;
int positionM = 0;
public:
    DependencyField(wxString name, int position = 0);
    int getPosition();
    void getDependencies(std::vector<Dependency>& list) const;
    void addDependency(const Dependency& other);
    bool operator== (const DependencyField& other) const;
    bool operator <(const DependencyField& other) const; //For sorting

};

//! masks the object it points to so others see it transparently
class Dependency: public MetadataItem
{
private:
    MetadataItem *objectM;
    std::vector<DependencyField> fieldsM;
    MetadataItem *auxiliarM;  //For example, when listing a table as tependency of another tables, where whe will reference the FK related

public:
    virtual MetadataItem *getParent() const;
    virtual wxString getName_() const;
    virtual NodeType getType() const;
    virtual const wxString getTypeName() const;
    MetadataItem *getDependentObject() const;
    MetadataItem * getAuxiliar() const;

    Dependency(MetadataItem *object, MetadataItem *auxiliar=0);
    wxString getFields() const;
    void getFields(std::vector<DependencyField>& fields) const;
    void addField(const DependencyField& name);
    void setFields(const std::vector<DependencyField>& fields);
    void setFields(const std::vector<wxString>& fields);
    bool hasField(const DependencyField& name) const;
    bool operator== (const Dependency& other) const;
    bool operator!= (const Dependency& other) const;
    bool operator <(const Dependency& other) const; //For sorting
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

#endif //FR_METADATAITEM_H
