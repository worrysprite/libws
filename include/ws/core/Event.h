#pragma once
#include <functional>
#include <unordered_map>
#include <set>

namespace ws
{
	namespace core
	{
		struct Event
		{
			Event(int t = 0) :type(t) {}
			virtual ~Event() {}
			int				type;
		};

		using EventCallback = std::function<void(const Event&)>;

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
			virtual void once(int type, const EventCallback* callback, int priority = 0);
			/**
			 * 添加事件侦听，不需要时必须手动移除侦听
			 * 对于相同的侦听函数只添加一次
			 * 会覆盖once添加的侦听回调
			 */
			virtual void addEventListener(int type, const EventCallback* callback, int priority = 0);
			/**
			 * 移除once或addEventListener添加过的事件侦听
			 */
			virtual void removeEventListener(int type, const EventCallback* callback);
			virtual void dispatchEvent(const Event& event);

		protected:
			struct CallbackType
			{
				//只要回调函数相同就判定相同
				/*bool operator==(const CallbackType& other) const
				{
					return callback == other.callback;
				}*/

				bool operator<(const CallbackType& other) const
				{
					if (priority == other.priority)
					{
						return callback < other.callback;
					}
					return priority > other.priority;
				}

				const EventCallback* callback = nullptr;
				int priority = 0;
				bool once = false;
			};

			std::unordered_map<int, std::set<CallbackType>> listeners;
		};

		//provides a global instance
		inline EventDispatcher& getGlobalDispatcher()
		{
			static EventDispatcher dispatcher;
			return dispatcher;
		}
	}
}
