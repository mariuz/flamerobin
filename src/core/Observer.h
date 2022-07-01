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

#ifndef FR_OBSERVER_H
#define FR_OBSERVER_H

#include <list>

class Subject;

class Observer
{
private:
    unsigned updateLockM;
    // pointer to objects that it is watching
    std::list<Subject*> subjectsM;

    // following methods are only called from Subject
    friend class Subject;

    void addSubject(Subject* subject);
    void removeSubject(Subject* subject);
    // BE CAREFUL: if function gets called when subject is destroyed
    //             its derived-class destructor has already been called so
    //             you can't, for example, dynamic_cast it to MetadataItem*
    virtual void subjectRemoved(Subject* subject);

    // call doUpdate() instead of update() from Subject
    // to prevent recursive calls
    void doUpdate();
    // only Subject calls it, descending classes can still override it
    virtual void update() = 0;
public:
    Observer();
    virtual ~Observer();
};

#endif
