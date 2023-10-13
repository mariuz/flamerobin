//	Initialization of the library

/*	(C) Copyright 2000-2006 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)

    The contents of this file are subject to the IBPP License (the "License");
    you may not use this file except in compliance with the License.  You may
    obtain a copy of the License at http://www.ibpp.org or in the 'license.txt'
    file which must have been distributed along with this file.

    This software, distributed under the License, is distributed on an "AS IS"
    basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
    License for the specific language governing rights and limitations
    under the License.
*/


#ifdef _MSC_VER
#pragma warning(disable: 4786 4996)
#ifndef _DEBUG
#pragma warning(disable: 4702)
#endif
#endif

#include "_ibpp.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <limits>

#ifdef IBPP_WINDOWS
#include <shlwapi.h>

// Optional Registry Keys introduced by Firebird Server 1.5.x
#define REG_KEY_ROOT_INSTANCES	"SOFTWARE\\Firebird Project\\Firebird Server\\Instances"
#define FB_DEFAULT_INSTANCE	  	"DefaultInstance"

#endif

#ifdef IBPP_UNIX
#ifdef IBPP_LATE_BIND

#include <dlfcn.h>
#include <stdlib.h>

//empty string terminated list of Firebird SO libraries to try in turn
static const char* fblibs[] = {"libfbembed.so.2.5","libfbembed.so.2.1","libfbclient.so.2",""};

#endif
#endif


// Many compilers confuses those following min/max with macros min and max !
#undef min
#undef max

namespace ibpp_internals
{
	const double consts::dscales[19] = {
		  1, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8,
		  1E9, 1E10, 1E11, 1E12, 1E13, 1E14, 1E15,
		  1E16, 1E17, 1E18 };

	const int consts::Dec31_1899 = 693595;

	const int16_t consts::min16 = std::numeric_limits<int16_t>::min();
	const int16_t consts::max16 = std::numeric_limits<int16_t>::max();
	const int32_t consts::min32 = std::numeric_limits<int32_t>::min();
	const int32_t consts::max32 = std::numeric_limits<int32_t>::max();

    FBCLIENT gds;	// Global unique GDS instance

#ifdef _DEBUG
	std::ostream& operator<< (std::ostream& a, flush_debug_stream_type)
	{
		if (std::stringstream* p = dynamic_cast<std::stringstream*>(&a))
		{
#ifdef IBPP_WINDOWS
			::OutputDebugString(("IBPP: " + p->str() + "\n").c_str());
#endif
			p->str("");
		}
		return a;
	}
#endif	// _DEBUG

}

using namespace ibpp_internals;

#ifdef IBPP_WINDOWS
HMODULE IBPP_LoadLibrary(std::string library) {
    HMODULE handle = 0;
    handle = LoadLibrary(library.c_str());
    if (handle == 0) {
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID != 0) {
            if ((PathFileExists(library.c_str()) == 1) || (errorMessageID != 126)) {

                LPSTR messageBuffer = nullptr;

                    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

                    std::string message(messageBuffer, size);
                    throw LogicExceptionImpl(library, messageBuffer);

                    LocalFree(messageBuffer);
            }

        }

    }
    return handle;
};
#endif

FBCLIENT* FBCLIENT::Call()
{
	// Let's load the CLIENT library, if it is not already loaded.
	// The load is guaranteed to be done only once per application.
	// Loading order : specifically defined path > local directory fbembed > local directory fbclient > PATH and System directories > Look at Win registry for Fb setup folder > gds32.dll

	if (! mReady)
	{
#ifdef IBPP_WINDOWS

		// Let's load the FBCLIENT.DLL or GDS32.DLL, we won't release it.
		// Windows will do that for us when the executable will terminate.

		char fbdll[MAX_PATH];
		HKEY hkey_instances;

		// Try to load FBCLIENT.DLL from each of the additional optional paths
		// that may have been specified through ClientLibSearchPaths().
		mHandle = 0;

        // try specific library
        if (lstrlen(mfbdll.c_str()) > 0) {
            //mHandle = LoadLibrary(mfbdll.c_str());
            mHandle = IBPP_LoadLibrary(mfbdll);
        }

        if (mHandle == 0) {
            std::string::size_type pos = 0;
            while (pos < mSearchPaths.size())
            {
                std::string::size_type newpos = mSearchPaths.find(';', pos);

                std::string path;
                if (newpos == std::string::npos) 
                    path = mSearchPaths.substr(pos);
                else 
                    path = mSearchPaths.substr(pos, newpos - pos);

                if (path.size() >= 1)
                {
                    if (path[path.size() - 1] != '\\') 
                        path += '\\';
                    path.append("fbclient.dll");
                    mHandle = LoadLibrary(path.c_str());
                    if (mHandle != 0 || newpos == std::string::npos) 
                        break;
                }
                pos = newpos + 1;
            }
        }

		if (mHandle == 0)
		{
			// Try to load FBCLIENT.DLL from the current application location.  This
			// is a usefull step for applications using the embedded version of FB
			// or a local copy (for whatever reasons) of the dll.

			int len = GetModuleFileName(NULL, fbdll, sizeof(fbdll));
			if (len != 0)
			{
				// Get to the last '\' (this one precedes the filename part).
				// There is always one after a success call to GetModuleFileName().
				char* p = fbdll + len;
				do {--p;} while (*p != '\\');
				*p = '\0';
				lstrcat(fbdll, "\\fbembed.dll");// Local copy could be named fbembed.dll
				mHandle = IBPP_LoadLibrary(fbdll);
				if (mHandle == 0)
				{
					*p = '\0';
					lstrcat(fbdll, "\\fbclient.dll");	// Or possibly renamed fbclient.dll
					mHandle = IBPP_LoadLibrary(fbdll);
				}
			}
		}

        if (mHandle == 0)
        {
            // Let's try from the PATH and System directories
            mHandle = IBPP_LoadLibrary("fbclient.dll");
        }

		if (mHandle == 0)
		{
			// Try to locate FBCLIENT.DLL through the optional FB 1.5 registry
			// key. Note that this will have to be enhanced in a later version
			// of IBPP in order to be able to select among multiple instances
			// that Firebird Server version > 1.5 might introduce.

			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_ROOT_INSTANCES, 0,
					KEY_READ, &hkey_instances) == ERROR_SUCCESS
				// try 64 bit registry view for 32 bit client program with 64 bit server
				|| RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_ROOT_INSTANCES, 0,
					KEY_READ | KEY_WOW64_64KEY, &hkey_instances) == ERROR_SUCCESS)
			{
				DWORD keytype;
				DWORD buflen = sizeof(fbdll);
				if (RegQueryValueEx(hkey_instances, FB_DEFAULT_INSTANCE, 0,
						&keytype, reinterpret_cast<UCHAR*>(fbdll),
						&buflen) == ERROR_SUCCESS
					&& keytype == REG_SZ)
				{
                    int len = lstrlen(fbdll);

#if !defined(_WIN64)
                    // try 32 bit client library of 64 bit server
                    lstrcpy(fbdll + len, "WOW64\\fbclient.dll");
                    mHandle = IBPP_LoadLibrary(fbdll);
                    if (mHandle == 0)
                    {
                        lstrcpy(fbdll + len, "bin\\WOW64\\fbclient.dll");
                        mHandle = IBPP_LoadLibrary(fbdll);
                    }
#endif
                    // for Firebird 3+
                    if (mHandle == 0) {
                        lstrcpy(fbdll + len, "fbclient.dll");
                        mHandle = IBPP_LoadLibrary(fbdll);
                    }
                    // for Firebird 2.5 -
                    if (mHandle == 0) {
                        lstrcpy(fbdll + len, "bin\\fbclient.dll");
                        mHandle = IBPP_LoadLibrary(fbdll);
                    }
                }
				RegCloseKey(hkey_instances);
			}
		}

        if (mHandle == 0)
        {
            // Not found. Last try : attemps loading gds32.dll from PATH and
            // System directories
            mHandle = IBPP_LoadLibrary("gds32.dll");
        }

		if (mHandle == 0)
			throw LogicExceptionImpl("GDS::Call()",
				_("Can't find or load FBEMBED.DLL FBCLIENT.DLL or GDS32.DLL"));
		
#endif

#ifdef IBPP_UNIX
#ifdef IBPP_LATE_BIND

               mHandle = 0;
               if (getenv("FBLIB") != 0)
                       mHandle = dlopen(getenv("FBLIB"),RTLD_LAZY);
               else
               {
                       int ixlib = 0;
                       while (fblibs[ixlib] != "")
                       {
                               mHandle = dlopen(fblibs[ixlib],RTLD_LAZY);
                               if (mHandle != 0) break;
                               ixlib++;
                       }
               }

               if (mHandle == 0)
                                       throw LogicExceptionImpl("FBCLIENT::Call()",
                                               _("Can't find or load the Firebird Client Library"));

#endif
#endif


		// Get the entry points that we need

#ifdef IBPP_WINDOWS
#define IB_ENTRYPOINT(X) \
			if ((m_##X = (proto_##X*)GetProcAddress(mHandle, "isc_"#X)) == 0) \
                throw LogicExceptionImpl("FBCLIENT:gds()", _("Entry-point isc_"#X" not found"))
#define FB_ENTRYPOINT(X) \
            if ((m_##X = (proto_##X*)GetProcAddress(mHandle, "fb_"#X)) == 0) \
                throw LogicExceptionImpl("FBCLIENT:gds()", _("Entry-point fb_"#X" not found"))
#endif
#ifdef IBPP_UNIX
#ifdef IBPP_LATE_BIND
#define IB_ENTRYPOINT(X) \
    if ((m_##X = (proto_##X*)dlsym(mHandle,"isc_"#X)) == 0) \
        throw LogicExceptionImpl("FBCLIENT:gds()", _("Entry-point isc_"#X" not found"))
#define FB_ENTRYPOINT(X) \
    if ((m_##X = (proto_##X*)dlsym(mHandle,"fb_"#X)) == 0) \
        throw LogicExceptionImpl("FBCLIENT:gds()", _("Entry-point fb_"#X" not found"))
#else
#define IB_ENTRYPOINT(X) m_##X = (proto_##X*)isc_##X
#define FB_ENTRYPOINT(X) m_##X = (proto_##X*)fb_##X
#endif
#endif

		IB_ENTRYPOINT(create_database);
		IB_ENTRYPOINT(attach_database);
		IB_ENTRYPOINT(detach_database);
		IB_ENTRYPOINT(drop_database);
		IB_ENTRYPOINT(database_info);
		IB_ENTRYPOINT(open_blob2);
		IB_ENTRYPOINT(create_blob2);
		IB_ENTRYPOINT(close_blob);
		IB_ENTRYPOINT(cancel_blob);
		IB_ENTRYPOINT(get_segment);
		IB_ENTRYPOINT(put_segment);
		IB_ENTRYPOINT(blob_info);
		IB_ENTRYPOINT(array_lookup_bounds);
		IB_ENTRYPOINT(array_get_slice);
		IB_ENTRYPOINT(array_put_slice);
		IB_ENTRYPOINT(vax_integer);
		IB_ENTRYPOINT(sqlcode);
        IB_ENTRYPOINT(sql_interprete);
        #if defined(FB_API_VER) && FB_API_VER >= 20
        FB_ENTRYPOINT(interpret);
        #else
        IB_ENTRYPOINT(interprete);
        #endif
		IB_ENTRYPOINT(que_events);
		IB_ENTRYPOINT(cancel_events);
		IB_ENTRYPOINT(start_multiple);
		IB_ENTRYPOINT(commit_transaction);
		IB_ENTRYPOINT(commit_retaining);
		IB_ENTRYPOINT(rollback_transaction);
		IB_ENTRYPOINT(rollback_retaining);
		IB_ENTRYPOINT(dsql_execute_immediate);
		IB_ENTRYPOINT(dsql_allocate_statement);
		IB_ENTRYPOINT(dsql_describe);
		IB_ENTRYPOINT(dsql_describe_bind);
		IB_ENTRYPOINT(dsql_prepare);
		IB_ENTRYPOINT(dsql_execute);
		IB_ENTRYPOINT(dsql_execute2);
		IB_ENTRYPOINT(dsql_fetch);
		IB_ENTRYPOINT(dsql_free_statement);
		IB_ENTRYPOINT(dsql_set_cursor_name);
		IB_ENTRYPOINT(dsql_sql_info);

		IB_ENTRYPOINT(service_attach);
		IB_ENTRYPOINT(service_detach);
		IB_ENTRYPOINT(service_start);
		IB_ENTRYPOINT(service_query);

		mReady = true;
	}

	return this;
}

namespace IBPP
{

	bool CheckVersion(uint32_t AppVersion)
	{
		//(void)gds.Call(); 		// Just call it to trigger the initialization
		return (AppVersion & 0xFFFFFF00) ==
				(IBPP::Version & 0xFFFFFF00) ? true : false;
	}

#ifdef IBPP_WINDOWS
	void ClientLibSearchPaths(const std::string& paths)
	{
		gds.mSearchPaths.assign(paths);
	}
#else
	void ClientLibSearchPaths(const std::string&)
	{
	}
#endif

	//	Factories for our Interface objects

    Service ServiceFactory(const std::string& ServerName,
				const std::string& UserName, const std::string& UserPassword,
                const std::string& RoleName, const std::string& CharSet,
                const std::string& FBClient )
	{
        if (FBClient.length() != 0)
            gds.mfbdll = FBClient;
        (void)gds.Call();			// Triggers the initialization, if needed
		return new ServiceImpl(ServerName, UserName, UserPassword, RoleName, CharSet);
	}

	Database DatabaseFactory(const std::string& ServerName,
		const std::string& DatabaseName, const std::string& UserName,
		const std::string& UserPassword, const std::string& RoleName,
		const std::string& CharSet, const std::string& CreateParams,
        const std::string& FBClient)
	{
        
        if (FBClient.length() != 0)
            gds.mfbdll = FBClient;
		(void)gds.Call();			// Triggers the initialization, if needed
		return new DatabaseImpl(ServerName, DatabaseName, UserName,
								UserPassword, RoleName, CharSet, CreateParams);
	}

	Transaction TransactionFactory(Database db, TAM am,
					TIL il, TLR lr, TFF flags)
	{
        (void)gds.Call();			// Triggers the initialization, if needed
		return new TransactionImpl(	dynamic_cast<DatabaseImpl*>(db.intf()),
									am, il, lr, flags);
	}

	Statement StatementFactory(Database db, Transaction tr)
	{
        (void)gds.Call();			// Triggers the initialization, if needed
		return new StatementImpl(	dynamic_cast<DatabaseImpl*>(db.intf()),
									dynamic_cast<TransactionImpl*>(tr.intf()));
	}

	Blob BlobFactory(Database db, Transaction tr)
	{
        (void)gds.Call();			// Triggers the initialization, if needed
		return new BlobImpl(dynamic_cast<DatabaseImpl*>(db.intf()),
							dynamic_cast<TransactionImpl*>(tr.intf()));
	}

	Array ArrayFactory(Database db, Transaction tr)
	{
        (void)gds.Call();			// Triggers the initialization, if needed
		return new ArrayImpl(dynamic_cast<DatabaseImpl*>(db.intf()),
							dynamic_cast<TransactionImpl*>(tr.intf()));
	}

	Events EventsFactory(Database db)
	{
        (void)gds.Call();			// Triggers the initialization, if needed
		return new EventsImpl(dynamic_cast<DatabaseImpl*>(db.intf()));
	}

    bool isIntegerNumber(SDT type)
    {
        switch (type) {
        case SDT::sdSmallint:
        case SDT::sdInteger:
        case SDT::sdLargeint:
        case SDT::sdInt128:
            return true;
        }
        return false;

    }

    bool isRationalNumber(SDT type)
    {
        if (isIntegerNumber(type))
            return true;
        switch (type) {
        case SDT::sdDouble:
        case SDT::sdFloat:
        case SDT::sdDec16:
        case SDT::sdDec34:
            return true;
        }
        return false;

    }
}
