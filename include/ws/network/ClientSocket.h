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
			ClientSocket() :sockfd(0), status(SocketStatus::DISCONNECTED),
				isExit(false), _remotePort(0), lastStatus(SocketStatus::DISCONNECTED) {}

			virtual ~ClientSocket();

			void connect(const std::string& ip, uint16_t port);
			void close();

			virtual void update();

			inline bool isConnected(){ return status == SocketStatus::CONNECTED; }
			inline const std::string& remoteIP(){ return _remoteIP; }
			inline uint16_t remotePort() { return _remotePort; }

			void send(const ByteArray& packet);
			void send(const void* data, size_t length);

			std::function<void()>			onConnected;
			std::function<void()>			onClosed;
			std::function<void(ByteArray&)>	onReceived;

		protected:
			uint16_t				_remotePort;
			std::string				_remoteIP;

			ByteArray				readerBuffer;
			ByteArray				writerBuffer;
			std::mutex				readerMtx;
			std::mutex				writerMtx;

		private:
#ifdef _WIN32
			static bool initWinsock();
#endif

			enum class SocketStatus
			{
				DISCONNECTED,
				CONNECTING,
				CONNECTED
			};
			SocketStatus					lastStatus;
			SocketStatus					status;
			Socket							sockfd;
			std::unique_ptr<std::thread>	workerThread;
			bool							isExit;

			void					workerProc();
			bool					tryToRecv();
			bool					tryToSend();
			void					reset();
		};
	}
}

#endif
