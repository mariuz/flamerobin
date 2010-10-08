/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

class Database;
typedef boost::shared_ptr<Database> DatabasePtr;
typedef std::vector<DatabasePtr> DatabasePtrs;

class Domain;
class Domains;
class Exception;
class Exceptions;
class ForeignKey;
class Function;
class Functions;
class Generator;
class Generators;
class Index;

class MetadataItem;
typedef boost::shared_ptr<MetadataItem> MetadataItemPtr;

class Parameter;
class PrimaryKeyConstraint;
class Procedure;
class Procedures;
class Relation;
class Role;
class Roles;

class Root;
typedef boost::shared_ptr<Root> RootPtr;

class Server;
typedef boost::shared_ptr<Server> ServerPtr;
typedef std::vector<ServerPtr> ServerPtrs;

class SysTables;
class Table;
class Tables;
class Trigger;
class Triggers;
class UniqueConstraint;
class View;
class Views;
//-----------------------------------------------------------------------------
#endif // FR_METADATACLASSES_H
