/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FBCPP_CLIENT_H
#define FBCPP_CLIENT_H

#include "config.h"
#include "fb-api.h"
#include "SmartPtrs.h"
#include <cassert>
#include <concepts>
#include <memory>
#include <string>
#include <type_traits>

#if FB_CPP_USE_BOOST_DLL != 0
#include <boost/dll.hpp>
#endif


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Represents a Firebird client library instance.
	/// The Client must exist and remain valid while there are other objects using it, such as Attachment, Transaction
	/// and Statement.
	///
	class Client final
	{
	public:
		///
		/// Constructs a Client object that uses the specified IMaster interface.
		///
		explicit Client(fb::IMaster* master)
			: master{master}
		{
			assert(master);
		}

#if FB_CPP_USE_BOOST_DLL != 0
		///
		/// Constructs a Client object that loads the Firebird client library (or embedded engine)
		/// from the specified path using Boost.DLL.
		///
		explicit Client(const boost::dll::fs::path& fbclientLibPath,
			boost::dll::load_mode::type loadMode = boost::dll::load_mode::append_decorations |
				boost::dll::load_mode::search_system_folders)
			: Client{boost::dll::shared_library{fbclientLibPath, loadMode}}
		{
		}

		///
		/// Constructs a Client object that uses the specified Boost.DLL shared_library
		/// representing the Firebird client library (or embedded engine).
		///
		explicit Client(boost::dll::shared_library fbclientLib)
			: fbclientLib(fbclientLib)
		{
			const auto fbGetMasterInterface =
				fbclientLib.get<decltype(fb::fb_get_master_interface)>("fb_get_master_interface");
			master = fbGetMasterInterface();
			assert(master);
		}
#endif

		///
		/// Move constructor.
		/// A moved Client object becomes invalid.
		///
		Client(Client&& o) noexcept
			: master{o.master},
			  util{o.util},
			  int128Util{o.int128Util},
			  decFloat16Util{o.decFloat16Util},
			  decFloat34Util{o.decFloat34Util}
#if FB_CPP_USE_BOOST_DLL != 0
			  ,
			  fbclientLib{std::move(o.fbclientLib)}
#endif
		{
			o.master = nullptr;
			o.util = nullptr;
			o.int128Util = nullptr;
			o.decFloat16Util = nullptr;
			o.decFloat34Util = nullptr;
		}

		///
		/// If the Client object was created using Boost.DLL, this destructor just releases the internal
		/// boost::dll::shared_library resource.
		/// It nevers automatically shuts down the Firebird client library (or embedded engine) instance.
		///
		~Client() noexcept = default;

		Client& operator=(Client&&) = delete;

		Client& operator=(const Client& o) = delete;
		Client(const Client&) = delete;

	public:
		///
		/// Returns whether the Client object is valid.
		///
		bool isValid() noexcept
		{
			return master != nullptr;
		}

		///
		/// Returns the Firebird IMaster interface.
		///
		fb::IMaster* getMaster() noexcept
		{
			return master;
		}

		///
		/// Returns a Firebird IUtil interface.
		///
		fb::IUtil* getUtil()
		{
			assert(master);

			if (!util)
				util = master->getUtilInterface();

			return util;
		}

		///
		/// Returns a Firebird IInt128 interface.
		///
		template <std::derived_from<fb::IStatus> StatusType>
		fb::IInt128* getInt128Util(StatusType* status)
		{
			assert(status);

			if (!int128Util)
				int128Util = getUtil()->getInt128(status);

			return int128Util;
		}

		///
		/// Returns a Firebird IDecFloat16 interface.
		///
		template <std::derived_from<fb::IStatus> StatusType>
		fb::IDecFloat16* getDecFloat16Util(StatusType* status)
		{
			assert(status);

			if (!decFloat16Util)
				decFloat16Util = getUtil()->getDecFloat16(status);

			return decFloat16Util;
		}

		///
		/// Returns a Firebird IDecFloat34 interface.
		///
		template <std::derived_from<fb::IStatus> StatusType>
		fb::IDecFloat34* getDecFloat34Util(StatusType* status)
		{
			assert(status);

			if (!decFloat34Util)
				decFloat34Util = getUtil()->getDecFloat34(status);

			return decFloat34Util;
		}

		///
		/// Creates and returns a Firebird IStatus instance.
		///
		FbUniquePtr<fb::IStatus> newStatus()
		{
			assert(master);
			return fbUnique(master->getStatus());
		}

		///
		/// Shuts down the Firebird client library (or embedded engine) instance.
		///
		void shutdown();

	private:
		fb::IMaster* master;
		fb::IUtil* util = nullptr;
		fb::IInt128* int128Util = nullptr;
		fb::IDecFloat16* decFloat16Util = nullptr;
		fb::IDecFloat34* decFloat34Util = nullptr;
#if FB_CPP_USE_BOOST_DLL != 0
		boost::dll::shared_library fbclientLib;
#endif
	};
}  // namespace fbcpp


#endif  // FBCPP_CLIENT_H
