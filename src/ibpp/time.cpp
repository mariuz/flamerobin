///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, Time class implementation
//
///////////////////////////////////////////////////////////////////////////////
//
//	The contents of this file are subject to the Mozilla Public License
//	Version 1.0 (the "License"); you may not use this file except in
//	compliance with the License. You may obtain a copy of the License at
//	http://www.mozilla.org/MPL/
//
//	Software distributed under the License is distributed on an "AS IS"
//	basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//	License for the specific language governing rights and limitations
//	under the License.
//
//	The Original Code is "IBPP 0.9" and all its associated documentation.
//
//	The Initial Developer of the Original Code is T.I.P. Group S.A.
//	Portions created by T.I.P. Group S.A. are
//	Copyright (C) 2000 T.I.P Group S.A.
//	All Rights Reserved.
//
//	Contributor(s): ______________________________________.
//
///////////////////////////////////////////////////////////////////////////////
//
//	COMMENTS
//	* Tabulations should be set every four characters when editing this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "ibpp.h"
#include "_internals.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <time.h>		// Can't use <ctime> thanks to MSVC6 buggy library

using namespace ibpp_internals;

void IBPP::Time::Now(void)
{
	time_t systime = time(0);
	tm* loctime = localtime(&systime);
	IBPP::itot(&mTime, loctime->tm_hour, loctime->tm_min, loctime->tm_sec);
}

void IBPP::Time::SetTime(int32_t hour, int32_t minute, int32_t second)
{
	if (hour < 0 || hour > 23 ||
		minute < 0 || minute > 59 ||
			second < 0 || second > 59)
				throw LogicExceptionImpl("Time::SetTime",
					_("Invalid hour, minute, second values"));
	IBPP::itot(&mTime, hour, minute, second);
}

void IBPP::Time::GetTime(int32_t& hour, int32_t& minute, int32_t& second) const
{
	IBPP::ttoi(mTime, &hour, &minute, &second);
}

IBPP::Time::Time(int32_t hour, int32_t minute, int32_t second)
{
	SetTime(hour, minute, second);
}

IBPP::Time::Time(const IBPP::Time& copied)
{
	mTime = copied.mTime;
}

IBPP::Time::Time(const int32_t& copied)
{
	if (copied < 0 || copied > 86399) throw LogicExceptionImpl("Time::Time(int&)", _("Invalid time value"));
	mTime = copied;
}

IBPP::Time& IBPP::Time::operator=(const IBPP::Timestamp& assigned)
{
	mTime = assigned.GetTime();
	return *this;
}

IBPP::Time& IBPP::Time::operator=(const IBPP::Time& assigned)
{
	mTime = assigned.mTime;
	return *this;
}

IBPP::Time& IBPP::Time::operator=(const int32_t& assigned)
{
	if (assigned < 0 || assigned > 86399)
		throw LogicExceptionImpl("Time::operator=(int)", _("Invalid time value"));
	mTime = assigned;
	return *this;
}

//
//	EOF
//

