/*
  Copyright (c) 2004-2025 The FlameRobin Development Team

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

// Regression tests for GitHub issue #436:
// "Fatal Error when Dropping a Trigger"
//
// Root cause: DBHTreeItemData (a Subject observer) holds a raw MetadataItem*
// pointer (observedItemM).  When a trigger is dropped via SQL the collection
// erases its shared_ptr, destroying the MetadataItem.  Subject::~Subject()
// calls detachAllObservers() -> Observer::removeSubject() ->
// subjectRemoved(subject).  Because DBHTreeItemData does not override
// subjectRemoved(), observedItemM is never cleared.  A subsequent call to
// getSelectedMetadataItem() returns that dangling pointer, and calling
// getDatabase() on it crashes the application.
//
// These tests verify the Observer/Subject contract that makes a safe fix
// possible: subjectRemoved() IS called whenever the subject is either
// explicitly detached or destroyed, giving observers a chance to clear
// any raw pointer they hold to it.

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <iostream>

#include "core/Observer.h"
#include "core/Subject.h"

namespace
{

bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << testName << " failed.\n";
    return false;
}

// Minimal concrete Subject used by the tests.
class TestSubject : public Subject
{
};

// Observer that mirrors the FIXED behaviour that DBHTreeItemData should
// implement: it clears its raw pointer inside subjectRemoved() so that
// the pointer is never left dangling after the Subject is destroyed.
class SafeObserver : public Observer
{
public:
    Subject* rawPtrM;
    int updateCallCount;
    int subjectRemovedCallCount;

    SafeObserver()
        : rawPtrM(nullptr), updateCallCount(0), subjectRemovedCallCount(0)
    {
    }

    void observeSubject(Subject* s)
    {
        if (rawPtrM != s)
        {
            if (rawPtrM)
                rawPtrM->detachObserver(this);
            rawPtrM = s;
            if (rawPtrM)
                rawPtrM->attachObserver(this, false);
        }
    }

protected:
    virtual void update() override
    {
        ++updateCallCount;
    }

    // Correctly clears the raw pointer when the subject is removed/destroyed.
    // This is the fix for issue #436.
    virtual void subjectRemoved(Subject* /*subject*/) override
    {
        rawPtrM = nullptr;
        ++subjectRemovedCallCount;
    }
};

} // namespace

int main()
{
    bool ok = true;

    // Test 1: notifyObservers() calls update() on attached observers.
    {
        TestSubject subject;
        SafeObserver obs;
        obs.observeSubject(&subject);

        subject.notifyObservers();

        ok = check(obs.updateCallCount == 1,
            "notifyObservers calls update()") && ok;
    }

    // Test 2: subjectRemoved() is called when Subject::detachObserver() is
    // invoked explicitly – the pointer should be cleared afterwards.
    {
        TestSubject subject;
        SafeObserver obs;
        obs.observeSubject(&subject);

        ok = check(obs.rawPtrM != nullptr,
            "explicit detach: ptr set before detach") && ok;

        subject.detachObserver(&obs);

        ok = check(obs.subjectRemovedCallCount == 1,
            "explicit detach: subjectRemoved() called") && ok;
        ok = check(obs.rawPtrM == nullptr,
            "explicit detach: ptr cleared") && ok;
    }

    // Test 3: subjectRemoved() is called when the Subject is DESTROYED –
    // regression for issue #436.
    //
    // When a trigger is dropped the MetadataCollection erases its shared_ptr,
    // which destroys the MetadataItem (Subject).  Subject::~Subject() calls
    // detachAllObservers(), which in turn calls subjectRemoved() on every
    // attached observer.  An observer that overrides subjectRemoved() and
    // clears its raw pointer avoids the dangling-pointer crash.
    {
        SafeObserver obs;

        {
            TestSubject subject;
            obs.observeSubject(&subject);

            ok = check(obs.rawPtrM != nullptr,
                "issue #436: ptr set while subject alive") && ok;

            // subject goes out of scope here and is destroyed
        }

        ok = check(obs.subjectRemovedCallCount == 1,
            "issue #436: subjectRemoved() called on subject destruction") && ok;
        ok = check(obs.rawPtrM == nullptr,
            "issue #436: ptr cleared when subject destroyed") && ok;
    }

    // Test 4: Detaching an observer that was never attached is a no-op and
    // does not call subjectRemoved().
    {
        TestSubject subject;
        SafeObserver obs;
        subject.detachObserver(&obs);   // never attached

        ok = check(obs.subjectRemovedCallCount == 0,
            "detach unattached observer: subjectRemoved() not called") && ok;
    }

    // Test 5: A single observer can be attached to multiple subjects; each
    // destruction independently calls subjectRemoved().
    {
        SafeObserver obs;
        {
            TestSubject s1;
            TestSubject s2;
            s1.attachObserver(&obs, false);
            s2.attachObserver(&obs, false);

            // Destroying s1 calls subjectRemoved() once; obs.rawPtrM is not
            // used here (the observer is tracking the count, not a pointer).
        }
        // Both subjects destroyed: subjectRemoved() called twice.
        ok = check(obs.subjectRemovedCallCount == 2,
            "two subjects: subjectRemoved() called for each") && ok;
    }

    return ok ? 0 : 1;
}
