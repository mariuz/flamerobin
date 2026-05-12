/*
 * MIT License
 *
 * Copyright (c) 2026 Adriano dos Santos Fernandes
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

#ifndef FBCPP_SERVICE_MANAGER_H
#define FBCPP_SERVICE_MANAGER_H

#include "Exception.h"
#include "fb-api.h"
#include "SmartPtrs.h"
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	class Client;

	///
	/// Represents options used when creating a ServiceManager object.
	///
	class ServiceManagerOptions final
	{
	public:
		///
		/// Returns the server used to attach to the service manager.
		///
		const std::optional<std::string>& getServer() const
		{
			return server;
		}

		///
		/// Sets the server used to attach to the service manager.
		///
		ServiceManagerOptions& setServer(const std::string& value)
		{
			server = value;
			return *this;
		}

		///
		/// Returns the service manager name.
		///
		const std::string& getServiceManagerName() const
		{
			return serviceManagerName;
		}

		///
		/// Sets the service manager name.
		///
		ServiceManagerOptions& setServiceManagerName(const std::string& value)
		{
			serviceManagerName = value;
			return *this;
		}

		///
		/// Returns the user name used to attach to the service manager.
		///
		const std::optional<std::string>& getUserName() const
		{
			return userName;
		}

		///
		/// Sets the user name used to attach to the service manager.
		///
		ServiceManagerOptions& setUserName(const std::string& value)
		{
			userName = value;
			return *this;
		}

		///
		/// Returns the password used to attach to the service manager.
		///
		const std::optional<std::string>& getPassword() const
		{
			return password;
		}

		///
		/// Sets the password used to attach to the service manager.
		///
		ServiceManagerOptions& setPassword(const std::string& value)
		{
			password = value;
			return *this;
		}

		///
		/// Returns the role used to attach to the service manager.
		///
		const std::optional<std::string>& getRole() const
		{
			return role;
		}

		///
		/// Sets the role used to attach to the service manager.
		///
		ServiceManagerOptions& setRole(const std::string& value)
		{
			role = value;
			return *this;
		}

		///
		/// Returns the raw service attach SPB.
		///
		const std::vector<std::uint8_t>& getSpb() const
		{
			return spb;
		}

		///
		/// Sets the raw service attach SPB.
		///
		ServiceManagerOptions& setSpb(const std::vector<std::uint8_t>& value)
		{
			spb = value;
			return *this;
		}

		///
		/// Sets the raw service attach SPB.
		///
		ServiceManagerOptions& setSpb(std::vector<std::uint8_t>&& value)
		{
			spb = std::move(value);
			return *this;
		}

	private:
		std::optional<std::string> server;
		std::string serviceManagerName = "service_mgr";
		std::optional<std::string> userName;
		std::optional<std::string> password;
		std::optional<std::string> role;
		std::vector<std::uint8_t> spb;
	};

	///
	/// Represents a connection to the Firebird service manager.
	///
	class ServiceManager
	{
	public:
		///
		/// Function invoked when a verbose service output line is available.
		///
		using VerboseOutput = std::function<void(std::string_view line)>;

		///
		/// Attaches to the service manager specified by the given options.
		///
		explicit ServiceManager(Client& client, const ServiceManagerOptions& options = {});

		///
		/// Move constructor.
		/// A moved ServiceManager object becomes invalid.
		///
		ServiceManager(ServiceManager&& o) noexcept
			: client{o.client},
			  handle{std::move(o.handle)}
		{
		}

		///
		/// @brief Transfers ownership of another ServiceManager into this one.
		///
		/// The old handle is detached via `disconnect()`.
		/// After the assignment, `this` is valid (with `o`'s handle) and `o` is invalid.
		///
		ServiceManager& operator=(ServiceManager&& o) noexcept
		{
			if (this != &o)
			{
				if (isValid())
				{
					try
					{
						detachHandle();
					}
					catch (...)
					{
						// swallow
					}
				}

				client = o.client;
				handle = std::move(o.handle);
			}

			return *this;
		}

		ServiceManager(const ServiceManager&) = delete;
		ServiceManager& operator=(const ServiceManager&) = delete;

		///
		/// Detaches from the service manager.
		///
		~ServiceManager() noexcept
		{
			if (isValid())
			{
				try
				{
					detachHandle();
				}
				catch (...)
				{
					// swallow
				}
			}
		}

	public:
		///
		/// Returns whether the ServiceManager object is valid.
		///
		bool isValid() noexcept
		{
			return handle != nullptr;
		}

		///
		/// Returns the Client object reference used to create this ServiceManager object.
		///
		Client& getClient() noexcept
		{
			return *client;
		}

		///
		/// Returns the internal Firebird IService handle.
		///
		FbRef<fb::IService> getHandle() noexcept
		{
			return handle;
		}

		///
		/// Detaches from the service manager.
		///
		void disconnect();

	protected:
		static void addSpbInt(fb::IXpbBuilder* builder, impl::StatusWrapper* status, unsigned char tag,
			std::uint64_t value, const char* what)
		{
			if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
				throw FbCppException(std::string(what) + " is too large");

			if (value > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
				builder->insertBigInt(status, tag, static_cast<std::int64_t>(value));
			else
				builder->insertInt(status, tag, static_cast<int>(value));
		}

		void startAction(const std::vector<std::uint8_t>& spb);
		void waitForCompletion(const VerboseOutput& verboseOutput = {}, bool requestStdin = false);

	private:
		void detachHandle();

	private:
		Client* client;
		FbRef<fb::IService> handle;
	};
}  // namespace fbcpp


#endif  // FBCPP_SERVICE_MANAGER_H
