///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, date calculations helper functions
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
//	* The following date calculations were inspired by web pages found on
//	  Peter Baum web homepage at 'http://www.capecod.net/~pbaum/'.
//	  His contact info is at : 'http://home.capecod.net/~pbaum/contact.htm'.
//	  Please, understand that Peter Baum is not related to this IBPP project.
//	  So __please__, do not contact him regarding IBPP matters.
//
///////////////////////////////////////////////////////////////////////////////

#include "ibpp.h"

namespace
{
	const int Dec31_1899 = 693595;
};

//	Take a date, in its integer format as used in IBPP internals and splits
//	it in year (4 digits), month (1-12), day (1-31)

bool IBPP::dtoi (int date, int *y, int *m, int *d)
{
    int RataDie, Z, H, A, B, C;
    int year, month, day;

	// Validity control.
	if (date < IBPP::MinDate || date > IBPP::MaxDate)
		return false;

	// The "Rata Die" is the date specified as the number of days elapsed since
	// 31 Dec of year 0. So 1 Jan 0001 is 1.

    RataDie = date + Dec31_1899;	// Correction, IBPP uses the '0' on 31 Dec 1899.

    Z = RataDie + 306;
    H = 100*Z - 25;
    A = H/3652425;
    B = A - A/4;
    year = (100*B + H) / 36525;
    C = B + Z - 365*year - year / 4;
    month = (5*C + 456) / 153;
    day = C - (153*month - 457) / 5;
    if (month > 12) { year += 1; month -= 12; }

	if (y != 0) *y = year;
	if (m != 0) *m = month;
	if (d != 0) *d = day;

	return true;
}

//	Take a date from its components year, month, day and convert it to the
//	integer representation used internally in IBPP.

bool IBPP::itod (int *pdate, int year, int month, int day)
{
    int RataDie, result;
	int y, m, d;

	d = day;	m = month;	y = year;
    if (m < 3) { m += 12; y -= 1; }
    RataDie = d + (153*m - 457) / 5 + 365*y + y/4 - y/100 + y/400 - 306;

    result = RataDie - Dec31_1899;   // Correction, IBPP '0' is 31 Dec 1899

	// Validity control
	if (result < IBPP::MinDate || result > IBPP::MaxDate)
		return false;

	*pdate = result;
	return true;
}

//	Time calculations. Internal format is the number of seconds elapsed since
//	midnight. Splits such a time in its hours, minutes, seconds components.

void IBPP::ttoi (int itime, int *h, int *m, int *s)
{
	int hh, mm, ss;

	hh = (int) (itime / 3600L);		itime = itime - (hh) * 3600L;
	mm = (int) (itime / 60L);
	ss = (int) (itime - (mm) * 60L);

	if (h != 0) *h = hh;
	if (m != 0) *m = mm;
	if (s != 0) *s = ss;

	return;
}

//	Get the internal time format, given hour, minute, second.

void IBPP::itot (int *ptime, int hour, int minute, int second)
{
	*ptime = hour * 3600 + minute * 60 + second;
	return;
}

//
//	Eof
//
