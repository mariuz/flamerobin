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

  Contributor(s): Nando Dessena
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <sstream>
#include <string>

#include "collection.h"
#include "column.h"
#include "config/Config.h"
#include "constraints.h"
#include "database.h"
#include "domain.h"
#include "MetadataItemVisitor.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
//! new undefined column
Column::Column()
    : MetadataItem()
{
	typeM = ntColumn;
}
//-----------------------------------------------------------------------------
//! initialize properties
void Column::Init(bool notnull, std::string source, bool computed, std::string computedSource, std::string collation)
{
	source.erase(source.find_last_not_of(" ")+1);		// right trim everything
	collation.erase(collation.find_last_not_of(" ")+1);
	notnullM = notnull;
	sourceM = source;
	computedM = computed;
	computedSourceM = computedSource;
	collationM = collation;
}
//-----------------------------------------------------------------------------
bool Column::isNullable() const
{
	return !notnullM;
}
//-----------------------------------------------------------------------------
bool Column::isComputed() const
{
	return computedM;
}
//-----------------------------------------------------------------------------
bool Column::isPrimaryKey() const
{
	Table *t = dynamic_cast<Table *>(getParent());
	if (!t)	// view/SP
		return false;
	ColumnConstraint *key = t->getPrimaryKey();
	if (!key)
		return false;
	for (ColumnConstraint::const_iterator it = key->begin(); it != key->end(); ++it)
		if ((*it) == getName())
			return true;
	return false;
}
//-----------------------------------------------------------------------------
//! retrieve datatype from domain if possible
std::string Column::getDatatype()
{
	enum { showType=0, showFormula, showAll };
	int flag = showFormula;
	config().getValue("ShowComputed", flag);
	// view columns are all computed and have their source empty
	if (computedM && flag == showFormula && !computedSourceM.empty())
		return computedSourceM;

	std::string ret;
	Domain *d = getDomain();
	std::string datatype;
 	if (d)
		datatype = d->getDatatypeAsString();
	else
		datatype = sourceM;

	enum { showDatatype=0, showDomain, showBoth };
	int show = showBoth;
 	config().getValue("ShowDomains", show);

	if (!d || d->isSystem() || show == showBoth || show == showDatatype)
		ret += datatype;

	if (d && !d->isSystem() && (show == showBoth || show == showDomain))
	{
		if (!ret.empty())
			ret += " ";
		ret += "(" + d->getName() + ")";
	}

	if (computedM && flag == showAll && !computedSourceM.empty())
		ret += " (" + computedSourceM + ")";
	return ret;
}
//-----------------------------------------------------------------------------
//! printable name = column_name + column_datatype [+ not null]
std::string Column::getPrintableName()
{
	std::string ret = getName() + " " + getDatatype();
	if (notnullM)
		ret += " not null";
	return ret;
}
//-----------------------------------------------------------------------------
Domain *Column::getDomain() const
{
	Database *d = getDatabase();
	if (!d)
		return 0;
	for (MetadataCollection<Domain>::const_iterator it = d->domainsBegin(); it != d->domainsEnd(); ++it)
		if ((*it).getName() == sourceM)
			return (Domain *)&(*it);

	// since we haven't find the domain, check the database
	return d->loadMissingDomain(sourceM);
}
//-----------------------------------------------------------------------------
std::string Column::getSource() const
{
	return sourceM;
}
//-----------------------------------------------------------------------------
std::string Column::getCollation() const
{
	return collationM;
}
//-----------------------------------------------------------------------------
std::string Column::getDropSqlStatement() const
{
	return "ALTER TABLE " + getParent()->getName() + " DROP " + getName();
}
//-----------------------------------------------------------------------------
void Column::acceptVisitor(MetadataItemVisitor* visitor)
{
	visitor->visit(*this);
}
//-----------------------------------------------------------------------------
