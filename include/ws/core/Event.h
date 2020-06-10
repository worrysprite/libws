#ifndef __WS_EVENT_H__
#define __WS_EVENT_H__

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace ws
{
	namespace core
	{
		struct Event
		{
			Event(int t = 0) :type(t){}
			virtual ~Event(){}
			int				type;
		};

		typedef std::function<void(const Event&)> EventCallback;

		struct EventListener
		{
			virtual ~EventListener() {}
			virtual void onEvent(const Event& evt) = 0;
		
			void operator()(const Event& evt) { onEvent(evt); }
		};

		class EventDispatcher
		{
		public:
			EventDispatcher() {}
			virtual ~EventDispatcher() {}

			/**
			 * 添加一次事件侦听，触发一次后自动移除侦听
			 * 对于相同的侦听函数只添加一次
			 * 会覆盖addEventListener添加的侦听回调
			 */
			virtual void once(int type, const EventCallback* callback);
			/**
			 * 添加事件侦听，不需要时必须手动移除侦听
			 * 对于相同的侦听函数只添加一次
			 * 会覆盖once添加的侦听回调
			 */
			virtual void addEventListener(int type, const EventCallback* callback);
			/**
			 * 移除once或addEventListener添加过的事件侦听
			 */
			virtual void removeEventListener(int type, const EventCallback* callback);
			virtual void dispatchEvent(const Event& event);

		protected:
			struct CallbackType
			{
				CallbackType(const EventCallback* cb) :
					callback(cb), once(false) {}

				//只要回调函数相同就判定相同
				bool operator==(const CallbackType& other) const
				{
					return callback == other.callback;
				}

				struct hash
				{
					//以回调函数作为hash
					size_t operator()(const CallbackType& cb) const
					{
						static std::hash<const void*> ptrHash;
						return ptrHash(cb.callback);
					}
				};

				const EventCallback* callback;
				bool once;
			};

			std::unordered_map<int, std::unordered_set<CallbackType, CallbackType::hash>> listeners;
		};
	}
}

#endif
