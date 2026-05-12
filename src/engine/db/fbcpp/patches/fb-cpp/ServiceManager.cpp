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

#include "ServiceManager.h"
#include "Client.h"
#include "Exception.h"
#include <cassert>

using namespace fbcpp;
using namespace fbcpp::impl;


static void emitVerboseChunk(
	std::string& pendingLine, const std::string_view chunk, const ServiceManager::VerboseOutput& verboseOutput)
{
	if (!verboseOutput || chunk.empty())
		return;

	for (char ch : chunk)
	{
		if (ch == '\r')
			continue;

		if (ch == '\n')
		{
			verboseOutput(pendingLine);
			pendingLine.clear();
		}
		else
			pendingLine += ch;
	}
}


ServiceManager::ServiceManager(Client& client, const ServiceManagerOptions& options)
	: client{&client}
{
	const auto master = client.getMaster();
	StatusWrapper statusWrapper{client};

	auto spbBuilder = fbUnique(client.getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_ATTACH,
		reinterpret_cast<const std::uint8_t*>(options.getSpb().data()),
		static_cast<unsigned>(options.getSpb().size())));

	if (const auto userName = options.getUserName())
		spbBuilder->insertString(&statusWrapper, isc_spb_user_name, userName->c_str());

	if (const auto password = options.getPassword())
		spbBuilder->insertString(&statusWrapper, isc_spb_password, password->c_str());

	if (const auto role = options.getRole())
		spbBuilder->insertString(&statusWrapper, isc_spb_sql_role_name, role->c_str());

	auto dispatcher = fbRef(master->getDispatcher());
	const auto spbBuffer = spbBuilder->getBuffer(&statusWrapper);
	const auto spbBufferLen = spbBuilder->getBufferLength(&statusWrapper);
	auto service = options.getServiceManagerName();

	if (const auto host = options.getServer())
		service = host.value() + ':' + service;

	handle.reset(dispatcher->attachServiceManager(&statusWrapper, service.c_str(), spbBufferLen, spbBuffer));
}

void ServiceManager::disconnect()
{
	detachHandle();
}

void ServiceManager::startAction(const std::vector<std::uint8_t>& spb)
{
	assert(isValid());

	StatusWrapper statusWrapper{*client};
	handle->start(&statusWrapper, static_cast<unsigned>(spb.size()), spb.data());
}

void ServiceManager::waitForCompletion(const VerboseOutput& verboseOutput, bool requestStdin)
{
	assert(isValid());

	StatusWrapper statusWrapper{*client};
	auto receiveBuilder =
		fbUnique(client->getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_RECEIVE, nullptr, 0));
	receiveBuilder->insertTag(&statusWrapper, verboseOutput ? isc_info_svc_line : isc_info_svc_to_eof);

	if (requestStdin)
		receiveBuilder->insertTag(&statusWrapper, isc_info_svc_stdin);

	const auto receiveLength = receiveBuilder->getBufferLength(&statusWrapper);
	const auto* receiveBuffer = receiveBuilder->getBuffer(&statusWrapper);

	std::vector<std::uint8_t> buffer(16u * 1024u);
	std::string pendingLine;
	unsigned stdinRequest = 0;

	for (bool running = true; running;)
	{
		auto sendBuilder =
			fbUnique(client->getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::SPB_SEND, nullptr, 0));

		if (stdinRequest)
			throw FbCppException("Service requested stdin input");

		std::fill(buffer.begin(), buffer.end(), 0);
		handle->query(&statusWrapper, sendBuilder->getBufferLength(&statusWrapper),
			sendBuilder->getBuffer(&statusWrapper), receiveLength, receiveBuffer, static_cast<unsigned>(buffer.size()),
			buffer.data());

		auto responseBuilder = fbUnique(client->getUtil()->getXpbBuilder(
			&statusWrapper, fb::IXpbBuilder::SPB_RESPONSE, buffer.data(), static_cast<unsigned>(buffer.size())));

		stdinRequest = 0;
		int outputLength = 0;
		bool notReady = false;

		for (responseBuilder->rewind(&statusWrapper); running && !responseBuilder->isEof(&statusWrapper);
			responseBuilder->moveNext(&statusWrapper))
		{
			switch (responseBuilder->getTag(&statusWrapper))
			{
				case isc_info_svc_line:
				{
					const auto* line = responseBuilder->getString(&statusWrapper);
					const auto length = static_cast<int>(responseBuilder->getLength(&statusWrapper));
					if (verboseOutput && length > 0)
						verboseOutput(std::string_view{line, static_cast<size_t>(length)});
					outputLength = length;
					break;
				}

				case isc_info_svc_to_eof:
				{
					const auto* bytes = reinterpret_cast<const char*>(responseBuilder->getBytes(&statusWrapper));
					const auto length = static_cast<int>(responseBuilder->getLength(&statusWrapper));
					emitVerboseChunk(pendingLine, std::string_view{bytes, static_cast<size_t>(length)}, verboseOutput);
					outputLength = length;
					break;
				}

				case isc_info_svc_stdin:
					stdinRequest = static_cast<unsigned>(responseBuilder->getInt(&statusWrapper));
					break;

				case isc_info_end:
					running = false;
					break;

				case isc_info_truncated:
				case isc_info_data_not_ready:
				case isc_info_svc_timeout:
					notReady = true;
					break;

				default:
					break;
			}
		}

		if (outputLength || stdinRequest || notReady)
			running = true;
	}

	if (verboseOutput && !pendingLine.empty())
		verboseOutput(pendingLine);
}

void ServiceManager::detachHandle()
{
	assert(isValid());

	StatusWrapper statusWrapper{*client};
	handle->detach(&statusWrapper);
	handle.reset();
}
