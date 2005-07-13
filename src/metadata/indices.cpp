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
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  Contributor(s):
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <string>
#include <vector>
#include "metadataitem.h"
#include "indices.h"
//------------------------------------------------------------------------------
Index::Index(bool unique, bool active, bool ascending, double statistics)
{
	typeM = ntIndex;
	uniqueFlagM = unique;
	activeM = active;
	indexTypeM = (ascending ? itAscending : itDescending);
	statisticsM = statistics;
}
//------------------------------------------------------------------------------
bool Index::isActive()
{
	return activeM;
}
//------------------------------------------------------------------------------
bool Index::isUnique()
{
	return uniqueFlagM;
}
//------------------------------------------------------------------------------
double Index::getStatistics()
{
	return statisticsM;
}
//------------------------------------------------------------------------------
std::vector<std::string> *Index::getSegments()
{
	return &segmentsM;
}
//------------------------------------------------------------------------------
std::string Index::getFieldsAsString()
{
	std::string retval;
	for (std::vector<std::string>::iterator it = segmentsM.begin(); it != segmentsM.end(); ++it)
	{
		if (!retval.empty())
			retval += ",";
		retval += (*it);
	}
	return retval;
}
//------------------------------------------------------------------------------
Index::IndexType Index::getIndexType()
{
	return indexTypeM;
}
//------------------------------------------------------------------------------
