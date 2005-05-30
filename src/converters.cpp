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

#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "config.h"
#include "converters.h"
//---------------------------------------------------------------------------------------
//! formats date according to DateFormat string
std::string GetHumanDate(int year, int month, int day, std::string DateFormat)
{
	std::string value;
	for (std::string::iterator c = DateFormat.begin(); c != DateFormat.end(); c++)
	{
		int var = -1;
		std::string format = "%02d";
		switch (*c)
		{
			case 'd':	format = "%d";
			case 'D':	var = day;			break;
			case 'm':	format = "%d";
			case 'M':	var = month;		break;
			case 'Y':	format = "%04d";
			case 'y':	var = year;			break;
			default:	value += *c;
		}

		if (*c == 'y')
			var %= 100;

		if (var != -1)
		{
			char str[10];
			sprintf(str, format.c_str(), var);
			value += str;
		}
	}
	return value;
}
//---------------------------------------------------------------------------------------
//! formats time according to TimeFormat string
std::string GetHumanTime(int hour, int minute, int second, std::string TimeFormat)
{
	std::string value;
	for (std::string::iterator c = TimeFormat.begin(); c != TimeFormat.end(); c++)
	{
		int var = -1;
		std::string format = "%02d";
		switch (*c)
		{
			case 'h':	format = "%d";
			case 'H':	var = hour;			break;
			case 'm':	format = "%d";
			case 'M':	var = minute;		break;
			case 's':	format = "%d";
			case 'S':	var = second;		break;
			default:	value += *c;
		}

		if (var != -1)
		{
			char str[10];
			sprintf(str, format.c_str(), var);
			value += str;
		}
	}
	return value;
}
//---------------------------------------------------------------------------------------
//! formats timestamp according to DateFormat and TimeFormat strings
std::string GetHumanTimestamp(IBPP::Timestamp ts, std::string DateFormat, std::string TimeFormat)
{
	int year, month, day, hour, minute, second;
	ts.GetDate(year, month, day);
	ts.GetTime(hour, minute, second);

	std::string value = GetHumanTime(hour, minute, second, TimeFormat);
	if (value != "")
		value = " " + value;
	return GetHumanDate(year, month, day, DateFormat) + value;
}
//---------------------------------------------------------------------------------------
//! takes value from column "col" of Statement and converts it into string
// returns false if value is null
bool CreateString(IBPP::Statement& st, int col, std::string& value)
{
	std::ostringstream svalue;

	if (st->IsNull(col))
		return false;

	// some defaults
	bool reformatNumbers = config().get("ReformatNumbers", false);
	int numberPrecision = config().get("NumberPrecision", 2);	// defaults to 2 decimal digits
	std::string dateFormat = config().get("DateFormat", std::string("D.M.Y"));
	std::string timeFormat = config().get("TimeFormat", std::string("H:M:S"));

	double dval;
	float fval;
	__int64 int64val;
	int x;
	IBPP::Date d;
	IBPP::Time t;
	IBPP::Timestamp ts;
	int year, month, day, hour, minute, second;

	IBPP::SDT DataType = st->ColumnType(col);
	if (st->ColumnScale(col))
		DataType = IBPP::sdDouble;

	value = "";
	switch (DataType)
	{
		case IBPP::sdString:
			st->Get(col, value);
			return true;
		case IBPP::sdInteger:
		case IBPP::sdSmallint:
			st->Get(col, &x);
			svalue << x;
			value = svalue.str();
			return true;
		case IBPP::sdDate:
			st->Get(col, d);
			dtoi(d, &year, &month, &day);
			value = GetHumanDate(year, month, day, dateFormat);
			return true;
		case IBPP::sdTime:
			st->Get(col, t);
			ttoi(t, &hour, &minute, &second);
			value = GetHumanTime(hour, minute, second, timeFormat);
			return true;
		case IBPP::sdTimestamp:
			st->Get(col, ts);
			value = GetHumanTimestamp(ts, dateFormat, timeFormat);
			return true;
		case IBPP::sdFloat:
			st->Get(col, &fval);
			svalue << std::fixed << fval;
			value = svalue.str();
			return true;
		case IBPP::sdDouble:
			st->Get(col, &dval);
			if (st->ColumnScale(col))
				svalue << std::fixed << std::setprecision(st->ColumnScale(col)) << dval;
			else
			{
				if (reformatNumbers)
					svalue << std::fixed << std::setprecision(numberPrecision) << dval;
				else
					svalue << std::fixed << dval;
			}
			value = svalue.str();
			return true;

		case IBPP::sdLargeint:
			st->Get(col, &int64val);
			svalue << int64val;
			value = svalue.str();
			return true;

			//sprintf(str, INT64FORMAT, int64val);
			//value = str;
			//return value;

		default:
			value = "[...]";
			return true;
	};

	//return true;
}
//---------------------------------------------------------------------------------------
