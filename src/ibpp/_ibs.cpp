///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, internal Status class implementation
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

using namespace ibpp_internals;

int IBS::SqlCode(void) const
{
	return (int)(*gds.Call()->m_sqlcode)(&mVector[0]);
}

const char* IBS::ErrorMessage(void) const
{
	char msg[1024];
	long sqlcode;

	if (! mMessage.empty()) return mMessage.c_str();	// If message compiled, returns it

	// Compiles the message (SQL part)
	std::ostringstream message;
	sqlcode = (*gds.Call()->m_sqlcode)(mVector);
	if (sqlcode != -999)
	{
		(*gds.Call()->m_sql_interprete)((short)sqlcode, msg, sizeof(msg));
		message<< "SQL Message : "<< sqlcode<< "\n"<< msg<< "\n\n";
	}

	message<< "Engine Code    : "<< EngineCode()<< "\n";

	// Compiles the message (Engine part)
	long* error = &mVector[0];
	try { (*gds.Call()->m_interprete)(msg, &error); }
	catch(...) { msg[0] = '\0'; }
	message<< "Engine Message :\n"<< msg;
	try
	{
		while ((*gds.Call()->m_interprete)(msg, &error))
			message<< "\n"<< msg;
	}
	catch (...) { }

	message<< "\n";
	mMessage = message.str();
	return mMessage.c_str();
}

void IBS::Reset(void)
{
	for (int i = 0; i < 20; i++) mVector[i] = 0;
	mMessage.erase();
}

IBS::IBS()
{
	Reset();
}

IBS::~IBS()
{
}

/** Copy Constructor
*/

IBS::IBS(IBS& copied)
{
	memcpy(mVector, copied.mVector, sizeof(mVector));
}

//
//	EOF
//
