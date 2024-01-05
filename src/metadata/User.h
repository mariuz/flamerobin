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


#ifndef FR_USER_H
#define FR_USER_H

#include <ibpp.h>

#include "metadata/MetadataClasses.h"
#include "metadata/metadataitem.h"

class User: public MetadataItem
    ,public std::enable_shared_from_this<User>
{
private:
    ServerWeakPtr serverM;
    wxString usernameM;
    wxString passwordM;
    wxString firstnameM;
    wxString middlenameM;
    wxString lastnameM;
    uint32_t useridM;
    uint32_t groupidM;
protected:
    virtual void loadProperties();
public:
    User(ServerPtr server);
    User(ServerPtr server, const IBPP::User& src);
    User(DatabasePtr database, const wxString& name);


    ServerPtr getServer() const;
    virtual bool isSystem() const;

    wxString getUsername() const;
    wxString getPassword() const;
    wxString getFirstName() const;
    wxString getMiddleName() const;
    wxString getLastName() const;
    uint32_t getUserId() const;
    uint32_t getGroupId() const;
    IBPP::User& getUserIBPP() const;

    void setUsername(const wxString& value);
    void setPassword(const wxString& value);
    void setFirstName(const wxString& value);
    void setMiddleName(const wxString& value);
    void setLastName(const wxString& value);
    void setUserId(uint32_t value);
    void setGroupId(uint32_t value);
    void setUserIBPP(const IBPP::User& usr);

    void assignTo(IBPP::User& dest) const;

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual const wxString getTypeName() const;
    virtual wxString getSource();


};

class User20: public User
{
protected:
    virtual void loadProperties();
public:
    User20(ServerPtr server);
    User20(ServerPtr server, const IBPP::User& src);
    User20(DatabasePtr database, const wxString& name);
};

class User30 : public User
{
protected:
    virtual void loadProperties();
public:
    User30(ServerPtr server);
    User30(ServerPtr server, const IBPP::User& src);
    User30(DatabasePtr database, const wxString& name);

};


class Users : public MetadataCollection<User>
{
protected:
    virtual void loadChildren();
public:
    Users(DatabasePtr database);


    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;

};

class Users20 : public Users
{
public:
    Users20(DatabasePtr database);
    virtual void load(ProgressIndicator* progressIndicator);

    virtual ItemType newItem(const wxString & name) {
        ItemType item(new User20(getDatabase(), name));
        return item;
    }

};

class Users30 : public Users
{
public:
    Users30(DatabasePtr database);
    virtual void load(ProgressIndicator* progressIndicator);

    virtual ItemType newItem(const wxString& name) {
        ItemType item(new User30(getDatabase(), name));
        return item;
    }
};




#endif
