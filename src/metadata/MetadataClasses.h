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


  $Id$

*/

//-----------------------------------------------------------------------------
#ifndef FR_METADATACLASSES_H
#define FR_METADATACLASSES_H

#include <vector>

#include <boost/shared_ptr.hpp>

class Column;
typedef boost::shared_ptr<Column> ColumnPtr;
typedef std::vector<ColumnPtr> ColumnPtrs;

class Database;
typedef boost::shared_ptr<Database> DatabasePtr;
typedef boost::weak_ptr<Database> DatabaseWeakPtr;
typedef std::vector<DatabasePtr> DatabasePtrs;

class Domain;
typedef boost::shared_ptr<Domain> DomainPtr;
typedef std::vector<DomainPtr> DomainPtrs;
class Domains;
typedef boost::shared_ptr<Domains> DomainsPtr;

class Exception;
typedef boost::shared_ptr<Exception> ExceptionPtr;
typedef std::vector<ExceptionPtr> ExceptionPtrs;
class Exceptions;
typedef boost::shared_ptr<Exceptions> ExceptionsPtr;

class ForeignKey;

class Function;
typedef boost::shared_ptr<Function> FunctionPtr;
typedef std::vector<FunctionPtr> FunctionPtrs;
class Functions;
typedef boost::shared_ptr<Functions> FunctionsPtr;

class Generator;
typedef boost::shared_ptr<Generator> GeneratorPtr;
typedef std::vector<GeneratorPtr> GeneratorPtrs;
class Generators;
typedef boost::shared_ptr<Generators> GeneratorsPtr;

class Index;

class MetadataItem;
typedef boost::shared_ptr<MetadataItem> MetadataItemPtr;

class Parameter;
typedef boost::shared_ptr<Parameter> ParameterPtr;
typedef std::vector<ParameterPtr> ParameterPtrs;

class PrimaryKeyConstraint;

class Procedure;
typedef boost::shared_ptr<Procedure> ProcedurePtr;
typedef std::vector<ProcedurePtr> ProcedurePtrs;
class Procedures;
typedef boost::shared_ptr<Procedures> ProceduresPtr;

class Relation;

class Role;
typedef boost::shared_ptr<Role> RolePtr;
typedef std::vector<RolePtr> RolePtrs;
class Roles;
typedef boost::shared_ptr<Roles> RolesPtr;

class Root;
typedef boost::shared_ptr<Root> RootPtr;

class Server;
typedef boost::shared_ptr<Server> ServerPtr;
typedef boost::weak_ptr<Server> ServerWeakPtr;
typedef std::vector<ServerPtr> ServerPtrs;

class SysDomains;
typedef boost::shared_ptr<SysDomains> SysDomainsPtr;

class SysRoles;
typedef boost::shared_ptr<SysRoles> SysRolesPtr;

class SysTables;
typedef boost::shared_ptr<SysTables> SysTablesPtr;

class Table;
typedef boost::shared_ptr<Table> TablePtr;
typedef std::vector<TablePtr> TablePtrs;
class Tables;
typedef boost::shared_ptr<Tables> TablesPtr;

class Trigger;
typedef boost::shared_ptr<Trigger> TriggerPtr;
typedef std::vector<TriggerPtr> TriggerPtrs;
class Triggers;
typedef boost::shared_ptr<Triggers> TriggersPtr;

class UniqueConstraint;

class User;
typedef boost::shared_ptr<User> UserPtr;
typedef std::vector<UserPtr> UserPtrs;

class View;
typedef boost::shared_ptr<View> ViewPtr;
typedef std::vector<ViewPtr> ViewPtrs;
class Views;
typedef boost::shared_ptr<Views> ViewsPtr;
//-----------------------------------------------------------------------------
#endif // FR_METADATACLASSES_H
