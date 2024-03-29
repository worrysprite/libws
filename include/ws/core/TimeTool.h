#pragma once
#include <stdint.h>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#endif

namespace ws
{
	namespace core
	{
		namespace TimeTool
		{
			//获取时间戳
			template<class ClockType> requires std::chrono::is_clock_v<ClockType>
			inline time_t getUnixtime() { return std::chrono::duration_cast<std::chrono::milliseconds>(ClockType::now().time_since_epoch()).count(); }

			//获取系统时间戳（毫秒）
			inline time_t getSystemTime() { return getUnixtime<std::chrono::system_clock>(); }

			//把时间戳转成本地日期
			inline void LocalTime(time_t t, tm& date)
			{
#ifdef _WIN32
				localtime_s(&date, &t);
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || \
defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
				localtime_r(&t, &date);
#endif
			}

			//把时间戳转成UTC日期
			inline void GMTime(time_t t, tm& date)
			{
#ifdef _WIN32
				gmtime_s(&date, &t);
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || \
defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
				gmtime_r(&t, &date);
#endif
			}

			//获取系统运行时间（毫秒）
			inline uint64_t getTickCount()
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

			//获取时间戳当日的0点时间（本地时区）
			inline uint64_t getZeroHourOfTime(uint64_t time = 0)
			{
				if (!time)
				{
					time = getSystemTime();
				}
				tm date;
				LocalTime(time / 1000, date);
				date.tm_hour = 0;
				date.tm_min = 0;
				date.tm_sec = 0;
				return mktime(&date) * 1000ULL;
			}

			//获取时间戳本周的第一天0点的时间（本地时区），startOfWeek=N表示周N算作每周第一天（0=周日）
			inline uint64_t getFirstDayOfWeek(uint64_t time = 0, uint32_t startOfWeek = 0)
			{
				if (!time)
				{
					time = getSystemTime();
				}
				tm date;
				LocalTime(time / 1000, date);

				//计算往前推移多少天
				int offset = date.tm_wday - int(startOfWeek % 7);
				if (offset < 0)
					offset += 7;

				//获取前offset天的0点
				time -= offset * 86400000ULL;
				LocalTime(time / 1000, date);
				date.tm_hour = 0;
				date.tm_min = 0;
				date.tm_sec = 0;
				return mktime(&date) * 1000ULL;
			}

			//获取时间戳本月的第一天0点的时间（本地时区）
			inline uint64_t getFirstDayOfMonth(uint64_t time = 0)
			{
				if (!time)
				{
					time = getSystemTime();
				}
				tm date;
				LocalTime(time / 1000, date);
				date.tm_mday = 1;
				date.tm_hour = 0;
				date.tm_min = 0;
				date.tm_sec = 0;
				return mktime(&date) * 1000ULL;
			}

			//判断一个时间是否是昨天或更早之前
			inline bool isYesterdayBefore(uint64_t time, uint64_t offset = 0)
			{
				if (!time)
				{
					return true;
				}
				uint64_t now = getSystemTime();
				if (time >= now)
				{
					return false;
				}
				uint64_t diff = now - offset - getZeroHourOfTime(time - offset);
				return diff > 24 * 3600000;
			}

			//获取当前时间在本周的第N天（0-6）0=Sunday
			inline int getDayOfWeek(uint64_t time = 0)
			{
				if (!time)
					time = getSystemTime();
				tm date = { 0 };
				LocalTime(time / 1000, date);
				return date.tm_wday;
			}

			//获取当前时间在本月的第N天（1-31）
			inline int getDayOfMonth(uint64_t time = 0)
			{
				if (!time)
					time = getSystemTime();
				tm date = { 0 };
				LocalTime(time / 1000, date);
				return date.tm_mday;
			}

			//获取时区偏移的分钟数（非线程安全）
			inline int getTimeZoneOffset()
			{
				auto localts = time(nullptr);
				tm gmt;
				GMTime(localts, gmt);
				auto gmts = mktime(&gmt);
				return (int)difftime(localts, gmts) / 60;
			}
		}
	}
}
