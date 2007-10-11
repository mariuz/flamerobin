/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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

#include "metadata/metadataitem.h"
//-----------------------------------------------------------------------------
typedef const char* xpm_t;
//-----------------------------------------------------------------------------
wxBitmap getImage(NodeType type)
{
    #include "object.xpm"
    #include "column.xpm"
    #include "database.xpm"
    #include "domain.xpm"
    //#include "images/folder.xpm" // unused
    //#include "images/foldero.xpm" // unused
    #include "function.xpm"
    #include "generator.xpm"
    #include "generators.xpm"
    #include "procedure.xpm"
    #include "procedures.xpm"
    #include "root.xpm"
    #include "server.xpm"
    #include "systemtable.xpm"
    #include "systemtables.xpm"
    #include "table.xpm"
    #include "tables.xpm"
    #include "trigger.xpm"
    #include "view.xpm"
    #include "key.xpm"

    xpm_t** map = new xpm_t*[ntLastType];
    for (int i = 0; i < ntLastType; i++)
        map[i] = object_xpm;

    map[ntUnknown]      = object_xpm;
    map[ntRoot]         = root_xpm;
    map[ntServer]       = server_xpm;
    map[ntDatabase]     = database_xpm;
    map[ntTable]        = table_xpm;
    map[ntTables]       = tables_xpm;
    map[ntSysTable]     = systemtable_xpm;
    map[ntSysTables]    = systemtables_xpm;
    map[ntView]         = view_xpm;
    map[ntViews]        = view_xpm;
    map[ntProcedure]    = procedure_xpm;
    map[ntProcedures]   = procedures_xpm;
    map[ntGenerator]    = generator_xpm;
    map[ntFunction]     = function_xpm;
    map[ntTrigger]      = trigger_xpm;
    map[ntTriggers]     = trigger_xpm;
    map[ntGenerators]   = generators_xpm;
    map[ntFunctions]    = function_xpm;
    map[ntColumn]       = column_xpm;
    map[ntDomains]      = domain_xpm;
    //map[ntRole]       = role_xpm;
    //map[ntRoles]      = roles_xpm;
    map[ntDomain]       = domain_xpm;
    map[ntParameter]    = column_xpm;
    map[ntPrimaryKey]   = key_xpm;
    map[ntComputed]     = function_xpm;

    wxBitmap ret(map[type]);

    delete [] map;
    return ret;
}
//-----------------------------------------------------------------------------
wxBitmap getImage32(NodeType type)
{
    // default image
    #include "flamerobin.xpm"

    #include "column32.xpm"
    #include "database32.xpm"
    #include "domain32.xpm"
    #include "function32.xpm"
    #include "generator32.xpm"
    #include "procedure32.xpm"
    #include "server32.xpm"
    #include "systemtable32.xpm"
    #include "table32.xpm"
    #include "trigger32.xpm"
    #include "view32.xpm"

    xpm_t** map = new xpm_t*[ntLastType];
    for (int i = 0; i < ntLastType; i++)
        map[i] = flamerobin_xpm;

    map[ntUnknown]      = flamerobin_xpm;
    map[ntTable]        = table_xpm;
    map[ntSysTable]     = systemtable32_xpm;
    map[ntView]         = view_xpm;
    map[ntProcedure]    = procedure_xpm;
    map[ntGenerator]    = generator_xpm;
    map[ntFunction]     = function_xpm;
    map[ntTrigger]      = trigger_xpm;
    map[ntColumn]       = column_xpm;
    map[ntDomain]       = domain_xpm;
    map[ntDatabase]     = database32_xpm;
    map[ntServer]       = server32_xpm;
    wxBitmap ret(map[type]);

    delete [] map;
    return ret;
}
//-----------------------------------------------------------------------------
