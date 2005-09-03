///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, Date class implementation
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

void IBPP::Date::Today(void)
{
	time_t systime = time(0);
	tm* loctime = localtime(&systime);

	if (! IBPP::itod(&mDate, loctime->tm_year + 1900,
		loctime->tm_mon + 1, loctime->tm_mday))
			throw LogicExceptionImpl("Date::Today", "Out of range date");
}

void IBPP::Date::SetDate(int year, int month, int day)
{
	if (! IBPP::itod(&mDate, year, month, day))
		throw LogicExceptionImpl("Date::SetDate", "Out of range date");
}

void IBPP::Date::GetDate(int& year, int& month, int& day) const
{
	if (! IBPP::dtoi(mDate, &year, &month, &day))
		throw LogicExceptionImpl("Date::GetDate", "Out of range date");
}

void IBPP::Date::Add(int days)
{
	int year;
	int newdate = mDate + days;		// days can be signed
	if (! IBPP::dtoi(newdate, &year, 0, 0))
		throw LogicExceptionImpl("Date::Add()", "Out of range date");
	mDate = newdate;
}

void IBPP::Date::StartOfMonth(void)
{
	int year, month;
	if (! IBPP::dtoi(mDate, &year, &month, 0))
		throw LogicExceptionImpl("Date::StartOfMonth()", "Out of range date");
	if (! IBPP::itod(&mDate, year, month, 1))		// First of same month
		throw LogicExceptionImpl("Date::StartOfMonth()", "Out of range date");
}

void IBPP::Date::EndOfMonth(void)
{
	int year, month;
	if (! IBPP::dtoi(mDate, &year, &month, 0))
		throw LogicExceptionImpl("Date::EndOfMonth()", "Out of range date");
	if (++month > 12) { month = 1; year++; }
	if (! IBPP::itod(&mDate, year, month, 1))	// First of next month
		throw LogicExceptionImpl("Date::EndOfMonth()", "Out of range date");
	mDate--;	// Last day of original month, all weird cases accounted for
}

IBPP::Date::Date(const int& copied)
{
	int year;
	if (! IBPP::dtoi(copied, &year, 0, 0))
		throw LogicExceptionImpl("Date::Date(int&)", "Out of range date");
	mDate = copied;
}

IBPP::Date::Date(int year, int month, int day)
{
	SetDate(year, month, day);
}

IBPP::Date::Date(const IBPP::Date& copied)
{
	mDate = copied.mDate;
}

IBPP::Date& IBPP::Date::operator=(const IBPP::Timestamp& assigned)
{
	mDate = assigned.GetDate();
	return *this;
}

IBPP::Date& IBPP::Date::operator=(const IBPP::Date& assigned)
{
	mDate = assigned.mDate;
	return *this;
}

IBPP::Date& IBPP::Date::operator=(const int& assigned)
{
	int year;
	if (! IBPP::dtoi(assigned, &year, 0, 0))
		throw LogicExceptionImpl("Date::operator=(int)", "Out of range date");
	mDate = assigned;
	return *this;
}

//	Eof
