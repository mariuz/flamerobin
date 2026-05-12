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

#include "EventListener.h"
#include "Exception.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <limits>

using namespace fbcpp;
using namespace fbcpp::impl;


static std::uint32_t readUint32LE(const std::uint8_t* data) noexcept
{
	return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8) |
		(static_cast<std::uint32_t>(data[2]) << 16) | (static_cast<std::uint32_t>(data[3]) << 24);
}

static void writeUint32LE(std::uint8_t* data, std::uint32_t value) noexcept
{
	data[0] = static_cast<std::uint8_t>(value & 0xFF);
	data[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
	data[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
	data[3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
}


EventListener::EventListener(Attachment& attachment, const std::vector<std::string>& eventNames, Callback callback)
	: attachment{attachment},
	  client{attachment.getClient()},
	  eventNames{eventNames},
	  callback{callback},
	  firebirdCallback{*this}
{
	assert(attachment.isValid());

	if (eventNames.empty())
		throw std::invalid_argument{"An EventListener requires at least one event"};

	if (!callback)
		throw std::invalid_argument{"EventListener callback must not be empty"};

	for (const auto& name : eventNames)
	{
		if (name.empty())
			throw std::invalid_argument{"Event names must not be empty"};

		if (name.size() > std::numeric_limits<std::uint8_t>::max())
			throw std::invalid_argument{"Event names must be shorter than 256 bytes"};
	}

	if (eventNames.size() > std::numeric_limits<std::uint8_t>::max())
		throw std::invalid_argument{"Number of events must be smaller than 256"};

	std::size_t bufferLength = 1;  // Event block version byte

	for (const auto& name : eventNames)
		bufferLength += 1 + name.size() + sizeof(std::uint32_t);

	eventBuffer.assign(bufferLength, 0);
	resultBuffer.assign(bufferLength, 0);

	auto* eventBufferPtr = eventBuffer.data();
	*eventBufferPtr++ = 1;  // Event parameter block version.
	countOffsets.resize(eventNames.size());

	for (std::size_t i = 0; i < eventNames.size(); ++i)
	{
		const auto& name = eventNames[i];
		*eventBufferPtr++ = static_cast<std::uint8_t>(name.size());
		std::memcpy(eventBufferPtr, name.data(), name.size());
		eventBufferPtr += name.size();
		countOffsets[i] = static_cast<unsigned>(eventBufferPtr - eventBuffer.data());
		eventBufferPtr += sizeof(std::uint32_t);
	}

	assert(static_cast<std::size_t>(eventBufferPtr - eventBuffer.data()) == eventBuffer.size());

	rawCounts.resize(eventNames.size());
	listening = true;
	running = true;

	StatusWrapper statusWrapper{client};

	eventsHandle.reset(attachment.getHandle()->queEvents(
		&statusWrapper, &firebirdCallback, static_cast<unsigned>(eventBuffer.size()), eventBuffer.data()));

	dispatcher = std::thread{&EventListener::dispatchLoop, this};
}

bool EventListener::isListening() noexcept
{
	std::lock_guard mutexGuard{mutex};
	return listening;
}

void EventListener::stop()
{
	std::unique_lock mutexGuard{mutex};

	if (!running)
		return;

	listening = false;
	mutexGuard.unlock();

	try
	{
		cancelEventsHandle();
	}
	catch (...)
	{
		// swallow
	}

	firebirdCallback.detach();

	condition.notify_all();

	if (dispatcher.joinable())
		dispatcher.join();

	mutexGuard.lock();
	pendingNotifications.clear();
	running = false;
	mutexGuard.unlock();

	eventsHandle.reset();
}

void EventListener::decodeEventCounts()
{
	if (eventNames.empty())
		return;

	const auto* current = resultBuffer.data();
	auto* baseline = eventBuffer.data();

	for (std::size_t i = 0; i < eventNames.size(); ++i)
	{
		const auto offset = countOffsets[i];

		if (offset + sizeof(std::uint32_t) > resultBuffer.size() || offset + sizeof(std::uint32_t) > eventBuffer.size())
		{
			rawCounts[i] = 0;
			continue;
		}

		const auto newValue = readUint32LE(current + offset);
		const auto oldValue = readUint32LE(baseline + offset);
		rawCounts[i] = newValue >= oldValue ? (newValue - oldValue) : 0;
		writeUint32LE(baseline + offset, newValue);
	}
}

void EventListener::handleEvent(unsigned length, const std::uint8_t* events)
{
	try
	{
		const auto eventBlockLength = static_cast<unsigned>(eventBuffer.size());
		unsigned copyLength = 0;
		bool notify = false;
		bool shouldRequeue = false;

		{  // scope
			std::lock_guard mutexGuard{mutex};

			if (!listening)
				return;

			copyLength =
				static_cast<unsigned>(std::min<std::size_t>(length, std::min(eventBuffer.size(), resultBuffer.size())));

			if (copyLength == 0)
				return;

			std::memcpy(resultBuffer.data(), events, copyLength);

			decodeEventCounts();

			std::vector<EventCount> counts;
			counts.reserve(eventNames.size());

			for (std::size_t i = 0; i < eventNames.size(); ++i)
			{
				const auto value = rawCounts[i];

				if (value != 0)
					counts.push_back(EventCount{eventNames[i], static_cast<std::uint32_t>(value)});
			}

			if (first)
			{
				if (!counts.empty())
				{
					pendingNotifications.emplace_back(std::move(counts));
					notify = true;
				}
			}
			else
				first = true;

			shouldRequeue = listening;
		}

		if (notify)
			condition.notify_one();

		if (!shouldRequeue)
			return;

		auto attachmentHandle = attachment.getHandle();

		if (!attachmentHandle)
		{
			std::lock_guard mutexGuard{mutex};

			if (listening)
			{
				listening = false;
				condition.notify_all();
			}

			return;
		}

		StatusWrapper statusWrapper{client};
		FbRef<fb::IEvents> newHandle;

		try
		{
			newHandle.reset(
				attachmentHandle->queEvents(&statusWrapper, &firebirdCallback, eventBlockLength, eventBuffer.data()));
		}
		catch (...)
		{
			{  // scope
				std::lock_guard mutexGuard{mutex};

				if (!listening)
					return;

				listening = false;
				condition.notify_all();
			}

			return;
		}

		FbRef<fb::IEvents> previousHandle;

		{
			std::lock_guard mutexGuard{mutex};

			if (listening)
			{
				previousHandle = std::move(eventsHandle);
				eventsHandle = std::move(newHandle);
			}
		}
	}
	catch (...)
	{
		// Prevent exceptions from escaping into Firebird's C API callback.
		// If we can't handle the event, stop listening to avoid repeated failures.
		try
		{
			std::lock_guard mutexGuard{mutex};

			if (listening)
			{
				listening = false;
				condition.notify_all();
			}
		}
		catch (...)
		{
			// If we can't even acquire the mutex, there's nothing we can do.
		}
	}
}

void EventListener::dispatchLoop()
{
	while (true)
	{
		std::vector<EventCount> notification;

		{  // scope
			std::unique_lock mutexGuard{mutex};
			condition.wait(mutexGuard, [this] { return !pendingNotifications.empty() || !listening; });

			if (pendingNotifications.empty())
			{
				if (!listening)
					break;

				continue;
			}

			notification = std::move(pendingNotifications.front());
			pendingNotifications.pop_front();
		}

		try
		{
			callback(notification);
		}
		catch (...)
		{
			assert(false);
		}
	}
}

void EventListener::cancelEventsHandle()
{
	FbRef<fb::IEvents> handle;

	{  // scope
		std::lock_guard mutexGuard{mutex};
		handle = eventsHandle;
	}

	if (!handle)
		return;

	StatusWrapper statusWrapper{client};

	handle->cancel(&statusWrapper);
}
