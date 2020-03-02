#include "ws/network/ServerSocket.h"
#include "ws/core/Log.h"
#include "ws/core/TimeTool.h"
#include <errno.h>
#include <string.h>

using namespace ws::network;

//===================== Client Implements ========================
// socket threads
Client::Client() : server(nullptr), socket(0), readBuffer(BUFFER_SIZE),
isClosing(false), writeBuffer(BUFFER_SIZE), hasData(false), id(0)
{
	memset(&addr, 0, sizeof(addr));
}

// socket threads
Client::~Client()
{
}

// main thread
void Client::send(const char* data, size_t length)
{
	std::lock_guard<std::mutex> lock(writerMtx);
	writeBuffer.writeData(data, length);
}

// main thread
void Client::send(const ByteArray& packet)
{
	std::lock_guard<std::mutex> lock(writerMtx);
	writeBuffer.writeBytes(packet);
}

//-----------------------windows implements start-------------------------------
#ifdef _WIN32
// main thread
ServerSocket::ServerSocket() :completionPort(nullptr), lpfnAcceptEx(nullptr), nextClientID(0),
listenSocket(0), ioDataPoolSize(0), ioDataPostedSize(0), numClients(0)
{
	
}

int ServerSocket::processEventThread()
{
	DWORD BytesTransferred;
	LPOVERLAPPED lpOverlapped;
	Client* client = NULL;
	OverlappedData* ioData = NULL;
	DWORD numBytes;
	DWORD Flags = 0;
	BOOL bRet = false;

	while (true)
	{
		bRet = GetQueuedCompletionStatus(completionPort, &BytesTransferred,
						(PULONG_PTR)&client, (LPOVERLAPPED*)&lpOverlapped, INFINITE);
		ioData = (OverlappedData*)CONTAINING_RECORD(lpOverlapped, OverlappedData, overlapped);
		if (bRet == 0)
		{
			DWORD error(GetLastError());
			if (lpOverlapped)
			{
				if (ERROR_CONNECTION_ABORTED != error)
				{
					client->isClosing = true;
					if (error != ERROR_NETNAME_DELETED)
						Log::e("GetQueuedCompletionStatus Error: %d", error);
				}
				releaseOverlappedData(ioData);
			}
			continue;
		}
		switch (ioData->operationType)
		{
		case SocketOperation::ACCEPT:
		{
			if (numClients < config.maxConnection)
			{
				sockaddr_in localAddr;
				getAcceptedSocketAddress(ioData->buffer, &localAddr);
				client = addClient(ioData->acceptSocket, localAddr);
				CreateIoCompletionPort((HANDLE)client->socket, completionPort, (ULONG_PTR)client, 0);

				initOverlappedData(*ioData, SocketOperation::RECEIVE);
				WSARecv(client->socket, &(ioData->wsabuff), 1, &numBytes, &Flags, &(ioData->overlapped), NULL);
			}
			else
			{
				shutdown(ioData->acceptSocket, SD_BOTH);
				closesocket(ioData->acceptSocket);
				releaseOverlappedData(ioData);
			}
			postAcceptEx();
			break;
		}

		case SocketOperation::RECEIVE:
		{
			if (BytesTransferred == 0)
			{
				client->isClosing = true;
				releaseOverlappedData(ioData);
			}
			else
			{
				if (client->isClosing)	// ignore if closing
				{
					releaseOverlappedData(ioData);
					continue;
				}
				writeClientBuffer(client, ioData->buffer, BytesTransferred);
				client->hasData = true;
				initOverlappedData(*ioData, SocketOperation::RECEIVE);
				WSARecv(client->socket, &(ioData->wsabuff), 1, &numBytes, &Flags, &(ioData->overlapped), NULL);
			}
			break;
		}

		case SocketOperation::SEND:
		{
			if (BytesTransferred == 0)
			{
				client->isClosing = true;
				releaseOverlappedData(ioData);
				Log::e("send error! %d", WSAGetLastError());
			}
			else
			{
				releaseOverlappedData(ioData);
			}
			break;
		}

		case SocketOperation::CLOSE_SERVER:
		{
			releaseOverlappedData(ioData);
			return 0;
		}
		}
	}
	return 0;
}	//end of processEvent

int ServerSocket::initWinsock()
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

int ServerSocket::startListen()
{
	if (-1 == initWinsock())
	{
		return -1;
	}
	// create an i/o completion port
	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == completionPort)
	{
		Log::e("CreateIoCompletionPort failed. Error: %d", GetLastError());
		return -1;
	}

	// get number of cores of cpu
	SYSTEM_INFO mySysInfo;
	GetSystemInfo(&mySysInfo);

	// create threads to process i/o completion port events
	std::function<int()> eventProc(std::bind(&ServerSocket::processEventThread, this));
	DWORD numThreads = config.numIOCPThreads;
	if (numThreads == 0)
	{
		numThreads = mySysInfo.dwNumberOfProcessors * 2;
	}
	for (DWORD i = 0; i < numThreads; ++i)
	{
		std::thread* th = new std::thread(eventProc);
		eventThreads.push_back(th);
	}

	// create listen socket
	listenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	// bind listen socket to i/o completion port
	CreateIoCompletionPort((HANDLE)listenSocket, completionPort, (ULONG_PTR)0, 0);

	// bind socket
	SOCKADDR_IN srvAddr;
	srvAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(config.listenPort);
	int result = bind(listenSocket, (SOCKADDR*)&srvAddr, sizeof(SOCKADDR));
	if (SOCKET_ERROR == result)
	{
		Log::e("Bind failed. Error: %d", GetLastError());
		return -1;
	}

	// start listen
	result = listen(listenSocket, NUM_ACCEPTEX);
	if (SOCKET_ERROR == result)
	{
		Log::e("Listen failed. Error: %d", GetLastError());
		return -1;
	}
	Log::i("server is listening port %d, waiting for clients...", config.listenPort);

	//AcceptEx function pointer
	lpfnAcceptEx = NULL;
	//AcceptEx function GUID
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	//get acceptex function pointer
	DWORD dwBytes = 0;
	result = WSAIoctl(listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx), &lpfnAcceptEx, sizeof(lpfnAcceptEx),
		&dwBytes, NULL, NULL);
	if (result != 0)
	{
		Log::e("WSAIoctl get AcceptEx function pointer failed... %d", WSAGetLastError());
		return -1;
	}

	//post acceptEx
	for (int i = 0; i < NUM_ACCEPTEX; ++i)
	{
		postAcceptEx();
	}
	return 0;
}	//end of startListen

// main thread
void ServerSocket::cleanup()
{
	// stop event threads
	for (size_t i = 0; i < eventThreads.size(); ++i)
	{
		postCloseServer();
	}
	for (auto th : eventThreads)
	{
		th->join();
		Log::d("server socket event thread joined");
		delete th;
	}
	eventThreads.clear();
	// disconnect all clients
	for (auto client : allClients)
	{
		client.second->isClosing = true;
		destroyClient(client.second);
	}
	allClients.clear();
	numClients = 0;
	// clear all caches
	for (auto ioData : ioDataPool)
	{
		if (ioData->acceptSocket)
		{
			closesocket(ioData->acceptSocket);
		}
	}
	for (auto ioData : ioDataPosted)
	{
		if (ioData->acceptSocket)
		{
			closesocket(ioData->acceptSocket);
		}
		delete ioData;
	}
	shutdown(listenSocket, SD_BOTH);
	closesocket(listenSocket);
	CloseHandle(completionPort);
	WSACleanup();
} // end of cleanup

// main thread
void ServerSocket::flushClient(Client* client)
{
	if (!client || client->isClosing)
	{
		return;
	}
	std::lock_guard<std::mutex> writeLock(client->writerMtx);
	client->writeBuffer.readPosition(0);
	size_t remain = client->writeBuffer.size();
	if (!remain)
	{
		return;
	}

	while (remain > 0)
	{
		OverlappedData* sendData = createOverlappedData(SocketOperation::SEND);
		size_t length = client->writeBuffer.readData(sendData->buffer, BUFFER_SIZE);
		sendData->wsabuff.len = (ULONG)length;
		WSASend(client->socket, &(sendData->wsabuff), 1, NULL, 0, &(sendData->overlapped), NULL);
		if (length >= remain)
		{
			break;
		}
		remain = client->writeBuffer.readAvailable();
	}
	client->writeBuffer.truncate();
}

// main thread and socket threads
ServerSocket::OverlappedData* ServerSocket::createOverlappedData(SocketOperation operation, size_t size /*= BUFFER_SIZE*/, SOCKET acceptedSock /*= NULL*/)
{
	OverlappedData* ioData = nullptr;
	std::lock_guard<std::mutex> lock(ioDataPoolMtx);
	ioData = ioDataPool.alloc();
	ioData->wsabuff.buf = ioData->buffer;
	initOverlappedData(*ioData, operation, size, acceptedSock);
	ioDataPosted.push_back(ioData);
	ioDataPoolSize = ioDataPool.size();
	ioDataPostedSize = ioDataPosted.size();
	return ioData;
}

// socket threads
void ServerSocket::releaseOverlappedData(OverlappedData* data)
{
	std::lock_guard<std::mutex> lock(ioDataPoolMtx);
	ioDataPosted.remove(data);
	ioDataPool.free(data);
	ioDataPoolSize = ioDataPool.size();
	ioDataPostedSize = ioDataPosted.size();
}

void ServerSocket::initOverlappedData(OverlappedData& data, SocketOperation operation, size_t size /*= BUFFER_SIZE*/, SOCKET acceptedSock /*= NULL*/)
{
	memset(&(data.overlapped), 0, sizeof(OVERLAPPED));
	memset(data.buffer, 0, BUFFER_SIZE);
	data.wsabuff.len = (ULONG)size;
	data.operationType = operation;
	data.acceptSocket = acceptedSock;
}

// main thread and socket threads
int ServerSocket::postAcceptEx()
{
	SOCKET acceptSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	OverlappedData* ioData = createOverlappedData(SocketOperation::ACCEPT, BUFFER_SIZE, acceptSocket);
	
	DWORD dwBytes = 0;
	int result = lpfnAcceptEx(listenSocket, acceptSocket, ioData->buffer, 0,
								ADDRESS_LENGTH, ADDRESS_LENGTH, &dwBytes, &(ioData->overlapped));
	if (result == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
	{
		Log::e("lpfnAcceptEx error.. %d", WSAGetLastError());
		return -1;
	}
	return 0;
}

// socket threads
int ServerSocket::getAcceptedSocketAddress(char* buffer, sockaddr_in* addr)
{
	//GetAcceptExSockAddrs function pointer
	LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs = nullptr;
	//GetAcceptExSockAddrs function GUID
	GUID guidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	//get GetAcceptExSockAddrs function pointer
	DWORD dwBytes = 0;
	int result = WSAIoctl(listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptExSockAddrs, sizeof(guidGetAcceptExSockAddrs), &lpfnGetAcceptExSockAddrs, sizeof(lpfnGetAcceptExSockAddrs),
		&dwBytes, NULL, NULL);
	if (result == 0)
	{
		sockaddr* localAddr = nullptr;
		int localAddrLength = 0;
		sockaddr* remoteAddr = nullptr;
		int remoteAddrLength;
		lpfnGetAcceptExSockAddrs(buffer, 0, ADDRESS_LENGTH, ADDRESS_LENGTH, &localAddr, &localAddrLength, &remoteAddr, &remoteAddrLength);
		memcpy(addr, remoteAddr, localAddrLength);
		return 0;
	}
	else
	{
		return -1;
	}
}

// main thread
void ServerSocket::postCloseServer()
{
	OverlappedData* ioData = createOverlappedData(SocketOperation::CLOSE_SERVER);
	PostQueuedCompletionStatus(completionPort, 0, listenSocket, &(ioData->overlapped));
}

// main thread
void ServerSocket::destroyClient(Client* client)
{
	shutdown(client->socket, SD_BOTH);
	closesocket(client->socket);
	client->onDisconnected();
	client->server = nullptr;
	config.destroyClient(client);
}

// socket thread
void ServerSocket::writeClientBuffer(Client* client, char* data, size_t size)
{
	std::lock_guard<std::mutex> lock(client->readerMtx);
	client->readBuffer.writeData(data, size);
}

//-----------------------linux implements start-------------------------------
#elif defined(__linux__)
// main thread
ServerSocket::ServerSocket() :
isExit(false), eventThread(nullptr), epfd(0), nextClientID(0), listenSocket(0)
{
	
}

int ServerSocket::processEventThread()
{
	epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
	epoll_event events[EPOLL_SIZE];
	sockaddr clientAddr;
	socklen_t addrlen = sizeof(sockaddr);
	while (!isExit)
	{
		int eventCount = epoll_wait(epfd, events, EPOLL_SIZE, -1);
		if (eventCount == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			Log::e("epoll wait error=%s", strerror(errno));
			return -1;
		}
		for (int i = 0; i < eventCount; ++i)
		{
			epoll_event& evt(events[i]);
			if (evt.events & EPOLLERR || evt.events & EPOLLHUP)
			{
				// handle errors
			}
			if (evt.events & EPOLLIN)
			{
				if (evt.data.fd == listenSocket)
				{
					while (true)
					{
						Socket acceptedSocket = accept4(listenSocket, &clientAddr, &addrlen, SOCK_NONBLOCK);
						if (acceptedSocket == -1)
						{
							if (errno != EAGAIN && errno != EWOULDBLOCK)
							{
								Log::e("accept error: %s", strerror(errno));
							}
							break;
						}
						if (numClients < config.maxConnection)
						{
							ev.data.ptr = addClient(acceptedSocket, (sockaddr_in&)clientAddr);
							epoll_ctl(epfd, EPOLL_CTL_ADD, acceptedSocket, &ev);
						}
						else
						{
							close(acceptedSocket);
						}
					}
				}
				else if (evt.data.fd != pipe_fd[0])
				{
					Client* client = (Client*)evt.data.ptr;
					readIntoBuffer(client);
				}
			}
			if (evt.events & EPOLLOUT)
			{
				Client* client = (Client*)evt.data.ptr;
				writeFromBuffer(client);
			}
			if (evt.events & EPOLLRDHUP)
			{
				Client* client = (Client*)evt.data.ptr;
				client->isClosing = true;
			}
		}
	}
	return 0;
}	//end of processEvent

int ServerSocket::startListen()
{
	listenSocket = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listenSocket < 0)
	{
		Log::e("create listen socket error.");
		return -1;
	}

	sockaddr_in srvAddr;
	srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(config.listenPort);

	int optval = 1;
	if(setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int))!=0)
	{
		Log::e("setsockopt error. errno=%d", errno);
		return -1;
	}

	int result = bind(listenSocket, (sockaddr*)&srvAddr, sizeof(srvAddr));
	if (result < 0)
	{
		Log::e("bind port %d error. errno=%d", config.listenPort, errno);
		return -1;
	}
	result = listen(listenSocket, 100);
	if (result < 0)
	{
		Log::e("listen port %d error.", config.listenPort);
		return -1;
	}
	Log::i("server is listening port %d, waiting for clients...", config.listenPort);

	epfd = epoll_create(EPOLL_SIZE);
	if (epfd < 0)
	{
		return -1;
	}
	epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listenSocket;
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenSocket, &ev);

	if (-1 == pipe(pipe_fd))
	{
		Log::e("create pipe error: %s", strerror(errno));
		return -1;
	}
	ev.data.fd = pipe_fd[0];
	epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_fd[0], &ev);

	// create threads to process epoll events
	std::function<int()> eventProc(std::bind(&ServerSocket::processEventThread, this));
	eventThread = new std::thread(eventProc);
	return 0;
}	//end of startListen

// main thread
void ServerSocket::cleanup()
{
	// stop event threads
	isExit = true;
	const char exitCode[] = "0";
	write(pipe_fd[1], exitCode, sizeof(exitCode));
	eventThread->join();
	Log::d("server socket event thread joined");
	delete eventThread;
	eventThread = NULL;
	close(pipe_fd[0]);
	close(pipe_fd[1]);
	// disconnect all clients
	for (auto client : allClients)
	{
		client.second->isClosing = true;
		destroyClient(client.second);
	}
	allClients.clear();
	numClients = 0;
	// close listen port
	shutdown(listenSocket, SHUT_RDWR);
	close(listenSocket);
}

// main thread
void ServerSocket::flushClient(Client* client)
{
	writeFromBuffer(client);
}

// socket thread
void ServerSocket::readIntoBuffer(Client* client)
{
	if (client->isClosing)
	{
		return;
	}
	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);
	int length = recv(client->socket, buffer, BUFFER_SIZE, 0);
	if (length == 0 || (length == -1 && errno != EWOULDBLOCK && errno != EAGAIN))
	{
		client->isClosing = true;
		return;
	}
	while (length > 0)
	{
		writeClientBuffer(client, buffer, length);
		if (length < BUFFER_SIZE)
		{
			break;
		}
		length = recv(client->socket, buffer, BUFFER_SIZE, 0);
		if (length == 0 || (length == -1 && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			client->isClosing = true;
		}
	}
	client->hasData = true;
}

// main thread and socket thread
void ServerSocket::writeFromBuffer(Client* client)
{
	if (client->isClosing)
	{
		return;
	}
	client->writeBuffer.lock();
	size_t remain = client->writeBuffer.size();
	if (!remain)
	{
		client->writeBuffer.unlock();
		return;
	}
	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);
	client->writeBuffer.position = 0;
	while (remain > 0)
	{
		size_t length = client->writeBuffer.readObject(buffer, BUFFER_SIZE);
		long sentLength = send(client->socket, buffer, length, 0);
		if (sentLength == -1)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				client->writeBuffer.position -= length;
				break;
			}
			else
			{
				client->writeBuffer.unlock();
				client->isClosing = true;
				return;
			}
		}
		if (sentLength < (long)length)	// tcp buffer full, stop write and wait epoll notify
		{
			client->writeBuffer.position -= length - sentLength;
			break;
		}
		if (length >= remain)
		{
			break;
		}
		remain = client->writeBuffer.available();
	}
	client->writeBuffer.cutHead(client->writeBuffer.position);
	client->writeBuffer.unlock();
}

// main thread
void ServerSocket::destroyClient(Client* client)
{
	shutdown(client->socket, SHUT_RDWR);
	if (-1 == close(client->socket))
	{
		Log::e("close socket error: %s", strerror(errno));
	}
	client->onDisconnected();
	client->server = nullptr;
	config.destroyClient(client);
}

// socket thread
void ServerSocket::writeClientBuffer(Client* client, char* data, size_t size)
{
	client->readBuffer.lock();
	size_t oldPosition = client->readBuffer.position;
	// write at the end
	client->readBuffer.position = client->readBuffer.size();
	client->readBuffer.writeObject(data, size);
	client->readBuffer.position = oldPosition;
	client->readBuffer.unlock();
}

#elif defined(__APPLE__)
// main thread
ServerSocket::ServerSocket() :
isExit(false), eventThread(nullptr), kqfd(0), nextClientID(0), listenSocket(0)
{
    
}

int ServerSocket::processEventThread()
{
    struct kevent ev_set[3];
    struct kevent events[KEVENT_SIZE];
    sockaddr clientAddr;
    socklen_t addrlen = sizeof(sockaddr);
    while (!isExit)
    {
        int eventCount = kevent(kqfd, nullptr, 0, events, KEVENT_SIZE, nullptr);
        if (eventCount == -1)
        {
            Log::e("kqueue wait error=%s", strerror(errno));
            return -1;
        }
        for (int i = 0; i < eventCount; ++i)
        {
            struct kevent& evt(events[i]);
            switch (evt.filter)
            {
            case EVFILT_EXCEPT:
            {
                // handle errors
                Log::e("socket exception");
                Client* client = (Client*)evt.udata;
                client->isClosing = true;
                break;
            }
            case EVFILT_READ:
                if (evt.ident == listenSocket)
                {
                    int numAccepted = (int)evt.data;
                    for (int j = 0; j < numAccepted; ++j)
                    {
                        Socket acceptedSocket = accept(listenSocket, &clientAddr, &addrlen);
                        if (acceptedSocket == -1)
                        {
                            if (errno != EAGAIN && errno != EWOULDBLOCK)
                            {
                                Log::e("accept error: %s", strerror(errno));
                            }
                            break;
                        }
                        if (numClients < config.maxConnection)
                        {
                            //setNonBlock(acceptedSocket);
                            Client* client = addClient(acceptedSocket, (sockaddr_in&)clientAddr);
                            EV_SET(&ev_set[0], acceptedSocket, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, client);
                            EV_SET(&ev_set[1], acceptedSocket, EVFILT_WRITE, EV_ADD|EV_DISABLE, 0, 0, client);
                            EV_SET(&ev_set[2], acceptedSocket, EVFILT_EXCEPT, EV_ADD|EV_ENABLE, 0, 0, client);
                            if (kevent(kqfd, ev_set, 3, nullptr, 0, nullptr) < 0)
                            {
                                Log::e("kevent add client error.");
                            }
                        }
                        else
                        {
                            close(acceptedSocket);
                        }
                    }
                }
                else
                {
                    readIntoBuffer((Client*)evt.udata, (uint32_t)evt.data);
                }
                break;
            case EVFILT_WRITE:
                writeFromBuffer((Client*)evt.udata, (uint32_t)evt.data);
                break;
            }
        }
    }
    return 0;
}	//end of processEvent

int ServerSocket::startListen()
{
    listenSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0)
    {
        Log::e("create listen socket error.");
        return -1;
    }
    
    struct sockaddr_in srvAddr;
    srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons(config.listenPort);
    
    int optval = 1;
    if(setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int))!=0)
    {
        Log::e("setsockopt error. errno=%d", errno);
        return -1;
    }
    if (!setNonBlock(listenSocket))
    {
        Log::e("set nonblock error.");
        return -1;
    }
    
    int result = bind(listenSocket, (sockaddr*)&srvAddr, sizeof(srvAddr));
    if (result < 0)
    {
        Log::e("bind port %d error. errno=%d", config.listenPort, errno);
        return -1;
    }
    result = listen(listenSocket, 100);
    if (result < 0)
    {
        Log::e("listen port %d error.", config.listenPort);
        return -1;
    }
    Log::i("server is listening port %d, waiting for clients...", config.listenPort);
    kqfd = kqueue();
    if (kqfd < 0)
    {
        return -1;
    }
    struct kevent evt;
    EV_SET(&evt, listenSocket, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (void*)(intptr_t)listenSocket);
    if (kevent(kqfd, &evt, 1, nullptr, 0, nullptr) < 0)
    {
        Log::e("kevent add listen socket error.");
        return -1;
    }
    if (-1 == pipe(pipe_fd))
    {
        Log::e("create pipe error: %s", strerror(errno));
        return -1;
    }
    EV_SET(&evt, pipe_fd[0], EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, nullptr);
    if (kevent(kqfd, &evt, 1, nullptr, 0, nullptr) < 0)
    {
        Log::e("kevent add pipe error.");
        return -1;
    }
    
    // create threads to process epoll events
    std::function<int()> eventProc(std::bind(&ServerSocket::processEventThread, this));
    eventThread = new std::thread(eventProc);
    return 0;
}	//end of startListen

// main thread
void ServerSocket::cleanup()
{
    // stop event threads
    isExit = true;
    const char exitCode[] = "0";
    write(pipe_fd[1], exitCode, sizeof(exitCode));
    eventThread->join();
    Log::d("server socket event thread joined");
    delete eventThread;
    eventThread = NULL;
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    // disconnect all clients
    for (auto client : allClients)
    {
        client.second->isClosing = true;
        destroyClient(client.second);
    }
    allClients.clear();
    numClients = 0;
    // close listen port
    shutdown(listenSocket, SHUT_RDWR);
    close(listenSocket);
}

// socket thread
void ServerSocket::readIntoBuffer(Client* client, uint32_t numBytes)
{
    if (client->isClosing)
    {
        return;
    }
    ByteArray& bytes = client->readBuffer;
    bytes.lock();
    size_t oldSize = bytes.size();
    bytes.expand(numBytes, oldSize);
    ssize_t length = recv(client->socket, bytes.getReaderPointer(), numBytes, 0);
	bytes.setContentSize(oldSize + numBytes);
    bytes.unlock();
    if (length != numBytes)
    {
        Log::e("recv data error!");
    }
    client->hasData = true;
}

// main thread
void ServerSocket::flushClient(Client* client)
{
    client->writeBuffer.lock();
    if (client->writeBuffer.size())
    {
        struct kevent evt;
        EV_SET(&evt, client->socket, EVFILT_WRITE, EV_ENABLE, 0, 0, client);
    }
	client->writeBuffer.unlock();
}

// socket thread
void ServerSocket::writeFromBuffer(Client* client, uint32_t numBytes)
{
    if (client->isClosing)
    {
        return;
    }
    ByteArray& bytes = client->writeBuffer;
    bytes.lock();
    size_t size = bytes.size();
    if (!size)
    {
        bytes.unlock();
        return;
    }
    if (size > numBytes)
    {
        size = numBytes;
    }
    ssize_t length = send(client->socket, bytes.getBytes(), size, 0);
    if (length < 0 || length != numBytes)
    {
        Log::e("send data error!");
    }
    bytes.cutHead(numBytes);
    if (!bytes.size())
    {
        struct kevent evt;
        EV_SET(&evt, client->socket, EVFILT_WRITE, EV_DISABLE, 0, 0, client);
    }
    bytes.unlock();
}

// main thread and socket thread
bool ServerSocket::setNonBlock(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0)
    {
        Log::d("fcntl get failed");
        return false;
    }
    int r = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    if (r < 0)
    {
        Log::d("fcntl set failed");
        return false;
    }
    return true;
}

// main thread
void ServerSocket::destroyClient(Client* client)
{
    shutdown(client->socket, SHUT_RDWR);
    if (-1 == close(client->socket))
    {
        Log::e("close socket error: %s", strerror(errno));
    }
    client->onDisconnected();
    client->server = nullptr;
    config.destroyClient(client);
}

#endif

// main thread
void ServerSocket::init(const ServerConfig& cfg)
{
	config = cfg;
	assert(config.createClient != nullptr);
	assert(config.destroyClient != nullptr);
	if (!config.maxConnection)
	{
		config.maxConnection = 1000;
	}
}

// main thread
void ServerSocket::update()
{
	addMtx.lock();
	if (addingClients.size() > 0)
	{
		for (auto client : addingClients)
		{
			if (config.onClientConnected)
			{
				config.onClientConnected(client);
			}
			allClients.insert(std::make_pair(client->id, client));
		}
		addingClients.clear();
	}
	addMtx.unlock();
	uint64_t now = TimeTool::getTickCount();
	for (auto iter = allClients.begin(); iter != allClients.end();)
	{
		auto client = iter->second;
		if (client->hasData)
		{
			client->hasData = false;
			client->lastActiveTime = now;
			client->onRecv();
		}
		else if (now < client->lastActiveTime)	// GetTickCount was reset after 49.7 days
		{
			client->lastActiveTime = now;
		}
		else if (config.kickTime && now - client->lastActiveTime > config.kickTime)
		{
			client->isClosing = true;
		}
		client->update();
		flushClient(client);
		if (client->isClosing)
		{
			iter = allClients.erase(iter);
			destroyClient(client);
		}
		else
		{
			++iter;
		}
	}
	numClients = allClients.size();
}

// socket threads
Client* ServerSocket::addClient(Socket sock, const sockaddr_in &addr)
{
	Client* client = config.createClient();
	client->server = this;
	client->lastActiveTime = TimeTool::getTickCount();
	client->socket = sock;
	client->addr = addr;

	addMtx.lock();
	client->id = ++nextClientID;
	addingClients.push_back(client);
	addMtx.unlock();
	return client;
}

// main thread
Client* ServerSocket::getClient(uint64_t clientID)
{
	Client* cs(nullptr);
	auto iter = allClients.find(clientID);
	if (iter != allClients.end())
	{
		cs = iter->second;
	}
	return cs;
}

// main thread
bool ServerSocket::kickClient(uint64_t clientID)
{
	Client* client = getClient(clientID);
	if (!client)
	{
		return false;
	}
	client->isClosing = true;
	return true;
}
