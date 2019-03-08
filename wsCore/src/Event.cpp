#include "ws/core/Event.h"
#include "ws/core/Signal.h"

using namespace ws::core;

void EventDispatcher::removeEventListener(int type, const EventCallback* callback)
{
	listeners[type].insert(callback);
}

void EventDispatcher::addEventListener(int type, const EventCallback* callback)
{
	auto iter = listeners.find(type);
	if (iter != listeners.end())
	{
		iter->second.erase(callback);
	}
}

void EventDispatcher::dispatchEvent(const Event& event)
{
	auto iter = listeners.find(event.type);
	if (iter != listeners.end())
	{
		std::unordered_set<const EventCallback*> cbList(iter->second);
		for (auto &callback : cbList)
		{
			(*callback)(event);
		}
	}
}
