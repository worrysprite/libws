#ifndef __WS_UTILS_TIMER_H__
#define __WS_UTILS_TIMER_H__

#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <list>

using namespace std::chrono;

namespace ws
{
	namespace core
	{
		typedef std::function<void()>	TimerCallback;

		struct Schedule final
		{
			Schedule(uint32_t idx, int32_t times, const milliseconds interval, const TimerCallback& cb) :
				index(idx), remainTimes(times), interval(interval), callback(cb) {}
			uint32_t						index = 0;
			int32_t							remainTimes = 0;	//剩余次数，-1无限
			const milliseconds				interval = 0ms;		//触发间隔
			const TimerCallback				callback;	//触发回调
		};
		using SchedulePtr = std::shared_ptr<Schedule>;

		class Timer final
		{
		public:
			using ScheduleList = std::list<SchedulePtr>;
			
			Timer() :wokerThread(std::bind(&Timer::timerProc, this)) {}
			~Timer()
			{
				isExit = true;
				wokerThread.join();
			}

			//添加计时回调
			SchedulePtr addTimeCall(milliseconds interval, const TimerCallback& callback,
				int32_t times = -1);

			//延迟调用
			inline SchedulePtr delayCall(milliseconds time, const TimerCallback& callback)
			{
				return addTimeCall(time, callback, 1);
			}

			//移除计时回调或延迟调用
			void remove(const SchedulePtr& schedule);

			void update();

		private:
			void timerProc();

			ScheduleList* findList(uint32_t index);

			static constexpr auto MIN_INTERVAL = 10ms;

			static constexpr int NEAR_FUTURE = 1 << 8;
			static constexpr int NEAR_MASK = NEAR_FUTURE - 1;
			static constexpr int NUM_FAR_WHEEL = 4;
			static constexpr int FAR_FUTURE = 1 << 6;
			static constexpr int FAR_MASK = FAR_FUTURE - 1;

			steady_clock::time_point	lastTime;

			uint32_t					tick = 0;
			
			bool						isExit = false;
			ScheduleList				nearFuture[NEAR_FUTURE];
			ScheduleList				farFuture[NUM_FAR_WHEEL][FAR_FUTURE];
			std::list<SchedulePtr>		dispatchList;
			std::list<SchedulePtr>		callList;

			std::mutex					addMtx;
			std::mutex					dispatchMtx;
			std::thread					wokerThread;
		};
	}
}

#endif