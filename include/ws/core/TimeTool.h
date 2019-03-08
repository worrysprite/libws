#ifndef __WS_UTILS_TIME_TOOL_H__
#define __WS_UTILS_TIME_TOOL_H__

#include <stdint.h>
#include <chrono>
#include "Log.h"
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std::chrono;

namespace ws
{
	namespace core
	{
		class TimeTool
		{
		public:
			//获取时间戳
			template<class ClockType>
			inline static uint64_t getUnixtime(){ return (uint64_t)duration_cast<milliseconds>(ClockType::now().time_since_epoch()).count(); }

			//获取系统时间戳（毫秒）
			inline static uint64_t getSystemTime() { return getUnixtime<system_clock>(); }

			//把时间戳转成日期
			static void LocalTime(uint64_t time, tm& date)
			{
				time_t t = time / 1000;
#ifdef _WIN32
				localtime_s(&date, &t);
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || \
defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
				localtime_r(&t, &date);
#endif
			}

			//获取系统运行时间（毫秒）
			static uint64_t getTickCount()
			{
#ifdef _WIN32
				return GetTickCount64();
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || \
defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
				struct timespec ts;
				clock_gettime(CLOCK_MONOTONIC, &ts);
				return (ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000);
#endif
			}

			//获取时间戳当日的0点时间
			static uint64_t getZeroHourOfTime(uint64_t time = 0)
			{
				if (!time)
				{
					time = getUnixtime<system_clock>();
				}
				tm date;
				LocalTime(time, date);
				date.tm_hour = 0;
				date.tm_min = 0;
				date.tm_sec = 0;
				return mktime(&date) * 1000ULL;
			}

			//获取时间戳当月的第一天0点的时间
			static uint64_t getFirstDayOfMonth(uint64_t time = 0)
			{
				if (!time)
				{
					time = getUnixtime<system_clock>();
				}
				tm date;
				LocalTime(time, date);
				date.tm_mday = 1;
				date.tm_hour = 0;
				date.tm_min = 0;
				date.tm_sec = 0;
				return mktime(&date) * 1000ULL;
			}

			//判断一个时间是否是昨天或更早之前
			static bool isYesterdayBefore(uint64_t time, uint64_t offset = 0)
			{
				if (!time)
				{
					return true;
				}
				uint64_t now = getUnixtime<system_clock>();
				if (time >= now)
				{
					return false;
				}
				uint64_t diff = now - offset - getZeroHourOfTime(time - offset);
				return diff > 24 * 3600000;
			}

			//获取当前时间在本月的第N天
			static uint8_t								getDayofMonth(uint64_t time)
			{
				tm date = {0};
				LocalTime(time, date);
				return date.tm_mday;
			}
		};
	}
}

#endif
