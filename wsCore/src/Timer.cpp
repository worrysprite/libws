#include "ws/core/Timer.h"

using namespace ws::core;

uint32_t Timer::addTimeCall(milliseconds interval, const TimerCallback& callback, int32_t times /*= -1*/)
{
	if (callback && times && (times == 1 || interval >= MIN_INTERVAL))
	{
		auto schedule = std::make_shared<Schedule>(times, interval, callback);
		std::lock_guard<std::mutex> lock(addMtx);
		schedule->id = ++lastID;
		insert(schedule);
		scheduleList[lastID] = schedule;
		return lastID;
	}
	return 0;
}

void Timer::remove(uint32_t id)
{
	std::lock_guard<std::mutex> lock(addMtx);
	auto iter = scheduleList.find(id);
	if (iter != scheduleList.end())
	{
		iter->second->remainTimes = 0;
		iter->second->callback = nullptr;
		scheduleList.erase(iter);
	}
}

void Timer::update()
{
	ScheduleList callList;
	{
		std::lock_guard<std::mutex> lock(dispatchMtx);
		callList.swap(dispatchList);
	}
	for (auto& item : callList)
	{
		if (item->callback)
		{
			item->callback();
		}
	}
}

void Timer::timerProc()
{
	lastTime = steady_clock::now();
	while (!isExit)
	{
		auto now = steady_clock::now();
		auto delta = (now - lastTime) / MIN_INTERVAL;
		lastTime += delta * MIN_INTERVAL;
		for (int i = 0; i < delta; ++i)
		{
			dispatch();
			++tick;
			expand();
		}
		std::this_thread::sleep_for(MIN_INTERVAL);
	}
}

void Timer::expand()
{
	if ((tick & NEAR_MASK) == 0)	//进位
	{
		uint32_t farTick = tick >> 8;
		std::lock_guard<std::mutex> lock(addMtx);
		for (int i = 0; i < NUM_FAR_WHEEL; ++i)
		{
			uint32_t index = farTick & FAR_MASK;
			auto& list = farFuture[i][index];
			for (auto& item : list)
			{
				move(item, i);
			}
			list.clear();
			if (index != 0)
			{
				break;	//停止进位
			}
			farTick >>= 6;
		}
	}
}

void Timer::move(const SchedulePtr& item, int level)
{
	if (level == 0)
	{
		nearFuture[item->index & NEAR_MASK].push_front(item);
	}
	else
	{
		uint32_t rshift = (level - 1) * 6 + 8;
		uint32_t index = (item->index >> rshift) & FAR_MASK;
		if (index == 0)
		{
			move(item, level - 1);
		}
		else
		{
			farFuture[level - 1][index].push_front(item);
		}
	}
}

void Timer::dispatch()
{
	auto& list = nearFuture[tick & NEAR_MASK];
	std::scoped_lock lock(addMtx, dispatchMtx);
	for (auto& item : list)
	{
		if (item->remainTimes)
		{
			dispatchList.push_front(item);
			if (item->remainTimes == -1 || --item->remainTimes > 0)
			{
				insert(item);
			}
			else
			{
				scheduleList.erase(item->id);
			}
		}
	}
	list.clear();
}

void Timer::insert(const SchedulePtr& item)
{
	auto interval = item->interval / MIN_INTERVAL;
	item->index = uint32_t(tick + interval);
	if (interval < NEAR_FUTURE)
	{
		nearFuture[item->index & NEAR_MASK].push_front(item);
	}
	else
	{
		interval >>= 8;
		uint32_t index = item->index >> 8;
		for (int i = 0; i < NUM_FAR_WHEEL; ++i)
		{
			if (interval < FAR_FUTURE)
			{
				farFuture[i][index & FAR_MASK].push_front(item);
				break;
			}
			interval >>= 6;
			index >>= 6;
		}
	}
}