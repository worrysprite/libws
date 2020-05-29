#include <functional>
#include <mutex>
#include <spdlog/spdlog.h>
#include "ws/network/ClientSocket.h"
#include "ws/core/TimeTool.h"

using namespace ws::network;

static constexpr uint64_t CONNECT_CD = 3000ULL;

#ifdef _WIN32

bool ClientSocket::initWinsock()
{
	WORD wVersionRequested = MAKEWORD(2, 2); // request WinSock lib v2.2
	WSADATA wsaData;	// Windows Socket info struct
	DWORD err = WSAStartup(wVersionRequested, &wsaData);

	if (0 != err)
	{
		spdlog::error("Request Windows Socket Library Error!");
		return false;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		spdlog::error("Request Windows Socket Version 2.2 Error!");
		return false;
	}
	return true;
}

#endif // WIN32

ClientSocket::~ClientSocket()
{
	isExit = true;
	if (workerThread)
	{
		workerThread->join();
	}
#ifdef _WIN32
	WSACleanup();
#endif // _WIN32
}

void ClientSocket::workerProc()
{
	while (!isExit)
	{
		switch (status)
		{
		case SocketStatus::CONNECTING:
		{
			uint64_t now = TimeTool::getSystemTime();
			if (now < lastConnectTime + CONNECT_CD)
			{
				break;
			}
			lastConnectTime = now;
			sockaddr_in addr;
			memset(&addr, 0, sizeof(sockaddr_in));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(_remotePort);
			inet_pton(AF_INET, _remoteIP.c_str(), &addr.sin_addr);
			if (::connect(sockfd, (sockaddr*)&addr, sizeof(sockaddr_in)) == 0)
			{
				status = SocketStatus::CONNECTED;
			}
			else
			{
				status = SocketStatus::DISCONNECTED;
			}
			break;
		}
		case SocketStatus::CONNECTED:
		{
			if (!tryToRecv())
			{
				status = SocketStatus::DISCONNECTED;
				break;
			}
			if (!tryToSend())
			{
				status = SocketStatus::DISCONNECTED;
			}
			break;
		}
        default:
            break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

bool ClientSocket::tryToRecv()
{
	fd_set set;
	FD_ZERO(&set);
	FD_SET(sockfd, &set);
	timeval time = {0, 0};
	while (select((int)sockfd + 1, &set, nullptr, nullptr, &time) > 0)
	{
		char buffer[BUFFER_SIZE];
		int length = (int)::recv(sockfd, buffer, BUFFER_SIZE, 0);
		if (length == 0 || length == -1)
		{
			return false;
		}
		readerMtx.lock();
		readerBuffer.writeData(buffer, length);
		readerMtx.unlock();
		if (length < BUFFER_SIZE)
		{
			break;
		}
	}
	return true;
}

bool ClientSocket::tryToSend()
{
	std::lock_guard<std::mutex> lock(writerMtx);
	size_t remain = writerBuffer.size();
	while (remain)
	{
		char buffer[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		int length = (int)writerBuffer.readData(buffer, BUFFER_SIZE);
		int sentLength = ::send(sockfd, buffer, length, 0);
		if (sentLength == -1)
		{
			return false;
		}
		else
		{
			if (sentLength < length)
			{
				writerBuffer.seek(sentLength - length);
			}
			remain -= sentLength;
		}
	}
	writerBuffer.truncate();
	return true;
}

void ClientSocket::connect(const std::string& ip, uint16_t port)
{
	if (!onReceived)
	{
		spdlog::error("must implements onReceived!");
		return;
	}
	if (!sockfd)
	{
#ifdef _WIN32
		initWinsock();
#endif // _WIN32
		sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}
	if (!workerThread)
	{
		workerThread = std::make_unique<std::thread>(std::bind(&ClientSocket::workerProc, this));
	}
	switch (status)
	{
	case SocketStatus::DISCONNECTED:
		_remoteIP = ip;
		_remotePort = port;
		status = lastStatus = SocketStatus::CONNECTING;
		break;
	case SocketStatus::CONNECTING:
		spdlog::debug("socket is connecting, please wait...");
		break;
	case SocketStatus::CONNECTED:
		spdlog::debug("socket is connected. you must disconnect before connect again.");
		break;
	}
}

void ClientSocket::update()
{
	if (lastStatus != status)
	{
		lastStatus = status;
		switch (status)
		{
		case SocketStatus::DISCONNECTED:
			reset();
			if (onClosed)
			{
				onClosed();
			}
			break;
		case SocketStatus::CONNECTED:
			if (onConnected)
			{
				onConnected();
			}
			break;
        default:
            break;
		}
	}
	if (status == SocketStatus::CONNECTED)
	{
		std::lock_guard<std::mutex> lock(readerMtx);
		if (readerBuffer.readAvailable())
		{
			onReceived(readerBuffer);
		}
		readerBuffer.cutHead(readerBuffer.readPosition());
	}
}

void ClientSocket::send(const ByteArray& packet)
{
	std::lock_guard<std::mutex> lock(writerMtx);
	writerBuffer.writeBytes(packet);
}

void ClientSocket::send(const void* data, size_t length)
{
	std::lock_guard<std::mutex> lock(writerMtx);
	writerBuffer.writeData(data, length);
}

void ClientSocket::close()
{
	if (isConnected())
	{
		status = SocketStatus::DISCONNECTED;
	}
}

void ClientSocket::reset()
{
#ifdef _WIN32
	closesocket(sockfd);
#elif defined(__linux__)
	::close(sockfd);
#elif defined(__APPLE__)
	::close(sockfd);
#endif
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	readerMtx.lock();
	readerBuffer.truncate();
	readerMtx.unlock();
	writerMtx.lock();
	writerBuffer.truncate();
	writerMtx.unlock();
}