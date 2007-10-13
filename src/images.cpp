/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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


  $Id$

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

#include "core/ArtProvider.h"
#include "metadata/metadataitem.h"
//-----------------------------------------------------------------------------
wxBitmap getImage(NodeType type)
{
    wxSize sz(16, 16);
    switch (type)
    {
        case ntColumn:
            return wxArtProvider::GetIcon(ART_Column, wxART_OTHER, sz);
        case ntComputed:
            return wxArtProvider::GetIcon(ART_Function, wxART_OTHER, sz);
        case ntDatabase:
            return wxArtProvider::GetIcon(ART_Database, wxART_OTHER, sz);
        case ntDomain:
        case ntDomains:
            return wxArtProvider::GetIcon(ART_Domain, wxART_OTHER, sz);
        case ntFunction:
        case ntFunctions:
            return wxArtProvider::GetIcon(ART_Function, wxART_OTHER, sz);
        case ntGenerator:
            return wxArtProvider::GetIcon(ART_Generator, wxART_OTHER, sz);
        case ntGenerators:
            return wxArtProvider::GetIcon(ART_Generators, wxART_OTHER, sz);
        case ntParameter:
            return wxArtProvider::GetIcon(ART_Column, wxART_OTHER, sz);
        case ntPrimaryKey:
            return wxArtProvider::GetIcon(ART_PrimaryKey, wxART_OTHER, sz);
        case ntProcedure:
            return wxArtProvider::GetIcon(ART_Procedure, wxART_OTHER, sz);
        case ntProcedures:
            return wxArtProvider::GetIcon(ART_Procedures, wxART_OTHER, sz);
        case ntRoot:
            return wxArtProvider::GetIcon(ART_Root, wxART_OTHER, sz);
        case ntServer:
            return wxArtProvider::GetIcon(ART_Server, wxART_OTHER, sz);
        case ntSysTable:
            return wxArtProvider::GetIcon(ART_SystemTable, wxART_OTHER, sz);
        case ntSysTables:
            return wxArtProvider::GetIcon(ART_SystemTables, wxART_OTHER, sz);
        case ntTable:
            return wxArtProvider::GetIcon(ART_Table, wxART_OTHER, sz);
        case ntTables:
            return wxArtProvider::GetIcon(ART_Tables, wxART_OTHER, sz);
        case ntTrigger:
        case ntTriggers:
            return wxArtProvider::GetIcon(ART_Trigger, wxART_OTHER, sz);
        case ntView:
        case ntViews:
            return wxArtProvider::GetIcon(ART_View, wxART_OTHER, sz);
    }
    return wxArtProvider::GetIcon(ART_Object, wxART_OTHER, sz);
}
//-----------------------------------------------------------------------------
wxBitmap getImage32(NodeType type)
{
    wxSize sz(32, 32);
    switch (type)
    {
        case ntColumn:
            return wxArtProvider::GetIcon(ART_Column, wxART_OTHER, sz);
        case ntDatabase:
            return wxArtProvider::GetIcon(ART_Database, wxART_OTHER, sz);
        case ntDomain:
            return wxArtProvider::GetIcon(ART_Domain, wxART_OTHER, sz);
        case ntFunction:
            return wxArtProvider::GetIcon(ART_Function, wxART_OTHER, sz);
        case ntGenerator:
            return wxArtProvider::GetIcon(ART_Generator, wxART_OTHER, sz);
        case ntProcedure:
            return wxArtProvider::GetIcon(ART_Procedure, wxART_OTHER, sz);
        case ntServer:
            return wxArtProvider::GetIcon(ART_Server, wxART_OTHER, sz);
        case ntSysTable:
            return wxArtProvider::GetIcon(ART_SystemTable, wxART_OTHER, sz);
        case ntTable:
            return wxArtProvider::GetIcon(ART_Table, wxART_OTHER, sz);
        case ntTrigger:
            return wxArtProvider::GetIcon(ART_Trigger, wxART_OTHER, sz);
        case ntView:
            return wxArtProvider::GetIcon(ART_View, wxART_OTHER, sz);
    }
    return wxArtProvider::GetIcon(ART_FlameRobin, wxART_OTHER, sz);
}
//-----------------------------------------------------------------------------
