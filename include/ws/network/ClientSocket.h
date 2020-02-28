#ifndef __WS_CLIENT_SOCKET_H__
#define __WS_CLIENT_SOCKET_H__

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
#define EPOLL_SIZE 1000
#ifndef Socket
typedef int Socket;
#endif
#elif defined(__APPLE__)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#ifndef Socket
typedef int Socket;
#endif
#endif
#include <stdint.h>
#include <thread>
#include "ws/core/ByteArray.h"

using namespace ws::core;

constexpr int BUFFER_SIZE = 1024;

namespace ws
{
	namespace network
	{
		class ClientSocket
		{
		public:
			ClientSocket();
			virtual ~ClientSocket();

			void connect(const std::string& ip, uint16_t port);
			void close();

			virtual void update();

			inline bool isConnected(){ return status == CONNECTED; }
			inline const std::string& remoteIP(){ return _remoteIP; }
			inline uint16_t remotePort(){ return _remotePort; }

			//void read();
			//void write();

		protected:
			uint16_t				_remotePort;
			std::string				_remoteIP;

			ByteArray				readBuffer;
			ByteArray				writeBuffer;
			std::mutex				readerMtx;
			std::mutex				writerMtx;

			virtual void onConnected(){}
			virtual void onClosed(){}

			void send(const ByteArray& packet);
			void send(const char* data, size_t length);

		private:
#ifdef _WIN32
			static int initWinsock();
#endif

			enum SocketStatus
			{
				DISCONNECTED,
				CONNECTING,
				CONNECTED
			};
			SocketStatus			lastStatus;
			SocketStatus			status;
			Socket					sockfd;
			std::thread*			workerThread;
			bool					isExit;

			void					workerProc();
			bool					tryToRecv();
			bool					tryToSend();
			void					reset();
		};
	}
}

#endif
