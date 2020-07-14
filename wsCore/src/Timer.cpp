#include "ws/core/Timer.h"

using namespace ws::core;

SchedulePtr Timer::addTimeCall(milliseconds interval, const TimerCallback& callback, int32_t times /*= -1*/)
{
	if (callback && times && (times == 1 || interval >= MIN_INTERVAL))
	{
		uint32_t index = (uint32_t)(tick + interval / MIN_INTERVAL);
		auto schedule = std::make_shared<Schedule>(index, times, interval, callback);
		auto list = findList(index);
		std::lock_guard<std::mutex> lock(addMtx);
		list->push_back(schedule);
		return schedule;
	}
	return nullptr;
}

void Timer::remove(const SchedulePtr& schedule)
{
	if (schedule)
	{
		std::lock_guard<std::mutex> lock(addMtx);
		schedule->remainTimes = 0;
	}
}

void Timer::update()
{
	{
		std::lock_guard<std::mutex> lock(dispatchMtx);
		callList.swap(dispatchList);
	}
	for (auto& item : callList)
	{
		item->callback();
	}
	callList.clear();
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
			auto &list = nearFuture[tick & NEAR_MASK];
			std::scoped_lock lock(addMtx, dispatchMtx);
			for (auto& item : list)
			{
				if (item->remainTimes)
				{
					dispatchList.push_back(item);
					if (item->remainTimes == -1 || --item->remainTimes)
					{
						item->index = (uint32_t)(tick + item->interval / MIN_INTERVAL);
						findList(item->index)->push_back(item);
					}
				}
			}
			list.clear();

			uint32_t wheelTick = ++tick;
			for (int i = 0; i < NUM_FAR_WHEEL; ++i)
			{
				wheelTick >>= 8;
				auto& list = farFuture[i][wheelTick & FAR_MASK];
				if (i == 0)
				{
					for (auto& item : list)
					{
						nearFuture[item->index & NEAR_MASK].push_back(item);
					}
				}
				else
				{
					uint32_t rshift = i * 6;
					auto target = farFuture[i - 1];
					for (auto& item : list)
					{
						target[(item->index >> rshift) & FAR_MASK].push_back(item);
					}
				}
				list.clear();
			}
		}
		std::this_thread::sleep_for(MIN_INTERVAL);
	}
}

Timer::ScheduleList* Timer::findList(uint32_t index)
{
	if (index < NEAR_FUTURE)
	{
		return &nearFuture[index];
	}
	else
	{
		index >>= 8;
		for (int i = 0; i < NUM_FAR_WHEEL; ++i)
		{
			if (index < FAR_FUTURE)
			{
				return &farFuture[i][index];
			}
			index >>= 6;
		}
	}
	return nullptr;
}
