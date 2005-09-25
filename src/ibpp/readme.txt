$Id$

IBPP, A C++ Client API for Firebird (and Interbase)
Some general notes about the ibpp project

---------------------------------------------------------------------------

The contents of this file are subject to the Mozilla Public License
Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is "IBPP 0.9" and all its associated documentation.

The Initial Developer of the Original Code is T.I.P. Group S.A.
Portions created by T.I.P. Group S.A. are
Copyright (C) 2000 T.I.P Group S.A.
All Rights Reserved.

Contributor(s): ______________________________________.

---------------------------------------------------------------------------

CALL FOR CONTRIBUTORS
If you have an interest in using such a non-visual, C++ based interface to
Firebird or Interbase in your projects, and are willing to build on this,
contact me to get involved : om@tipgroup.com, or subscribe to and write to
the list :

	ibpp-discuss@lists.sourceforge.net

To subscribe and later update your lists settings or unsubscribe, please
visit http://lists.sourceforge.net/mailman/listinfo/ibpp-discuss

---------------------------------------------------------------------------

DOCUMENTATION
The documentation is currently only available online on the project
website at http://www.ibpp.org.

---------------------------------------------------------------------------

Release Notes for Version 2.4 (August, 2005)


1/ Introduction

IBPP, where 'PP' stands for '++', is a C++ client interface for Firebird Server
versions 1.0, 1.5 and further. It also works with InterBase(r) 6.0, though it is
expected it might only support Firebird in the future. It is a class library,
free of any specific development tool dependancies. It is not tied to any
'visual' or 'RAD' tool. It was indeed developed to add Firebird access in any
C++ application. Those applications using IBPP can be non-visual (CORBA/COM
objects, other libraries of classes and functions, procedural 'legacy' code, for
instance). But it can of course also be used in visual or RAD environments. IBPP
is indeed purely a dynamic SQL interface to Firebird. In some easy to use C++
classes, you will find nearly all what is needed to access a Firebird database,
and manipulate the data. IBPP also offers access to most of the administrations
tasks : creating a database, modifying its structure, performing online backups,
administering user accounts on the server and so on.

More background information on http://www.ibpp.org website.


2/ Installation

There is no 'installation' per se because IBPP Project does not release
binaries. It is much better / safer that you build your own binaries of the
class library with your tools, while adjusting compiler settings to suit your
host projects. Please refer to the file howtobuild.txt from this distribution
for basic instructions. The online documentation on http://www.ibpp.org is a
second source of information.


3/ New or Updated Features since Version 2.3

Version 2.4 was meant essentially as a maintenance version. We chose to tag it
2.4 instead of 2.3.6 mostly because of the exception classes changes and the
blob interfaces extensions which both change the public API significantly in
some areas.

The next major version (3.0) will start beta some weeks after release of
version 2.4. This 2.4 release has been used by T.I.P. Group S.A. in Windows
products since february 2005. So at least on this platform it should be rock-
solid.

- Exception handling revision.  There is now IBPP::Exception as a base class.
SQLException specialization is thrown as result of an engine reported runtime
error. LogicException specialization is thrown in all other cases.

                    std::exception
                           |
                   IBPP::Exception
                 /                 \
    IBPP::LogicException    IBPP::SQLException

- Change in the interface definition of methods using or returning 64 bits
integers. The ibpp.h now uses the C99 standard (section 7.18) types which are
defined by including the stdint.h header. This is perfectly fine using all
compilers on which IBPP is maintained, except for one. Guess which one ? Yes,
MSVC does not supply that header nor those typedefs in any other file. The
solution is simple, ibpp.h does a typedef __int64 int64_t in case of the MSVC
compilers.

- Extended the Statement::Set() and Statement::Get() in regards to blobs and
std::string. Now one can directly Get(3, str) where str is a std::string and
the column at hands is a blob. The blob is retrieved and stored to the string.
The reverse is true for Set(3, str) where str is a std::string. It will be
stored directly. For simple case useages, this allows to bypass the useage of
any IBPP::Blob intermediate objects and, unless finer control is required
(which Create/Open/Write/Read/Close offer), it it much simpler to use blobs
from application code. Also, remember that std::string can store arbitrary
binary data which might contain one or multiple values 0. So this std::string
interface for blobs really has some potential.

- Added methods Save(const std::string&) and Load(std::string) to class Blob.
Those methods allow to easily read and write a full blob using a std::string.
A Save(), is the equivalent of Create(), a loop of Write(), and Close().
A Load(), is the equivalent of Open(), a loop of Read(), and Close(). Adding
this interface on IBPP::Blob was logical now that IBPP::Statement had a direct
support for blobs and std:string.

- An IBPP::Row has been introduced and the code refactored around it. This is
perfectly functionnal but is still a concept in the works for this 2.4 release.
So we ask users of IBPP not to start using IBPP::Row as is, consider it already
deprecated in its current form. We will introduce something 'bigger' in the
next major version (3.0.

- Added void ClientDLLSearchPaths(const std::string&);
On Win32 platform, ClientDLLSearchPaths() allows to setup one or multiple 
additional paths (separated with a ';') where IBPP will look for the client 
library (before the default implicit search locations). This is usefull for 
applications distributed with a 'private' copy of Firebird, when the registry 
is useless to identify the location from where to attempt loading the 
fbclient.dll / gds32.dll. If called, this function must be called *early* by 
the application, before *any* other function or object methods of IBPP. This 
is currently a NO-OP on platforms other than Win32.


4/ Comments, Acknowledgments

This 2.4 version is considered the best IBPP version available to date and we
recommend all its users to upgrade at their earliest opportunity.

We have a lot of people to thank. The ibpp.h file cites some of them. But many
people suggested various kind of enhancements or were invaluable for their
mentoring or expertise in various fields.


5/ History section - What was New or Updated between Version 2.2 and 2.3

- The Service interface now support sweep action. See ibpp.h for details:
	virtual void Sweep(const std::string& dbfile) = 0;

- Added void Statement::Plan(std::string&) to retrieve the plan after a prepare.

- Added Service::ListUsers() to retrieve the full list of usernames defined in
the security database.

- Added 'bool Connected(void)' methods to Database and Service class.

- Added 'bool Started(void)' method to Transaction class.

- Firebird 1.5 is specifically supported. Further, on Win32, the way IBPP looks
for and loads the client library has been slightly updated:

	1. Looks for fbembed.dll in host application directory.
	2. Looks for fbclient.dll in host application directory.
	3. Looks for fbclient.dll through 'DefaultInstance' registry key/value.
	4. Looks for fbclient.dll in system defined locations.
	5. Looks for gds32.dll in system defined locations.
	The DLL loaded is the first one found folllowing above order.

- Added new overloads to the set of Statement::Get() methods. These overloads
use references to basic data types instead of pointers to such (bool, short,
int, long, int64_t, float, double). Of course, not only the previous interfaces
are maintained for compatibility, but we don't see any reason to deprecate them
in the future.

- Now can be compiled and used on 'Darwin'. Some very small fixups were required.

- There is a set of solution and project files for users of Microsoft Visual
Studio .NET 2003. All is required is to open the 'ibpp.sln' and then hit build.


6/ History section - What was New or Updated between Version 2.1 and 2.2

- Dates valid range has been extended, both in the Date and Timestamp classes.
IBPP now fully support the range extending from 1 JAN 0001 to 31 DEC 9999. The
utility functions dtoi and itod now return a bool to signal a date as valid or
invalid.

	bool dtoi (int date, int *y, int *m, int *d);
	bool itod (int* pdate, int year, int month, int day);

- The Transaction interface fully support special lock reservation options
through new optional flags. See ibpp.h for details :

	enum TTR {trSharedWrite, trSharedRead,
		trProtectedWrite, trProtectedRead};

	enum TFF {tfIgnoreLimbo = 0x1, tfAutoCommit = 0x2, tfNoAutoUndo = 0x4};

	ITransaction* TransactionFactory(IDatabase* db, TAM am = amWrite,
		TIL il = ilConcurrency, TLR lr = lrWait, TFF flags = TFF(0));

	virtual void AddReservation(IDatabase* db,
			const std::string& table, TTR tr) = 0;

- The Service interface now fully support special backup/restore options through
new optional flags. See ibpp.h for details :

	enum BRF {
		brVerbose = 0x1,
		// Backup flags
		brIgnoreChecksums = 0x100, brIgnoreLimbo = 0x200,
		brMetadataOnly = 0x400, brNoGarbageCollect = 0x800,
		brNonTransportable = 0x1000, brConvertExtTables = 0x2000,
		// Restore flags
		brReplace = 0x10000, brDeactivateIdx = 0x20000,
		brNoShadow = 0x40000, brNoValidity = 0x80000,
		brPerTableCommit = 0x100000, brUseAllSpace = 0x200000
	};
	virtual void StartBackup(const std::string& dbfile,
		const std::string& bkfile, BRF flags = BRF(0)) = 0;
	virtual void StartRestore(const std::string& bkfile, const std::string& dbfile,
		int pagesize = 0, BRF flags = BRF(0)) = 0;

- The Service interface now support repair actions, with a range of new optional
flags. See ibpp.h for details :

	enum RPF
	{
		// Mandatory and mutually exclusives
		rpMendRecords = 0x1, rpValidatePages = 0x2, rpValidateFull = 0x4,
		// Options
		rpReadOnly = 0x100, rpIgnoreChecksums = 0x200, rpKillShadows = 0x400
	};
	virtual void Repair(const std::string& dbfile, RPF flags) = 0;

- The Service interface has a new member to get the version string from the
server :

	virtual void GetVersion(std::string& version) = 0;

- Some statistical and informational methods of Database interface have been fully
revised and now offer :

	virtual void Info(int* ODS, int* ODSMinor, int* PageSize, int* Pages,
		int* Buffers, int* Sweep, bool* Sync, bool* Reserve) = 0;
	virtual void Statistics(int* Fetches, int* Marks,
		int* Reads, int* Writes) = 0;
	virtual void Counts(int* Insert, int* Update, int* Delete, 
		int* ReadIdx, int* ReadSeq) = 0;

- The Database interface method DropEvent() has been dropped. It was never
implemented since year 2000 and its implementation would have introduced
unnecessary complications to work around some silly race conditions.

- Build support has been added for the Digital Mars C++ Compiler
(http://www.digitalMars.com).

- NULL macro has been dropped from the source code as this is not actually part
of the C++ Standard.

- Useage of '_' (underscore) as a leading character in internal identifiers of
the library has been dropped. Such naming convention is reserved for
implementation and thus may clash with system defined identifiers.

>>> EOF <<<
