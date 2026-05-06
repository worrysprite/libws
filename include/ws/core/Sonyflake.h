#pragma once
#include <chrono>
#include <cstdint>
#include <thread>
#include "TimeTool.h"

namespace ws::core
{
	class Sonyflake
	{
	public:
		// machine_id: 0 - 65535
		Sonyflake(uint16_t machineId = 0) : _machineId(machineId), lastTick(0), sequence(0)
#if _DEBUG
			, sleepCount(0), yieldCount(0)
#endif
		{
			// 1. 设置自定义 Epoch (例如 2024-01-01 00:00:00 UTC)
			constexpr uint64_t startEpoch = 1704067200000;

			// 2. 锚定系统时钟和单调时钟
			auto now = TimeTool::getUnixtime();
			startSystime = now - startEpoch;
			startSteadyTime = std::chrono::steady_clock::now();
		}

		// 注意不是线程安全，多线程使用请自行保证线程安全
		uint64_t nextId()
		{
			uint64_t currentTick = getCurrentTime() / 10;
			// Sonyflake 建议的时间单位是 10ms
			if (currentTick == lastTick)
			{
				if (++sequence == 0)
				{
					// 序列号溢出，当前 10ms 额度用完，强制进入下一个 10ms
					currentTick = waitNextTick((lastTick + 1) * 10) / 10;
				}
			}
			else
			{
				// 时间前进，重置序列号
				sequence = 0;
			}
			lastTick = currentTick;
			// 组装 ID (1bit 0 + 39bit time + 16bit machine + 8bit seq)
			return (lastTick << 24) | (uint64_t(_machineId) << 8) | uint64_t(sequence);
		}

		// get or set machineId
		uint16_t machineId() const { return _machineId; }
		void machineId(uint16_t machineId) { _machineId = machineId; }

#if _DEBUG
		uint32_t getSleepCount() const
		{
			return sleepCount;
		}

		uint32_t getYieldCount() const
		{
			return yieldCount;
		}
#endif

	private:
		// 获取相对自定义Epoch的稳定时间戳
		uint64_t getCurrentTime() const
		{
			// 基于单调时钟计算当前时间戳，不受时钟回拨影响
			auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - startSteadyTime
			).count();
			return startSystime + diff;
		}

		// 等待直到下一个 10ms
		uint64_t waitNextTick(uint64_t next)
		{
			uint64_t now = getCurrentTime();
			int64_t diff = next - now;
			while (diff > 0)
			{
				if (diff > 2)
				{
#if _DEBUG
					++sleepCount;
#endif
					std::this_thread::sleep_for(std::chrono::microseconds(1));
				}
				else
				{
#if _DEBUG
					++yieldCount;
#endif
					std::this_thread::yield();
				}
				now = getCurrentTime();
				diff = next - now;
			}
			return now;
		}

	private:
		uint64_t lastTick;     //10ms单位的时间戳
		uint64_t startSystime; //相对自定义Epoch起始系统时间戳
#if _DEBUG
		uint32_t sleepCount;
		uint32_t yieldCount;
#endif
		uint16_t _machineId;	//机器id
		uint8_t  sequence;	//序号

		// 时间锚点
		std::chrono::steady_clock::time_point startSteadyTime;
	};
}
