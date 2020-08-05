#ifndef __WS_UTILS_TIMER_H__
#define __WS_UTILS_TIMER_H__

#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <forward_list>

using namespace std::chrono;

namespace ws
{
	namespace core
	{
		class Timer final
		{
		public:
			using TimerCallback = std::function<void()>;

			Timer() :wokerThread(std::bind(&Timer::timerProc, this)) {}
			~Timer()
			{
				isExit = true;
				wokerThread.join();
			}

		private:
			//计划任务
			struct Schedule final
			{
				Schedule(int32_t times, const milliseconds interval, const TimerCallback& cb) :
					remainTimes(times), interval(interval), callback(cb) {}
				uint32_t						id = 0;
				uint32_t						index = 0;
				int32_t							remainTimes = 0;	//剩余次数，-1无限
				const milliseconds				interval = 0ms;		//触发间隔
				TimerCallback					callback;	//触发回调
			};
			using SchedulePtr = std::shared_ptr<Schedule>;
			using ScheduleList = std::forward_list<SchedulePtr>;

		public:
			//添加计时回调，返回一个计划id，可用于移除计时器
			uint32_t addTimeCall(milliseconds interval, const TimerCallback& callback,
				int32_t times = -1);

			//延迟调用，返回一个计划id，可用于移除计时器
			inline uint32_t delayCall(milliseconds time, const TimerCallback& callback)
			{
				return addTimeCall(time, callback, 1);
			}

			//移除计时回调或延迟调用
			void remove(uint32_t id);

			void update();

		private:
			void timerProc();
			void expand();
			void move(const SchedulePtr& item, int level);
			void dispatch();
			void insert(const SchedulePtr& item);

			static constexpr auto MIN_INTERVAL = 1ms;

			static constexpr int NEAR_FUTURE = 1 << 8;
			static constexpr int NEAR_MASK = NEAR_FUTURE - 1;
			static constexpr int NUM_FAR_WHEEL = 4;
			static constexpr int FAR_FUTURE = 1 << 6;
			static constexpr int FAR_MASK = FAR_FUTURE - 1;

			steady_clock::time_point		lastTime;

			uint32_t						tick = 0;
			uint32_t						lastID = 0;
			
			bool							isExit = false;
			ScheduleList					nearFuture[NEAR_FUTURE];
			ScheduleList					farFuture[NUM_FAR_WHEEL][FAR_FUTURE];
			ScheduleList					dispatchList;

			//已计划的定时器
			std::unordered_map<uint32_t, SchedulePtr>	scheduleList;

			std::mutex						addMtx;
			std::mutex						dispatchMtx;
			std::thread						wokerThread;
		};
	}
}

#endif