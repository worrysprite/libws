#ifndef __WS_CLIENT_SOCKET_H__
#define __WS_CLIENT_SOCKET_H__

#include <stdint.h>
#include <thread>
#include <mutex>
#include <functional>

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
			ClientSocket() = default;
			ClientSocket(const ClientSocket&) = delete;	//不允许复制
			virtual ~ClientSocket();

			void connect(const std::string& ip, uint16_t port);
			void close();

			virtual void update();

			inline bool isConnected() const { return status == SocketStatus::CONNECTED; }
			inline const std::string& remoteIP() const { return _remoteIP; }
			inline uint16_t remotePort() const { return _remotePort; }

			//发送数据，会复制数据到缓冲区，不必保持数据生命周期
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
			SocketStatus					lastStatus = SocketStatus::DISCONNECTED;
			SocketStatus					status = SocketStatus::DISCONNECTED;
			Socket							sockfd = 0;
			std::unique_ptr<std::thread>	workerThread;
			bool							isExit = false;
			uint64_t						lastConnectTime = 0;

			void					workerProc();
			bool					tryToRecv();
			bool					tryToSend();
			void					reset();
		};
	}
}

#endif
