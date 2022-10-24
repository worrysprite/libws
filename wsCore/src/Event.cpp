#include "ws/core/Event.h"
#include "ws/core/Signal.h"

using namespace ws::core;

void EventDispatcher::once(int type, const EventCallback* callback, int priority)
{
	auto& set = listeners[type];
	for (auto& cb : set)
	{
		if (cb.callback == callback)	//确保回调函数唯一性
		{
			const_cast<CallbackType&>(cb).once = true;
			const_cast<CallbackType&>(cb).priority = priority;
			return;
		}
	}
	set.insert(CallbackType{ callback, priority, true });
}

void EventDispatcher::addEventListener(int type, const EventCallback* callback, int priority)
{
	auto& set = listeners[type];
	for (auto& cb : set)
	{
		if (cb.callback == callback)	//确保回调函数唯一性
		{
			const_cast<CallbackType&>(cb).once = false;
			const_cast<CallbackType&>(cb).priority = priority;
			return;
		}
	}
	set.insert(CallbackType{ callback, priority, false });
}

void EventDispatcher::removeEventListener(int type, const EventCallback* callback)
{
	auto iter = listeners.find(type);
	if (iter != listeners.end())
	{
		auto& set = iter->second;
		for (auto cbIter = set.begin(); cbIter != set.end(); ++cbIter)
		{
			if (cbIter->callback == callback)
			{
				set.erase(cbIter);
				return;
			}
		}
	}
}

void EventDispatcher::dispatchEvent(const Event& event)
{
	auto iter = listeners.find(event.type);
	if (iter != listeners.end())
	{
		auto cbList = iter->second;
		for (auto &cb : cbList)
		{
			(*cb.callback)(event);
			if (cb.once)
			{
				iter->second.erase(cb);
			}
		}
	}
}
