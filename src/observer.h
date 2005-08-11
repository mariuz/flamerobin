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

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  Contributor(s):
*/

//
//Observer pattern [GoF]
//
//
//-----------------------------------------------------------------------------
#ifndef FR_OBSERVER_H
#define FR_OBSERVER_H

#include <list>

class Subject;
//-----------------------------------------------------------------------------
class Observer
{
protected:
	std::list<Subject *> observedObjectsM;			// pointer to objects that it is watching
public:
	Observer();
	virtual ~Observer();
	virtual void update() = 0;

	Subject *getFirstObservedObject();
	void addObservedObject(Subject *object);
	virtual void removeObservedObject(Subject *object);	// virtual so some controls can do something extra
};
//-----------------------------------------------------------------------------
#endif
