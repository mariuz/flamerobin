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

  $Id$

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

#include "metadata/metadataitem.h"
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
    #include "table.xpm"
    #include "tables.xpm"
    #include "trigger.xpm"
    #include "view.xpm"
    #include "key.xpm"

    char ***map = new char**[ntLastType];
    for (int i = 0; i < ntLastType; i++)
        map[i] = object_xpm;

    map[ntUnknown]         = object_xpm;
    map[ntRoot]         = root_xpm;
    map[ntServer]         = server_xpm;
    map[ntDatabase]     = database_xpm;
    map[ntTable]         = table_xpm;
    map[ntTables]         = tables_xpm;
    map[ntView]         = view_xpm;
    map[ntViews]         = view_xpm;
    map[ntProcedure]     = procedure_xpm;
    map[ntProcedures]     = procedures_xpm;
    map[ntGenerator]     = generator_xpm;
    map[ntFunction]     = function_xpm;
    map[ntSysTable]     = table_xpm;
    map[ntTrigger]         = trigger_xpm;
    map[ntTriggers]     = trigger_xpm;
    map[ntGenerators]     = generators_xpm;
    map[ntFunctions]     = function_xpm;
    map[ntSysTables]     = tables_xpm;
    map[ntColumn]         = column_xpm;
    map[ntDomains]         = domain_xpm;
    //map[ntRole]         = role_xpm;
    //map[ntRoles]         = roles_xpm;
    map[ntDomain]         = domain_xpm;
    map[ntParameter]     = column_xpm;
    map[ntPrimaryKey]    = key_xpm;
    map[ntComputed]     = function_xpm;

    wxBitmap ret(map[type]);

    delete [] map;
    return ret;
}
//-----------------------------------------------------------------------------
wxBitmap getImage32(NodeType type)
{
    // default image
    #include "fricon.xpm"

    #include "column32.xpm"
    #include "domain32.xpm"
    #include "function32.xpm"
    #include "generator32.xpm"
    #include "procedure32.xpm"
    #include "table32.xpm"
    #include "trigger32.xpm"
    #include "view32.xpm"

    char ***map = new char**[ntLastType];
    for (int i = 0; i < ntLastType; i++)
        map[i] = fricon_xpm;

    map[ntUnknown]         = fricon_xpm;
    map[ntTable]         = table_xpm;
    map[ntView]         = view_xpm;
    map[ntProcedure]     = procedure_xpm;
    map[ntGenerator]     = generator_xpm;
    map[ntFunction]     = function_xpm;
    map[ntSysTable]     = table_xpm;
    map[ntTrigger]         = trigger_xpm;
    map[ntColumn]         = column_xpm;
    map[ntDomain]         = domain_xpm;
    wxBitmap ret(map[type]);

    delete [] map;
    return ret;
}
//-----------------------------------------------------------------------------
