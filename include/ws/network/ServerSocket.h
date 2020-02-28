#ifndef __WS_SERVER_SOCKET_H__
#define __WS_SERVER_SOCKET_H__

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
#include <sys/event.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
constexpr int KEVENT_SIZE = 1000;
#ifndef Socket
typedef int Socket;
#endif
#endif

#include <assert.h>
#include <mutex>
#include <list>
#include <iostream>
#include <thread>
#include <functional>

#include <vector>
#include <set>
#include <map>
#include "ws/core/ByteArray.h"
#include "ws/core/ObjectPool.h"

constexpr int BUFFER_SIZE = 1024;
constexpr int ADDRESS_LENGTH = sizeof(sockaddr_in) + 16;
constexpr int NUM_ACCEPTEX = 100;

using namespace ws::core;
using namespace std::chrono;

namespace ws
{
	namespace network
	{
		class ServerSocket;
		class Client
		{
			friend class ServerSocket;
		public:
			Client();
			virtual ~Client();

			uint64_t				id;
			uint64_t				lastActiveTime;

			std::string		getIP()
			{
				char buffer[20] = {0};
				std::string result(inet_ntop(AF_INET, (void*)&addr.sin_addr, buffer, 20));
				return result;
			}
			virtual void	onRecv() = 0;
			virtual void	onDisconnected(){}
			virtual void	update(){}
			virtual void	send(const char* data, size_t length);
			virtual void	send(const ByteArray& packet);
			inline void		kick(){ isClosing = true; }

		protected:
			Socket					socket;
			sockaddr_in				addr;
			ServerSocket*			server;
			ByteArray				readBuffer;
			ByteArray				writeBuffer;
		private:
			bool					isClosing;
			bool					hasData;
			std::mutex				readerMtx;
			std::mutex				writerMtx;
		};

		struct ServerConfig
		{
			ServerConfig() :listenPort(0), maxConnection(0), numIOCPThreads(0), kickTime(0){}
			unsigned short							listenPort;
			unsigned int							maxConnection;
			unsigned char							numIOCPThreads;
			uint64_t								kickTime;
			std::function<Client*()>				createClient;
			std::function<void(Client*)>			onClientConnected;
			std::function<void(Client*)>			destroyClient;
		};

		class ServerSocket
		{
		public:
			ServerSocket();
			virtual ~ServerSocket(){}

			virtual void								init(const ServerConfig& cfg);
			virtual void								update();
			virtual void								cleanup();
			int											startListen();
			bool										kickClient(uint64_t clientID);
			Client*										getClient(uint64_t clientID);
			inline const std::map<uint64_t, Client*>&	getAllClients() const { return allClients; }
			inline size_t								numOnlines(){ return numClients; }
			inline const ServerConfig&					getConfig(){ return config; }

		protected:
			std::map<uint64_t, Client*>					allClients;

		private:
			ServerConfig								config;
			uint64_t									nextClientID;
			size_t										numClients;
			Socket										listenSocket;
			std::mutex									addMtx;
			std::list<Client*>							addingClients;

			int							processEventThread();
			Client*						addClient(Socket sock, const sockaddr_in &addr);
			void						destroyClient(Client* client);
			void						flushClient(Client* client);

#ifdef _WIN32
		public:
			inline size_t getIODataPoolSize(){ return ioDataPoolSize; }
			inline size_t getIODataPostedSize(){ return ioDataPostedSize; }

		private:
			enum SocketOperation
			{
				ACCEPT,
				RECEIVE,
				SEND,
				CLOSE_SERVER
			};

			struct OverlappedData
			{
				OVERLAPPED overlapped;
				WSABUF wsabuff;
				char buffer[BUFFER_SIZE];
				SocketOperation operationType;
				Socket acceptSocket;
			};

			HANDLE completionPort;
			LPFN_ACCEPTEX lpfnAcceptEx;

			ObjectPool<OverlappedData> ioDataPool;
			std::mutex ioDataPoolMtx;

			std::list<OverlappedData*> ioDataPosted;
			size_t ioDataPoolSize;
			size_t ioDataPostedSize;
			std::list<std::thread*> eventThreads;

			int initWinsock();
			int postAcceptEx();
			int getAcceptedSocketAddress(char* buffer, sockaddr_in* addr);
			void postCloseServer();
			void writeClientBuffer(Client* client, char* data, size_t size);

			OverlappedData* createOverlappedData(SocketOperation operation, size_t size = BUFFER_SIZE, Socket acceptedSock = NULL);
			void releaseOverlappedData(OverlappedData* data);
			void initOverlappedData(OverlappedData& data, SocketOperation operation, size_t size = BUFFER_SIZE, Socket acceptedSock = NULL);

#elif defined(__linux__)
		private:
			int epfd, pipe_fd[2];
			bool isExit;
			std::thread* eventThread;

			void readIntoBuffer(Client* client);
			void writeFromBuffer(Client* client);
			void writeClientBuffer(Client* client, char* data, size_t size);
#elif defined(__APPLE__)
        private:
            int kqfd, pipe_fd[2];
            bool isExit;
            std::thread* eventThread;
            
            bool setNonBlock(int sockfd);
            void readIntoBuffer(Client* client, uint32_t numBytes);
            void writeFromBuffer(Client* client, uint32_t numBytes);
#endif
		};
	}
}

#endif	//__SERVER_SOCKET_H__
