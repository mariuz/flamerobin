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

  Contributor(s): Milan Babuskov
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

//
//
//
//
//------------------------------------------------------------------------------
#include <list>
#include <algorithm>
#include "subject.h"
#include "observer.h"

//------------------------------------------------------------------------------
YxSubject::YxSubject()
{
	locksCountM = 0;
}
//------------------------------------------------------------------------------
YxSubject::~YxSubject()
{
	detachAllObservers();
}
//------------------------------------------------------------------------------
void YxSubject::attach(YxObserver* observer)
{
    if(observer && std::find(observersM.begin(), observersM.end(), observer)
            == observersM.end())
	{
		observer->addObservedObject(this);
        observersM.push_back(observer);
	}
}
//------------------------------------------------------------------------------
void YxSubject::detach(YxObserver* observer)
{
    if(!observer)
		return;

	observer->removeObservedObject(this);
	observersM.erase(std::find(observersM.begin(), observersM.end(), observer));

	if(!observersM.size())
		locksCountM = 0;
}
//------------------------------------------------------------------------------
void YxSubject::detachAllObservers()
{
	for (std::list<YxObserver *>::iterator i = observersM.begin(); i != observersM.end(); ++i)
		(*i)->removeObservedObject(this);
	observersM.clear();
	locksCountM = 0;
}
//------------------------------------------------------------------------------
void YxSubject::notify()
{
    if(!locksCountM)
        for(std::list<YxObserver *>::iterator it = observersM.begin(); it != observersM.end(); ++it)
            (*it)->update();
}
//------------------------------------------------------------------------------
void YxSubject::lockSubject()
{
    ++locksCountM;
}
//------------------------------------------------------------------------------
void YxSubject::unlockSubject(bool wantFullUnlock, bool doNotify)
{
    if(!locksCountM)
		return;

    if(wantFullUnlock)
		locksCountM = 0;
    else
		--locksCountM;

	if (doNotify)
		notify();
}
//------------------------------------------------------------------------------
