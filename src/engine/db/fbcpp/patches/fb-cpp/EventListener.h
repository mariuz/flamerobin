/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to do so, subject to the following conditions:
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

#ifndef FBCPP_EVENT_LISTENER_H
#define FBCPP_EVENT_LISTENER_H

#include "Attachment.h"
#include "Client.h"
#include "Exception.h"
#include "SmartPtrs.h"
#include "fb-api.h"
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	///
	/// Represents the number of occurrences for a registered event delivered by Firebird.
	///
	struct EventCount final
	{
		///
		/// Name of the event reported by the database.
		///
		std::string name;

		///
		/// Number of times the event fired since the last notification.
		///
		unsigned count;
	};

	///
	/// Observes Firebird events and forwards aggregated counts to a callback on a background thread.
	///
	class EventListener final
	{
	public:
		///
		/// Function invoked when new event counts are available.
		///
		using Callback = std::function<void(const std::vector<EventCount>& counts)>;

	private:
		class FirebirdCallback final : public fb::IEventCallbackImpl<FirebirdCallback, impl::StatusWrapper>
		{
		public:
			explicit FirebirdCallback(EventListener& owner) noexcept
				: owner{&owner}
			{
			}

			void eventCallbackFunction(unsigned length, const std::uint8_t* events) override
			{
				if (owner)
					owner->handleEvent(length, events);
			}

			void addRef() override
			{
				// The listener owns this callback for its whole lifetime.
			}

			int release() override
			{
				// Prevent Firebird from attempting to destroy the stack-allocated callback.
				return 1;
			}

			void detach() noexcept
			{
				owner = nullptr;
			}

		private:
			EventListener* owner;
		};

	public:
		///
		/// Creates an event listener for the specified attachment and event names using the provided callback.
		///
		explicit EventListener(Attachment& attachment, const std::vector<std::string>& eventNames, Callback callback);

		///
		/// Stops the listener and waits for any background work to finish.
		///
		~EventListener() noexcept
		{
			try
			{
				stop();
			}
			catch (...)
			{
				// swallow
			}
		}

		EventListener(const EventListener&) = delete;
		EventListener& operator=(const EventListener&) = delete;

		EventListener(EventListener&&) = delete;
		EventListener& operator=(EventListener&&) = delete;

	public:
		///
		/// Returns true if the listener is currently registered for event notifications.
		///
		bool isListening() noexcept;

		///
		/// Cancels event notifications and releases related resources.
		///
		void stop();

	private:
		void handleEvent(unsigned length, const std::uint8_t* events);
		void dispatchLoop();
		void cancelEventsHandle();
		void decodeEventCounts();

	private:
		Attachment& attachment;
		Client& client;
		std::vector<std::string> eventNames;
		Callback callback;
		FbRef<fb::IEvents> eventsHandle;
		FirebirdCallback firebirdCallback;
		std::vector<std::uint8_t> eventBuffer;
		std::vector<std::uint8_t> resultBuffer;
		std::vector<std::uint32_t> rawCounts;
		std::vector<unsigned> countOffsets;
		std::deque<std::vector<EventCount>> pendingNotifications;
		std::thread dispatcher;
		std::mutex mutex;
		std::condition_variable condition;
		bool listening = false;
		bool running = false;
		bool first = false;
	};
}  // namespace fbcpp


#endif  // FBCPP_EVENT_LISTENER_H
