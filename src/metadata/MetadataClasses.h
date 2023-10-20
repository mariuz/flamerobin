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


#ifndef FR_METADATACLASSES_H
#define FR_METADATACLASSES_H

#include <vector>
#include <memory>

class CharacterSet;
typedef std::shared_ptr<CharacterSet> CharacterSetPtr;
typedef std::vector<CharacterSetPtr> CharacterSetPtrs;
class CharacterSets;
typedef std::shared_ptr<CharacterSets> CharacterSetsPtr;

class Collation;
typedef std::shared_ptr<Collation> CollationPtr;
typedef std::vector<CollationPtr> CollationPtrs;
class Collations;
typedef std::shared_ptr<Collations> CollationsPtr;
class SysCollations;
typedef std::shared_ptr<SysCollations> SysCollationsPtr;


class Column;
typedef std::shared_ptr<Column> ColumnPtr;
typedef std::vector<ColumnPtr> ColumnPtrs;

class Database;
typedef std::shared_ptr<Database> DatabasePtr;
typedef std::weak_ptr<Database> DatabaseWeakPtr;
typedef std::vector<DatabasePtr> DatabasePtrs;

class Trigger;
typedef std::shared_ptr<Trigger> TriggerPtr;
typedef std::vector<TriggerPtr> TriggerPtrs;
class Triggers;
typedef std::shared_ptr<Triggers> TriggersPtr;

class DBTrigger;
typedef std::shared_ptr<DBTrigger> DBTriggerPtr;
typedef std::vector<DBTriggerPtr> DBTriggerPtrs;
class DBTriggers;
typedef std::shared_ptr<DBTriggers> DBTriggersPtr;

class DDLTrigger;
typedef std::shared_ptr<DDLTrigger> DDLTriggerPtr;
typedef std::vector<DDLTriggerPtr> DDLTriggerPtrs;
class DDLTriggers;
typedef std::shared_ptr<DDLTriggers> DDLTriggersPtr;

class DMLTrigger;
typedef std::shared_ptr<DMLTrigger> DMLTriggerPtr;
typedef std::vector<DMLTriggerPtr> DMLTriggerPtrs;
class DMLTriggers;
typedef std::shared_ptr<DMLTriggers> DMLTriggersPtr;

class Domain;
typedef std::shared_ptr<Domain> DomainPtr;
typedef std::vector<DomainPtr> DomainPtrs;
class Domains;
typedef std::shared_ptr<Domains> DomainsPtr;
class SysDomains;
typedef std::shared_ptr<SysDomains> SysDomainsPtr;

class Exception;
typedef std::shared_ptr<Exception> ExceptionPtr;
typedef std::vector<ExceptionPtr> ExceptionPtrs;
class Exceptions;
typedef std::shared_ptr<Exceptions> ExceptionsPtr;

class ForeignKey;

class FunctionSQL;
typedef std::shared_ptr<FunctionSQL> FunctionSQLPtr;
typedef std::vector<FunctionSQLPtr> FunctionSQLPtrs;
class FunctionSQLs;
typedef std::shared_ptr<FunctionSQLs> FunctionSQLsPtr;


class Generator;
typedef std::shared_ptr<Generator> GeneratorPtr;
typedef std::vector<GeneratorPtr> GeneratorPtrs;
class Generators;
typedef std::shared_ptr<Generators> GeneratorsPtr;

class Index;
typedef std::shared_ptr<Index> IndexPtr;
typedef std::vector<IndexPtr> IndexPtrs;
class Indices;
typedef std::shared_ptr<Indices> IndicesPtr;
class SysIndices;
typedef std::shared_ptr<SysIndices> SysIndicesPtr;
class UsrIndices;
typedef std::shared_ptr<UsrIndices> UsrIndicesPtr;


class MetadataItem;
typedef std::shared_ptr<MetadataItem> MetadataItemPtr;

class Method;
typedef std::shared_ptr<Method> MethodPtr;
typedef std::vector<MethodPtr> MethodPtrs;

class Package;
typedef std::shared_ptr<Package> PackagePtr;
typedef std::vector<PackagePtr> PackagePtrs;
class Packages;
typedef std::shared_ptr<Packages> PackagesPtr;
class SysPackages;
typedef std::shared_ptr<SysPackages> SysPackagesPtr;

class Parameter;
typedef std::shared_ptr<Parameter> ParameterPtr;
typedef std::vector<ParameterPtr> ParameterPtrs;

class PrimaryKeyConstraint;

class Procedure;
typedef std::shared_ptr<Procedure> ProcedurePtr;
typedef std::vector<ProcedurePtr> ProcedurePtrs;
class Procedures;
typedef std::shared_ptr<Procedures> ProceduresPtr;

class Relation;

class Role;
typedef std::shared_ptr<Role> RolePtr;
typedef std::vector<RolePtr> RolePtrs;
class Roles;
typedef std::shared_ptr<Roles> RolesPtr;
class SysRoles;
typedef std::shared_ptr<SysRoles> SysRolesPtr;

class Root;
typedef std::shared_ptr<Root> RootPtr;

class Server;
typedef std::shared_ptr<Server> ServerPtr;
typedef std::weak_ptr<Server> ServerWeakPtr;
typedef std::vector<ServerPtr> ServerPtrs;

class GTTable;
typedef std::shared_ptr<GTTable> GTTablePtr;
typedef std::vector<GTTablePtr> GTTablePtrs;
class GTTables;
typedef std::shared_ptr<GTTables> GTTablesPtr;

class SysTable;
typedef std::shared_ptr<SysTable> SysTablePtr;
typedef std::vector<SysTablePtr> SysTablePtrs;
class SysTables;
typedef std::shared_ptr<SysTables> SysTablesPtr;

class Table;
typedef std::shared_ptr<Table> TablePtr;
typedef std::vector<TablePtr> TablePtrs;
class Tables;
typedef std::shared_ptr<Tables> TablesPtr;

class UDF;
typedef std::shared_ptr<UDF> UDFPtr;
typedef std::vector<UDFPtr> UDFPtrs;
class UDFs;
typedef std::shared_ptr<UDFs> UDFsPtr;

class UniqueConstraint;

class User;
typedef std::shared_ptr<User> UserPtr;
typedef std::vector<UserPtr> UserPtrs;
class Users;
typedef std::shared_ptr<Users> UsersPtr;


class View;
typedef std::shared_ptr<View> ViewPtr;
typedef std::vector<ViewPtr> ViewPtrs;
class Views;
typedef std::shared_ptr<Views> ViewsPtr;

#endif // FR_METADATACLASSES_H
