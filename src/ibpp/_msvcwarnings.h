///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : MSVC specific file, forcing some unusefull warnings off
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

// Template generated names are often very long and MSVC generates a warning
// when compiling in debug mode if the length is greater than 255 symbols.
// This is a limitation of the Microsoft debugger/debugging info storage.
// At least as of MSVC++ 6.0. This might have improved in MSVC 7.0.
// This warning is really annoying to see, while does not carry any really
// usefull information. So we disable it.
//
// This _msvcwarnings.h file is included straight on the command-line when
// compiling, so the disabling takes effect for all units, without requiring
// any ugly platform conditionals in the source code. True, I wouldn't like
// #ifdef IBPP_MSVC
// #pragma warning(disable: 4786)
// #endif
// on the very top of every source file just to fix this compiler issue.

#pragma warning(disable: 4786)
#ifndef _DEBUG
#pragma warning(disable: 4702)
#endif

//
//	Eof
//
