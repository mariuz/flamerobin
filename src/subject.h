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

  Contributor(s): Milan Babuskov.
*/

//
//
//
//
//------------------------------------------------------------------------------
#ifndef FR_SUBJECT_H
#define FR_SUBJECT_H

#include <string>
#include <vector>
#include <list>

class Observer;
//------------------------------------------------------------------------------
class Subject
{
protected:
    std::list<Observer *> observersM;
    int locksCountM;

public:
	Subject();
	virtual ~Subject();

    void attach(Observer*);
    void detach(Observer*);
	void detachAllObservers();
    void notify();
    void lockSubject();
    void unlockSubject(bool wantFullUnlock = false, bool doNotify = true);
};
//------------------------------------------------------------------------------
#endif
