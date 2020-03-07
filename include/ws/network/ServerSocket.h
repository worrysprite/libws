#ifndef __WS_SERVER_SOCKET_H__
#define __WS_SERVER_SOCKET_H__

#include <assert.h>
#include <mutex>
#include <list>
#include <iostream>
#include <thread>
#include <functional>
#include <vector>
#include <set>
#include <map>

#include "ws/network/NetDef.h"
#include "ws/core/ByteArray.h"
#include "ws/core/ObjectPool.h"

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

			uint16_t				id;
			uint64_t				lastActiveTime;

			std::string		getIP()
			{
				char buffer[20] = {0};
				std::string result(inet_ntop(AF_INET, (void*)&addr.sin_addr, buffer, 20));
				return result;
			}
			virtual void	send(const void* data, size_t length);
			virtual void	send(const ByteArray& packet);
			inline void		kick(){ isClosing = true; }

		protected:
			virtual void	onRecv() = 0;
			virtual void	onDisconnected() {}

		protected:
			Socket					socket;
			sockaddr_in				addr;
			ServerSocket*			server;
			ByteArray				readerBuffer;
			ByteArray				writerBuffer;
			std::mutex				readerMtx;
			std::mutex				writerMtx;

		private:
			bool					isClosing;
			bool					hasNewData;
		};
		typedef std::shared_ptr<Client> ClientPtr;

		struct ServerConfig
		{
			ServerConfig() :listenPort(0), maxConnection(0), numIOCPThreads(0), kickTime(0){}
			std::string								listenAddr;
			uint16_t								listenPort;
			uint16_t								maxConnection;
			uint8_t									numIOCPThreads;
			uint64_t								kickTime;
			std::function<ClientPtr()>				createClient;
			std::function<void(ClientPtr)>			onClientConnected;
			std::function<void(ClientPtr)>			onClientDestroyed;
		};

		class ServerSocket
		{
		public:
			ServerSocket();
			virtual ~ServerSocket(){}

			virtual bool								init(const ServerConfig& cfg);
			virtual void								update();
			virtual void								cleanup();
			bool										startListen();
			bool										kickClient(uint32_t clientID);
			ClientPtr									getClient(uint32_t clientID);
			inline const std::map<uint16_t, ClientPtr>&	getAllClients() const { return allClients; }
			inline uint16_t								numOnlines(){ return numClients; }
			inline const ServerConfig&					getConfig(){ return config; }

		protected:
			std::map<uint16_t, ClientPtr>				allClients;

		private:
			ServerConfig								config;
			uint16_t									numClients;
			Socket										listenSocket;
			std::mutex									addMtx;
			std::list<ClientPtr>						addingClients;

			int							processEventThread();
			Client*						addClient(Socket sock, const sockaddr_in &addr);
			void						destroyClient(ClientPtr client);
			void						flushClient(ClientPtr client);
			uint16_t					getNextClientID();

#ifdef _WIN32
		public:
			inline size_t getIODataPoolSize(){ return ioDataPool.size(); }
			inline size_t getIODataPostedSize(){ return ioDataPosted.size(); }

		private:
			enum class SocketOperation
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

			std::list<std::shared_ptr<OverlappedData>> ioDataPosted;
			std::list<std::unique_ptr<std::thread>> eventThreads;

			bool initWinsock();
			int postAcceptEx();
			int getAcceptedSocketAddress(char* buffer, sockaddr_in* addr);
			void postCloseServer();
			void writeClientBuffer(Client& client, char* data, size_t size);

			std::shared_ptr<OverlappedData> createOverlappedData(SocketOperation operation, size_t size = BUFFER_SIZE, Socket acceptedSock = NULL);
			void releaseOverlappedData(OverlappedData* data);
			void initOverlappedData(OverlappedData& data, SocketOperation operation, size_t size = BUFFER_SIZE, Socket acceptedSock = NULL);

#elif defined(__linux__)
		private:
			int epfd, pipe_fd[2];
			bool isExit;
			std::unique_ptr<std::thread> eventThread;

			void readIntoBuffer(Client& client);
			void writeFromBuffer(Client& client);
			void writeClientBuffer(Client& client, char* data, size_t size);
#elif defined(__APPLE__)
        private:
            int kqfd, pipe_fd[2];
			bool isExit;
			std::unique_ptr<std::thread> eventThread;
            
            bool setNonBlock(int sockfd);
            void readIntoBuffer(Client& client, uint32_t numBytes);
            void writeFromBuffer(Client& client, uint32_t numBytes);
#endif
		};
	}
}

#endif	//__SERVER_SOCKET_H__
