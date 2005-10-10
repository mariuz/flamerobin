///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, DBKey class implementation
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

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace ibpp_internals;

//	Private implementation

//	Public implementation

void IBPP::DBKey::Clear(void)
{
	mDBKey.clear();
	mString.clear();
}

void IBPP::DBKey::SetKey(const void* key, int size)
{
	if (key == 0)
		throw LogicExceptionImpl("IBPP::DBKey::SetKey", _("Null DBKey reference detected."));
	if (size <= 0 || ((size >> 3) << 3) != size)
		throw LogicExceptionImpl("IBPP::DBKey::SetKey", _("Invalid DBKey size."));

	mDBKey.assign((const char*)key, (size_t)size);
	mString.clear();
}

void IBPP::DBKey::GetKey(void* key, int size) const
{
	if (mDBKey.empty())
		throw LogicExceptionImpl("IBPP::DBKey::GetKey", _("DBKey not assigned."));
	if (key == 0)
		throw LogicExceptionImpl("IBPP::DBKey::GetKey", _("Null DBKey reference detected."));
	if (size != (int)mDBKey.size())
		throw LogicExceptionImpl("IBPP::DBKey::GetKey", _("Incompatible DBKey size detected."));

	mDBKey.copy((char*)key, mDBKey.size());
}

const char* IBPP::DBKey::AsString() const
{
	if (mDBKey.empty())
		throw LogicExceptionImpl("IBPP::DBKey::GetString", _("DBKey not assigned."));

	if (mString.empty())
	{
		std::ostringstream hexkey;
		hexkey.setf(std::ios::hex, std::ios::basefield);
		hexkey.setf(std::ios::uppercase);

		const uint32_t* key = reinterpret_cast<const uint32_t*>(mDBKey.data());
		int n = (int)mDBKey.size() / 8;
		for (int i = 0; i < n; i++)
		{
			if (i != 0) hexkey<< "-";
			hexkey<< std::setw(4)<< key[i*2]<< ":";
			hexkey<< std::setw(8)<< key[i*2+1];
		}

		mString = hexkey.str();
	}

	return mString.c_str();
}

IBPP::DBKey::DBKey(const DBKey& copied)
{
	mDBKey = copied.mDBKey;
	mString = copied.mString;
}

IBPP::DBKey& IBPP::DBKey::operator=(const IBPP::DBKey& assigned)
{
	mDBKey = assigned.mDBKey;
	mString = assigned.mString;
	return *this;
}

//
//	EOF
//
