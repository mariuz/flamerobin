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

  Contributor(s):
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <sstream>
#include <string>

#include "database.h"
#include "collection.h"
#include "domain.h"
#include "constraints.h"
#include "ugly.h"
#include "column.h"
//------------------------------------------------------------------------------
//! new undefined column
YColumn::YColumn()
{
	typeM = ntColumn;
	parentM = 0;
}
//------------------------------------------------------------------------------
//! initialize properties
void YColumn::Init(bool notnull, std::string source, std::string collation)
{
	source.erase(source.find_last_not_of(" ")+1);		// right trim everything
	collation.erase(collation.find_last_not_of(" ")+1);
	notnullM = notnull;
	sourceM = source;
	collationM = collation;
}
//------------------------------------------------------------------------------
bool YColumn::isNullable() const
{
	return !notnullM;
}
//------------------------------------------------------------------------------
bool YColumn::isPrimaryKey() const
{
	YTable *t = dynamic_cast<YTable *>(parentM);
	if (!t)	// view/SP
		return false;
	ColumnConstraint *key = t->getPrimaryKey();
	if (!key)
		return false;
	for (ColumnConstraint::const_iterator it = key->begin(); it != key->end(); ++it)
		if ((*it) == nameM)
			return true;
	return false;
}
//------------------------------------------------------------------------------
std::string YColumn::getPrintableName() const
{
	YDomain *d = getDomain();
	std::string domain_name;
 	if (d)
		domain_name = d->getDatatypeAsString();
	else
		domain_name = sourceM;

	std::string ret = nameM + " " + domain_name;
	// OPTION: ability to show domain name together with data type.
	if (d && !d->isSystem())
		ret += " (" + d->getPrintableName() + ")";
	if (notnullM)
		ret += " not null";
	return ret;
}
//------------------------------------------------------------------------------
YDomain *YColumn::getDomain() const
{
	YDatabase *d = getDatabase();
	if (!d)
		return 0;
	for (YMetadataCollection<YDomain>::const_iterator it = d->domainsBegin(); it != d->domainsEnd(); ++it)
		if ((*it).getName() == sourceM)
			return (YDomain *)&(*it);

	// since we haven't find the domain, reload domains from database
	// possible causes: creating tables, creating new table fields, etc.
	d->loadObjects(ntDomain);

	// repeat the search
	for (YMetadataCollection<YDomain>::const_iterator it = d->domainsBegin(); it != d->domainsEnd(); ++it)
		if ((*it).getName() == sourceM)
			return (YDomain *)&(*it);

	return 0;
}
//------------------------------------------------------------------------------
std::string YColumn::getSource() const
{
	return sourceM;
}
//------------------------------------------------------------------------------
std::string YColumn::getCollation() const
{
	return collationM;
}
//------------------------------------------------------------------------------
