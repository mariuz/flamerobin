/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef FR_STRINGUTILS_H
#define FR_STRINGUTILS_H

#include <wx/filename.h>
#include <wx/string.h>

#include <string>


std::string wx2std(const wxString& input, wxMBConv* conv = wxConvCurrent);

wxString std2wxIdentifier(const std::string& input, wxMBConv* conv);

// Converts chars that have special meaning in HTML or XML, so they get
// displayed.
wxString escapeHtmlChars(const wxString& input, bool processNewlines = true);
wxString escapeXmlChars(const wxString& input);

// Returns string suitable for HTML META charset tag (used only if no
// conversion to UTF-8 is available, i.e. in non-Unicode build.
wxString getHtmlCharset();

// Standard way to confert a boolean to a string ("true"/"false").
inline wxString getBooleanAsString(bool value) { return value ? "true" : "false"; }
// Standard way to confert a string ("true"/"false") to a boolean.
inline bool getStringAsBoolean(wxString value) { return value == "true" ? true : false; }

// Converts a wxArrayString to a delimited string of values.
wxString wxArrayToString(const wxArrayString& arrayStr, const wxString& delimiter);

//! loads the file into wxString
wxString loadEntireFile(const wxFileName& filename);

//! wraps <text> into lines of maximum <maxWidth> characters, inserting a line
//  break after each line. All lines after the first are also indented by
//  <indent> spaces.
//  Code adapted from wxWidgets' wxTextWrapper function.
wxString wrapText(const wxString& text, size_t maxWidth, size_t indent);

#endif // FR_STRINGUTILS_H
