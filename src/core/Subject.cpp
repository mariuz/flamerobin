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
    }
}
//-----------------------------------------------------------------------------
void Subject::detachObserver(Observer* observer)
{
    if(!observer)
        return;

    observer->removeSubject(this);
    observersM.erase(find(observersM.begin(), observersM.end(), observer));
/*  // TODO: remove this when no negative side-effects
    if(!observersM.size())
        locksCountM = 0;
*/
}
//-----------------------------------------------------------------------------
void Subject::detachAllObservers()
{
    for (ObserverIterator i = observersM.begin(); i != observersM.end(); ++i)
        (*i)->removeSubject(this);
    observersM.clear();
/*  // TODO: remove this when no negative side-effects
    locksCountM = 0;
*/
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
inline void Subject::lockSubject()
{
    ++locksCountM;
}
//-----------------------------------------------------------------------------
void Subject::unlockSubject()
{
    if (isLocked())
    {
        --locksCountM;
        if (!isLocked() && needsNotifyObjectsM)
            notifyObservers();
    }
}
//-----------------------------------------------------------------------------
inline bool Subject::isLocked()
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
