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


#ifndef FR_ROLE_H
#define FR_ROLE_H

#include <vector>

#include "metadata/collection.h"
#include "metadata/metadataitem.h"
#include "metadata/privilege.h"

class ProgressIndicator;

class Role: public MetadataItem
{
private:
    std::vector<Privilege> privilegesM;
public:
    Role(DatabasePtr database, const wxString& name);
    wxString getOwner();
    std::vector<Privilege>* getPrivileges();
    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

class SysRoles: public MetadataCollection<Role>
{
protected:
    virtual void loadChildren();
public:
    SysRoles(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual bool isSystem() const;
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

class Roles: public MetadataCollection<Role>
{
protected:
    virtual void loadChildren();
public:
    Roles(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

#endif // FR_ROLE_H
