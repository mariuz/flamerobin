/*
  Copyright (c) 2004-2024 The FlameRobin Development Team

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


#ifndef FR_USER_30_H
#define FR_USER_30_H

#include <ibpp.h>

#include "metadata/MetadataClasses.h"
#include "metadata/metadataitem.h"
#include "metadata/User.h"


class User30: public User
{
protected:
    virtual void loadProperties();
public:
    User30(DatabasePtr database, const wxString& name);

    virtual wxString getAlterSqlStatement();

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

#endif // FR_USER_30_H
