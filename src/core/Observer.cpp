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
//-----------------------------------------------------------------------------
#include <algorithm>
#include <list>

#include "core/Observer.h"
#include "core/Subject.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
Observer::Observer()
{
}
//-----------------------------------------------------------------------------
Observer::~Observer()
{
    while (!subjectsM.empty())
    {
        list<Subject*>::iterator it = subjectsM.begin();
        // object will be removed by removeObservedObject()
        (*it)->detachObserver(this);
    }
}
//-----------------------------------------------------------------------------
Subject* Observer::getFirstSubject()
{
    if (subjectsM.empty())
        return 0;
    return *(subjectsM.begin());
}
//-----------------------------------------------------------------------------
void Observer::addSubject(Subject* subject)
{
    subjectsM.push_back(subject);
}
//-----------------------------------------------------------------------------
void Observer::removeSubject(Subject* subject)
{
    subjectsM.erase(find(subjectsM.begin(), subjectsM.end(), subject));
}
//-----------------------------------------------------------------------------
