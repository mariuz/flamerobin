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

#ifndef FR_PACKAGE_H
#define FR_PACKAGE_H

#include <vector>

#include "metadata/collection.h"
#include "metadata/privilege.h"

class ProgressIndicator;

class Method : public MetadataItem
{
private:
    bool functionM;
    wxString resultM;
protected:
public:
    Method(MetadataItem* parent, const wxString& name);

    bool isFunction() const;
    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void initialize(int MethodType);
    virtual wxString getQuotedName() const;

};

class Package : public MetadataItem
{
private:
    std::vector<Privilege> privilegesM;
    MethodPtrs methodsM;
    FunctionSQLPtrs functionsM;
    ProcedurePtrs proceduresM;
protected:
    virtual void loadChildren();
    virtual void lockChildren();
    virtual void unlockChildren();
    FunctionSQLPtr findFunctionSQL(const wxString& name) const;
    ProcedurePtr findProcedure(const wxString& name) const;
public:
    Package(DatabasePtr database, const wxString& name);

    bool getChildren(std::vector<MetadataItem *>& temp);

    MethodPtrs::iterator begin();
    MethodPtrs::iterator end();
    MethodPtrs::const_iterator begin() const;
    MethodPtrs::const_iterator end() const;

    size_t getMethodCount() const;
    MethodPtr findMethod(const wxString& name) const;

    wxString getOwner();
    wxString getSource();
    wxString getAlterSql(bool full = true);
    wxString getDefinition();   // used for calltip in sql editor
    wxString getSqlSecurity();
    wxString getAlterHeader();
    wxString getAlterBody();

    std::vector<Privilege>* getPrivileges(bool splitPerGrantor=true);

    void checkDependentPackage();

    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);

};

class Packages: public MetadataCollection<Package>
{
protected:
    virtual void loadChildren();
public:
    Packages(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

class SysPackages : public MetadataCollection<Package>
{
protected:
    virtual void loadChildren();
public:
    SysPackages(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};


#endif
