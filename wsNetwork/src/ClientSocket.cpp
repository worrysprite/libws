#include <functional>
#include "ws/network/ClientSocket.h"
#include "ws/core/Log.h"

using namespace ws::network;

#ifdef _WIN32

ClientSocket::ClientSocket() :sockfd(0), workerThread(nullptr), status(SocketStatus::DISCONNECTED),
isExit(false), _remotePort(0), lastStatus(SocketStatus::DISCONNECTED)
{
	initWinsock();
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	workerThread = new std::thread(std::bind(&ClientSocket::workerProc, this));
}

ClientSocket::~ClientSocket()
{
	isExit = true;
	workerThread->join();
	delete workerThread;
	WSACleanup();
}

int ClientSocket::initWinsock()
{
	WORD wVersionRequested = MAKEWORD(2, 2); // request WinSock lib v2.2
	WSADATA wsaData;	// Windows Socket info struct
	DWORD err = WSAStartup(wVersionRequested, &wsaData);
	if (0 != err)
	{
		Log::e("Request Windows Socket Library Error!");
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		Log::e("Request Windows Socket Version 2.2 Error!");
		return -1;
	}
	return 0;
}

void ClientSocket::reset()
{
	closesocket(sockfd);
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	readerMtx.lock();
	readBuffer.truncate();
	readerMtx.unlock();
	writerMtx.lock();
	writeBuffer.truncate();
	writerMtx.unlock();
}

#elif defined(__linux__)

ClientSocket::ClientSocket() :sockfd(0), workerThread(nullptr), status(DISCONNECTED),
isExit(false), _remotePort(0), lastStatus(DISCONNECTED)
{
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	workerThread = new std::thread(std::bind(&ClientSocket::workerProc, this));
}

ClientSocket::~ClientSocket()
{
	isExit = true;
	workerThread->join();
	delete workerThread;
}

void ClientSocket::reset()
{
	::close(sockfd);
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	readBuffer.lock();
	readBuffer.truncate();
	readBuffer.unlock();
	writeBuffer.lock();
	writeBuffer.truncate();
	writeBuffer.unlock();
}

#elif defined(__APPLE__)
ClientSocket::ClientSocket() :sockfd(0), workerThread(nullptr), status(DISCONNECTED),
isExit(false), _remotePort(0), lastStatus(DISCONNECTED)
{
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    workerThread = new std::thread(std::bind(&ClientSocket::workerProc, this));
}

ClientSocket::~ClientSocket()
{
    isExit = true;
    workerThread->join();
    delete workerThread;
}

void ClientSocket::reset()
{
    ::close(sockfd);
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    readBuffer.lock();
    readBuffer.truncate();
    readBuffer.unlock();
    writeBuffer.lock();
    writeBuffer.truncate();
    writeBuffer.unlock();
}

#endif // WIN32

void ClientSocket::workerProc()
{
	while (!isExit)
	{
		switch (status)
		{
		case SocketStatus::CONNECTING:
		{
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
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
		readBuffer.writeData(buffer, length);
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
	size_t remain = writeBuffer.size();
	while (remain)
	{
		char buffer[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		int length = (int)writeBuffer.readData(buffer, BUFFER_SIZE);
		int sentLength = ::send(sockfd, buffer, length, 0);
		if (sentLength == -1)
		{
			return false;
		}
		else
		{
			if (sentLength < length)
			{
				writeBuffer.seek(sentLength - length);
			}
			remain -= sentLength;
		}
	}
	writeBuffer.truncate();
	return true;
}

void ClientSocket::connect(const std::string& ip, uint16_t port)
{
	switch (status)
	{
	case SocketStatus::DISCONNECTED:
		_remoteIP = ip;
		_remotePort = port;
		status = lastStatus = SocketStatus::CONNECTING;
		break;
	case SocketStatus::CONNECTING:
		Log::d("socket is connecting, please wait...");
		break;
	case SocketStatus::CONNECTED:
		Log::d("socket is connected. you must disconnect before connect again.");
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
			onClosed();
			break;
		case SocketStatus::CONNECTED:
			onConnected();
			break;
        default:
            break;
		}
	}
}

void ClientSocket::send(const ByteArray& packet)
{
	std::lock_guard<std::mutex> lock(writerMtx);
	writeBuffer.writeBytes(packet);
}

void ClientSocket::send(const char* data, size_t length)
{
	std::lock_guard<std::mutex> lock(writerMtx);
	writeBuffer.writeData(data, length);
}

void ClientSocket::close()
{
	if (isConnected())
	{
		status = SocketStatus::DISCONNECTED;
	}
}
