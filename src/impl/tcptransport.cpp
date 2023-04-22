/**
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "tcptransport.hpp"
#include "internals.hpp"

#if RTC_ENABLE_WEBSOCKET

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace rtc::impl {

TcpTransport::TcpTransport(string hostname, string service, state_callback callback)
    : Transport(nullptr, std::move(callback)), mIsActive(true), mHostname(std::move(hostname)),
      mService(std::move(service)) {

	PLOG_DEBUG << "Initializing TCP transport";
}

TcpTransport::TcpTransport(socket_t sock, state_callback callback)
    : Transport(nullptr, std::move(callback)), mIsActive(false), mSock(sock) {

	PLOG_DEBUG << "Initializing TCP transport with socket";

	// Set non-blocking
	ctl_t b = 1;
	if (::ioctlsocket(mSock, FIONBIO, &b) < 0)
		throw std::runtime_error("Failed to set socket non-blocking mode");

	// Retrieve hostname and service
	struct sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
	if (::getpeername(mSock, reinterpret_cast<struct sockaddr *>(&addr), &addrlen) < 0)
		throw std::runtime_error("getsockname failed");

	char node[MAX_NUMERICNODE_LEN];
	char serv[MAX_NUMERICSERV_LEN];
	if (::getnameinfo(reinterpret_cast<struct sockaddr *>(&addr), addrlen, node,
	                  MAX_NUMERICNODE_LEN, serv, MAX_NUMERICSERV_LEN,
	                  NI_NUMERICHOST | NI_NUMERICSERV) != 0)
		throw std::runtime_error("getnameinfo failed");

	mHostname = node;
	mService = serv;
}

TcpTransport::~TcpTransport() { stop(); }

void TcpTransport::start() {
	Transport::start();

	PLOG_DEBUG << "Starting TCP recv thread";
	mThread = std::thread(&TcpTransport::runLoop, this);
}

bool TcpTransport::stop() {
	if (!Transport::stop())
		return false;

	PLOG_DEBUG << "Waiting for TCP recv thread";
	close();
	mThread.join();
	return true;
}

bool TcpTransport::send(message_ptr message) {
	std::unique_lock lock(mSockMutex);
	if(state() == State::Connecting)
		throw std::runtime_error("Connection is not open");

	if (state() != State::Connected)
		return false;

	if (!message)
		return trySendQueue();

	PLOG_VERBOSE << "Send size=" << (message ? message->size() : 0);
	return outgoing(message);
}

void TcpTransport::incoming(message_ptr message) {
	if (!message)
		return;

	PLOG_VERBOSE << "Incoming size=" << message->size();
	recv(message);
}

bool TcpTransport::outgoing(message_ptr message) {
	// mSockMutex must be locked
	// Flush the queue, and if nothing is pending, try to send directly
	if (trySendQueue() && trySendMessage(message))
		return true;

	mSendQueue.push(message);
	mInterrupter.interrupt(); // so the thread waits for writability
	return false;
}

string TcpTransport::remoteAddress() const { return mHostname + ':' + mService; }

void TcpTransport::connect(const string &hostname, const string &service) {
	PLOG_DEBUG << "Connecting to " << hostname << ":" << service;

	struct addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_ADDRCONFIG;

	struct addrinfo *result = nullptr;
	if (getaddrinfo(hostname.c_str(), service.c_str(), &hints, &result))
		throw std::runtime_error("Resolution failed for \"" + hostname + ":" + service + "\"");

	for (auto p = result; p; p = p->ai_next) {
		try {
			connect(p->ai_addr, socklen_t(p->ai_addrlen));

			PLOG_INFO << "Connected to " << hostname << ":" << service;
			freeaddrinfo(result);
			return;

		} catch (const std::runtime_error &e) {
			if (p->ai_next) {
				PLOG_DEBUG << e.what();
			} else {
				PLOG_WARNING << e.what();
			}
		}
	}

	freeaddrinfo(result);

	std::ostringstream msg;
	msg << "Connection to " << hostname << ":" << service << " failed";
	throw std::runtime_error(msg.str());
}

void TcpTransport::connect(const sockaddr *addr, socklen_t addrlen) {
	std::unique_lock lock(mSockMutex);
	try {
		char node[MAX_NUMERICNODE_LEN];
		char serv[MAX_NUMERICSERV_LEN];
		if (getnameinfo(addr, addrlen, node, MAX_NUMERICNODE_LEN, serv, MAX_NUMERICSERV_LEN,
		                NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
			PLOG_DEBUG << "Trying address " << node << ":" << serv;
		}

		PLOG_VERBOSE << "Creating TCP socket";

		// Create socket
		mSock = ::socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
		if (mSock == INVALID_SOCKET)
			throw std::runtime_error("TCP socket creation failed");

		// Set non-blocking
		ctl_t b = 1;
		if (::ioctlsocket(mSock, FIONBIO, &b) < 0)
			throw std::runtime_error("Failed to set socket non-blocking mode");

#ifdef __APPLE__
		// MacOS lacks MSG_NOSIGNAL and requires SO_NOSIGPIPE instead
		int opt = 1;
		if (::setsockopt(mSock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt)) < 0)
			throw std::runtime_error("Failed to disable SIGPIPE for socket");
#endif

		// Initiate connection
		int ret = ::connect(mSock, addr, addrlen);
		if (ret < 0 && sockerrno != SEINPROGRESS && sockerrno != SEWOULDBLOCK) {
			std::ostringstream msg;
			msg << "TCP connection to " << node << ":" << serv << " failed, errno=" << sockerrno;
			throw std::runtime_error(msg.str());
		}

		while (true) {
			fd_set writefds;
			FD_ZERO(&writefds);
			FD_SET(mSock, &writefds);
			struct timeval tv;
			tv.tv_sec = 10; // TODO: Make the timeout configurable
			tv.tv_usec = 0;
			ret = ::select(SOCKET_TO_INT(mSock) + 1, NULL, &writefds, NULL, &tv);

			if (ret < 0) {
				if (sockerrno == SEINTR || sockerrno == SEAGAIN) // interrupted
					continue;
				else
					throw std::runtime_error("Failed to wait for socket connection");
			}

			if (ret == 0) {
				std::ostringstream msg;
				msg << "TCP connection to " << node << ":" << serv << " timed out";
				throw std::runtime_error(msg.str());
			}

			int error = 0;
			socklen_t errorlen = sizeof(error);
			if (::getsockopt(mSock, SOL_SOCKET, SO_ERROR, (char *)&error, &errorlen) != 0)
				throw std::runtime_error("Failed to get socket error code");

			if (error != 0) {
				std::ostringstream msg;
				msg << "TCP connection to " << node << ":" << serv << " failed, errno=" << error;
				throw std::runtime_error(msg.str());
			}

			PLOG_DEBUG << "TCP connection to " << node << ":" << serv << " succeeded";
			break;
		}
	} catch (...) {
		if (mSock != INVALID_SOCKET) {
			::closesocket(mSock);
			mSock = INVALID_SOCKET;
		}
		throw;
	}
}

void TcpTransport::close() {
	std::unique_lock lock(mSockMutex);
	if (mSock != INVALID_SOCKET) {
		PLOG_DEBUG << "Closing TCP socket";
		::closesocket(mSock);
		mSock = INVALID_SOCKET;
	}
	changeState(State::Disconnected);
	mInterrupter.interrupt();
}

bool TcpTransport::trySendQueue() {
	// mSockMutex must be locked
	while (auto next = mSendQueue.peek()) {
		message_ptr message = std::move(*next);
		if (!trySendMessage(message)) {
			mSendQueue.exchange(message);
			return false;
		}
		mSendQueue.pop();
	}
	return true;
}

bool TcpTransport::trySendMessage(message_ptr &message) {
	// mSockMutex must be locked
	auto data = reinterpret_cast<const char *>(message->data());
	auto size = message->size();
	while (size) {
#if defined(__APPLE__) || defined(_WIN32)
		int flags = 0;
#else
		int flags = MSG_NOSIGNAL;
#endif
		int len = ::send(mSock, data, int(size), flags);
		if (len < 0) {
			if (sockerrno == SEAGAIN || sockerrno == SEWOULDBLOCK) {
				message = make_message(message->end() - size, message->end());
				return false;
			} else {
				PLOG_ERROR << "Connection closed, errno=" << sockerrno;
				throw std::runtime_error("Connection closed");
			}
		}

		data += len;
		size -= len;
	}
	message = nullptr;
	return true;
}

void TcpTransport::runLoop() {
	const size_t bufferSize = 4096;

	// Connect
	try {
		changeState(State::Connecting);
		if (mSock == INVALID_SOCKET)
			connect(mHostname, mService);

	} catch (const std::exception &e) {
		PLOG_ERROR << "TCP connect: " << e.what();
		changeState(State::Failed);
		return;
	}

	// Receive loop
	try {
		PLOG_INFO << "TCP connected";
		changeState(State::Connected);

		while (true) {
			std::unique_lock lock(mSockMutex);
			if (mSock == INVALID_SOCKET)
				break;

			fd_set readfds, writefds;
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_SET(mSock, &readfds);
			if (!mSendQueue.empty())
				FD_SET(mSock, &writefds);

			int n = std::max(mInterrupter.prepare(readfds), SOCKET_TO_INT(mSock) + 1);

			struct timeval tv;
			tv.tv_sec = 10;
			tv.tv_usec = 0;
			lock.unlock();
			int ret = ::select(n, &readfds, &writefds, NULL, &tv);
			lock.lock();
			if (mSock == INVALID_SOCKET)
				break;

			if (ret < 0) {
				throw std::runtime_error("Failed to wait on socket");
			} else if (ret == 0) {
				PLOG_VERBOSE << "TCP is idle";
				lock.unlock(); // unlock now since the upper layer might send on incoming
				incoming(make_message(0));
				continue;
			}

			if (FD_ISSET(mSock, &writefds))
				trySendQueue();

			if (FD_ISSET(mSock, &readfds)) {
				char buffer[bufferSize];
				int len = ::recv(mSock, buffer, bufferSize, 0);
				if (len < 0) {
					if (sockerrno == SEAGAIN || sockerrno == SEWOULDBLOCK) {
						continue;
					} else {
						PLOG_WARNING << "TCP connection lost";
						break;
					}
				}

				if (len == 0)
					break; // clean close

				lock.unlock(); // unlock now since the upper layer might send on incoming
				auto *b = reinterpret_cast<byte *>(buffer);
				incoming(make_message(b, b + len));
			}
		}
	} catch (const std::exception &e) {
		PLOG_ERROR << "TCP recv: " << e.what();
	}

	PLOG_INFO << "TCP disconnected";
	changeState(State::Disconnected);
	recv(nullptr);
}

} // namespace rtc::impl

#endif
