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

			void operator()(const Event& evt) { onEvent(evt); };
		};

		class EventDispatcher
		{
		public:
			EventDispatcher() {}
			virtual ~EventDispatcher() {}

			virtual void addEventListener(int type, const EventCallback* callback);
			virtual void removeEventListener(int type, const EventCallback* callback);
			virtual void dispatchEvent(const Event& event);

		protected:
			std::unordered_map<int, std::unordered_set<const EventCallback*>> listeners;
		};
	}
}

#endif
