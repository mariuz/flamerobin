/*
  Copyright (c) 2004-2013 The FlameRobin Development Team

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

//-----------------------------------------------------------------------------
#ifndef FR_TRIGGER_H
#define FR_TRIGGER_H

#include "metadata/collection.h"
#include "metadata/database.h"

class ProgressIndicator;
//-----------------------------------------------------------------------------
class Trigger: public MetadataItem
{
public:
    enum FiringTime
    {
        invalid,
        // relation triggers
        beforeIUD, afterIUD,
        // database triggers
        databaseConnect, databaseDisconnect,
        transactionStart, transactionCommit, transactionRollback
    };
private:
    wxString relationNameM;
    bool activeM;
    int positionM;
    wxString sourceM;
    int typeM;

    static FiringTime getFiringTime(int type);
protected:
    virtual void loadProperties();
public:
    Trigger(DatabasePtr database, const wxString& name);

    bool getActive();
    wxString getFiringEvent();
    FiringTime getFiringTime();
    int getPosition();
    wxString getRelationName();
    wxString getSource();
    wxString getAlterSql();
    bool isDatabaseTrigger();

    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
class Triggers: public MetadataCollection<Trigger>
{
protected:
    virtual void loadChildren();
public:
    Triggers(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};
//-----------------------------------------------------------------------------
#endif
