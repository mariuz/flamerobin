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

void IBPP::DBKey::BlindCopy(const DBKey& copied)
{
	mSize = copied.mSize;

	if (copied.mDBKey != 0)
	{
		mDBKey = new char[mSize];
		memcpy(mDBKey, copied.mDBKey, mSize);
	}
	else mDBKey = 0;

	if (copied.mString != 0)
	{
		mString = new char[strlen(copied.mString)+1];
		strcpy(mString, copied.mString);
	}
	else mString = 0;
}

//	Public implementation

void IBPP::DBKey::Clear(void)
{
	if (mDBKey != 0) delete [] reinterpret_cast<char*>(mDBKey);
	if (mString != 0) delete [] mString;
	mDBKey = 0;
	mString = 0;
	mSize = 0;
}

void IBPP::DBKey::SetKey(const void* key, int size)
{
	if (key == 0)
		throw LogicExceptionImpl("IBPP::DBKey::SetKey", "0 DBKey reference detected.");
	if (size <= 0 || ((size >> 3) << 3) != size)
		throw LogicExceptionImpl("IBPP::DBKey::SetKey", "Invalid DBKey size.");

	if (mString != 0)
	{
		delete [] mString;
		mString = 0;
	}
	if (mDBKey != 0) delete [] reinterpret_cast<char*>(mDBKey);
	mDBKey = new char[size];
	memcpy(mDBKey, key, size);
	mSize = size;
}

void IBPP::DBKey::GetKey(void* key, int size) const
{
	if (mDBKey == 0)
		throw LogicExceptionImpl("IBPP::DBKey::GetKey", "DBKey not assigned.");
	if (key == 0)
		throw LogicExceptionImpl("IBPP::DBKey::GetKey", "0 DBKey reference detected.");
	if (size != mSize)
		throw LogicExceptionImpl("IBPP::DBKey::GetKey", "Incompatible DBKey size detected.");

	memcpy(key, mDBKey, size);
}

const char* IBPP::DBKey::AsString(void) const
{
	if (mDBKey == 0)
		throw LogicExceptionImpl("IBPP::DBKey::GetString", "DBKey not assigned.");

	if (mString == 0)
	{
		std::ostringstream hexkey;
		hexkey.setf(std::ios::hex, std::ios::basefield);
		hexkey.setf(std::ios::uppercase);

		unsigned* key = reinterpret_cast<unsigned*>(mDBKey);
		int n = mSize / 8;
		for (int i = 0; i < n; i++)
		{
			if (i != 0) hexkey<< "-";
			hexkey<< std::setw(4)<< key[i*2]<< ":";
			hexkey<< std::setw(8)<< key[i*2+1];
		}

		size_t len = hexkey.str().size();
		mString = new char[len+1];
		hexkey.str().copy(mString, len);
		mString[len] = 0;
	}

	return mString;
}

IBPP::DBKey::DBKey()
{
	mDBKey = 0;
	mString = 0;
	mSize = 0;
}

IBPP::DBKey::DBKey(const DBKey& copied)
{
	BlindCopy(copied);
}

IBPP::DBKey& IBPP::DBKey::operator=(const IBPP::DBKey& assigned)
{
	if (mDBKey != 0) delete [] reinterpret_cast<char*>(mDBKey);
	if (mString != 0) delete [] mString;
	BlindCopy(assigned);
	return *this;
}

IBPP::DBKey::~DBKey()
{
	if (mDBKey != 0) delete [] reinterpret_cast<char*>(mDBKey);
	if (mString != 0) delete [] mString;
}

//
//	EOF
//
