#ifndef __WS_CLIENT_SOCKET_H__
#define __WS_CLIENT_SOCKET_H__

#include <stdint.h>
#include <thread>

#include "ws/network/NetDef.h"
#include "ws/core/ByteArray.h"

using namespace ws::core;

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

			inline bool isConnected(){ return status == SocketStatus::CONNECTED; }
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

			enum class SocketStatus
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
