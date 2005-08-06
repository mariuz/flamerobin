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

#include "config.h"
#include "database.h"
#include "collection.h"
#include "domain.h"
#include "constraints.h"
#include "ugly.h"
#include "visitor.h"
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
void YColumn::Init(bool notnull, std::string source, bool computed, std::string computedSource, std::string collation)
{
	source.erase(source.find_last_not_of(" ")+1);		// right trim everything
	collation.erase(collation.find_last_not_of(" ")+1);
	notnullM = notnull;
	sourceM = source;
	computedM = computed;
	computedSourceM = computedSource;
	collationM = collation;
}
//------------------------------------------------------------------------------
bool YColumn::isNullable() const
{
	return !notnullM;
}
//------------------------------------------------------------------------------
bool YColumn::isComputed() const
{
	return computedM;
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
//! retrieve datatype from domain if possible
std::string YColumn::getDatatype()
{
	enum { showType=0, showFormula, showAll };
	int flag = showFormula;
	config().getValue("ShowComputed", flag);
	// view columns are all computed and have their source empty
	if (computedM && flag == showFormula && !computedSourceM.empty())
		return computedSourceM;

	std::string ret;
	YDomain *d = getDomain();
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
//------------------------------------------------------------------------------
//! printable name = column_name + column_datatype [+ not null]
std::string YColumn::getPrintableName()
{
	std::string ret = nameM + " " + getDatatype();
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

	// since we haven't find the domain, check the database
	return d->loadMissingDomain(sourceM);
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
std::string YColumn::getDropSqlStatement() const
{
	return "ALTER TABLE " + getParent()->getName() + " DROP " + nameM;
}
//------------------------------------------------------------------------------
void YColumn::accept(Visitor *v)
{
	v->visit(*this);
}
//------------------------------------------------------------------------------
