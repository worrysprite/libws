#include "ws/core/Event.h"
#include "ws/core/Signal.h"

using namespace ws::core;

void EventDispatcher::once(int type, const EventCallback* callback)
{
	auto& cb = *listeners[type].emplace(callback).first;
	const_cast<CallbackType&>(cb).once = true;	//once属性不影响hash
}

void EventDispatcher::addEventListener(int type, const EventCallback* callback)
{
	auto& cb = *listeners[type].emplace(callback).first;
	const_cast<CallbackType&>(cb).once = false;	//once属性不影响hash
}

void EventDispatcher::removeEventListener(int type, const EventCallback* callback)
{
	auto iter = listeners.find(type);
	if (iter != listeners.end())
	{
		iter->second.erase(CallbackType(callback));
	}
}

void EventDispatcher::dispatchEvent(const Event& event)
{
	auto iter = listeners.find(event.type);
	if (iter != listeners.end())
	{
		std::unordered_set<CallbackType, CallbackType::hash> cbList(iter->second);
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