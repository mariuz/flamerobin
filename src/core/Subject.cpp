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

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <algorithm>
#include <list>

#include "core/Observer.h"
#include "core/Subject.h"
//-----------------------------------------------------------------------------
using namespace std;

typedef list<Observer*>::iterator ObserverIterator;
//-----------------------------------------------------------------------------
Subject::Subject()
{
    locksCountM = 0;
    needsNotifyObjectsM = false;
}
//-----------------------------------------------------------------------------
Subject::~Subject()
{
    detachAllObservers();
}
//-----------------------------------------------------------------------------
void Subject::attachObserver(Observer* observer)
{
    if (observer && std::find(observersM.begin(), observersM.end(), observer)
        == observersM.end())
    {
        observer->addSubject(this);
        observersM.push_back(observer);
        observer->update();
    }
}
//-----------------------------------------------------------------------------
void Subject::detachObserver(Observer* observer)
{
    if (!observer)
        return;

    observer->removeSubject(this);
    observersM.erase(find(observersM.begin(), observersM.end(), observer));
}
//-----------------------------------------------------------------------------
void Subject::detachAllObservers()
{
    for (ObserverIterator i = observersM.begin(); i != observersM.end(); ++i)
        (*i)->removeSubject(this);
    observersM.clear();
}
//-----------------------------------------------------------------------------
void Subject::notifyObservers()
{
    if (isLocked())
        needsNotifyObjectsM = true;
    else
    {
        // make sure there are no reentrancy problems
        ++locksCountM;
        for (ObserverIterator i = observersM.begin(); i != observersM.end(); ++i)
            (*i)->update();
        --locksCountM;
        needsNotifyObjectsM = false;
    }
}
//-----------------------------------------------------------------------------
void Subject::lockSubject()
{
    if (!locksCountM)
        lockedChanged(true);
    ++locksCountM;
}
//-----------------------------------------------------------------------------
void Subject::unlockSubject()
{
    if (isLocked())
    {
        --locksCountM;
        if (!isLocked())
        {
            lockedChanged(false);
            if (needsNotifyObjectsM)
                notifyObservers();
        }
    }
}
//-----------------------------------------------------------------------------
void Subject::lockedChanged(bool /*locked*/)
{
}
//-----------------------------------------------------------------------------
unsigned int Subject::getLockCount()
{
    return locksCountM;
}
//-----------------------------------------------------------------------------
bool Subject::isLocked()
{
    return locksCountM > 0;
}
//-----------------------------------------------------------------------------
SubjectLocker::SubjectLocker(Subject* subject)
{
    subjectM = 0;
    setSubject(subject);
}
//-----------------------------------------------------------------------------
SubjectLocker::~SubjectLocker()
{
    setSubject(0);
}
//-----------------------------------------------------------------------------
Subject* SubjectLocker::getSubject()
{
    return subjectM;
}
//-----------------------------------------------------------------------------
void SubjectLocker::setSubject(Subject* subject)
{
    if (subject != subjectM)
    {
        if (subjectM)
            subjectM->unlockSubject();
        subjectM = subject;
        if (subjectM)
            subjectM->lockSubject();
    }
}
//-----------------------------------------------------------------------------
