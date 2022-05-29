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


#include <algorithm>
#include <list>

#include "core/Observer.h"
#include "core/Subject.h"

class ObserverLocker
{
private:
    unsigned* lockPtrM;
public:
    ObserverLocker(unsigned* lock);
    ~ObserverLocker();
};

ObserverLocker::ObserverLocker(unsigned* lock)
    : lockPtrM(lock)
{
    if (lockPtrM)
        ++(*lockPtrM);
}

ObserverLocker::~ObserverLocker()
{
    if (lockPtrM)
        --(*lockPtrM);
}

Observer::Observer()
    : updateLockM(0)
{
}

Observer::~Observer()
{
    while (!subjectsM.empty())
    {
        std::list<Subject*>::iterator it = subjectsM.begin();
        // object will be removed by removeObservedObject()
        (*it)->detachObserver(this);
    }
}

void Observer::doUpdate()
{
    ObserverLocker lock(&updateLockM);
    if (updateLockM == 1)
        update();
}

void Observer::addSubject(Subject* subject)
{
    if (subject)
        subjectsM.push_back(subject);
}

void Observer::removeSubject(Subject* subject)
{
    std::list<Subject*>::iterator it = find(subjectsM.begin(),
        subjectsM.end(), subject);
    if (it != subjectsM.end())
    {
        subjectsM.erase(it);
        subjectRemoved(subject);
    }
}

void Observer::subjectRemoved(Subject* /*subject*/)
{
}

