///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, internal TPB class implementation
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
//	* TPB == Transaction Parameter Block/Buffer, see Interbase 6.0 C-API
//	* Tabulations should be set every four characters when editing this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "ibpp.h"
#include "_internals.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;

namespace {
	const int BUFFERINCR = 128;
};

void TPB::Grow(int needed)
{
	if (mBuffer == 0) ++needed;	// Initial alloc will require one more byte
	if ((mSize + needed) > mAlloc)
	{
		// We need to grow the buffer. We use increments of BUFFERINCR bytes.
		needed = (needed / BUFFERINCR + 1) * BUFFERINCR;
		char* newbuffer = new char[mAlloc + needed];
		if (mBuffer == 0)
		{
			// Initial allocation, initialize the version tag
			newbuffer[0] = isc_tpb_version3;
			mSize = 1;
		}
		else
		{
			// Move the old buffer content to the new one
			memcpy(newbuffer, mBuffer, mSize);
			delete [] mBuffer;
		}
		mBuffer = newbuffer;
		mAlloc += needed;
	}
}

void TPB::Insert(char item)
{
	Grow(1);
	mBuffer[mSize++] = item;
}

void TPB::Insert(const std::string& data)
{
	int len = (int)data.length();
	Grow(1 + len);
	mBuffer[mSize++] = (char)len;
	strncpy(&mBuffer[mSize], data.c_str(), len);
	mSize += len;
}

void TPB::Reset()
{
	if (mSize != 0)
	{
		delete [] mBuffer;
		mBuffer = 0;
		mSize = 0;
		mAlloc = 0;
	}
}

//
//	EOF
//
