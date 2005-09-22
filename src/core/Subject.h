/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Gregory Sapunkov.

  Portions created by the original developer
  are Copyright (C) 2004 Gregory Sapunkov.

  All Rights Reserved.

  $Id$

  Contributor(s): Milan Babuskov, Michael Hieke
*/

#ifndef FR_SUBJECT_H
#define FR_SUBJECT_H
//-----------------------------------------------------------------------------
#include <list>
#include <vector>

class Observer;
//-----------------------------------------------------------------------------
class Subject
{
protected:
    std::list<Observer*> observersM;
    int locksCountM;
    bool needsNotifyObjectsM;

public:
    Subject();
    virtual ~Subject();

    void attachObserver(Observer* observer);
    void detachObserver(Observer* observer);
    void detachAllObservers();
    void notifyObservers();
    // TODO: Make these protected, and this class and SubjectLocker friends,
    //       to force the use of SubjectLocker (exception-safe locking).
    //       Right now (un)lockSubject() is used in Root::load().
    virtual void lockSubject();
    virtual void unlockSubject();
};
//-----------------------------------------------------------------------------
class SubjectLocker
{
private:
    Subject* subjectM;
protected:
    Subject* getSubject();
    void setSubject(Subject* subject);
public:
    SubjectLocker(Subject* subject);
    ~SubjectLocker();
};
//-----------------------------------------------------------------------------
#endif
