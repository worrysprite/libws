#ifndef __WS_NET_DEF_H__
#define __WS_NET_DEF_H__

#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")		// win32 Socket lib
#pragma comment(lib, "Kernel32.lib")	// IOCP lib
#ifndef Socket
typedef SOCKET Socket;
#endif
#elif defined(__linux__)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>
constexpr int EPOLL_SIZE = 1000;
#ifndef Socket
typedef int Socket;
#endif
#elif defined(__APPLE__)
#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
constexpr int KEVENT_SIZE = 1000;
#ifndef Socket
typedef int Socket;
#endif
#endif

constexpr int BUFFER_SIZE = 1024;
constexpr int ADDRESS_LENGTH = sizeof(sockaddr_in) + 16;
constexpr int NUM_ACCEPTEX = 100;

#endif
