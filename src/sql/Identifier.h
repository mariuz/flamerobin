/*
  Copyright (c) 2004-2013 The FlameRobin Development Team

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
#ifndef FR_IDENTIFIER_H
#define FR_IDENTIFIER_H
//----------------------------------------------------------------------------
//! The purpose of this class is to abstract all the work with identifiers
//! so that we don't have to struggle with quoted identifiers all over the
//! place. If also makes matching easier (upper/lower case problems)
class Identifier
{
private:
    wxString textM;
    static bool needsQuoting(const wxString& s);
    static bool isQuoted(const wxString &s);
    static wxString& escape(wxString& s);
    static wxString& strip(wxString& s);
    static wxString& quote(wxString &s);
public:
    Identifier();
    Identifier(const wxString& source);
    void setText(const wxString& source);
    void setFromSql(const wxString& source);

    bool equals(const Identifier& rhs) const;
    bool equals(const wxString& rhs) const;
    wxString get() const;
    wxString getQuoted() const;
    static wxString userString(const wxString& s);
};
//----------------------------------------------------------------------------
#endif
