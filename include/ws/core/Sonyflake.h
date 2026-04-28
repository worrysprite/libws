#pragma once
#include <chrono>
#include <cstdint>
#include <thread>

namespace ws::core
{
	class Sonyflake
	{
	public:
		// machine_id: 0 - 65535
		Sonyflake(uint16_t machineId) : machineId(machineId), lastTime(0), sequence(0)
		{
			// 1. 设置自定义 Epoch (例如 2024-01-01 00:00:00 UTC)
			// Sonyflake 建议的时间单位是 10ms
			constexpr uint64_t startEpoch = 1704067200000;

			// 2. 锚定系统时钟和单调时钟
			auto sys_now = std::chrono::system_clock::now();
			startSteadyTime = std::chrono::steady_clock::now();

			auto duration =
				std::chrono::duration_cast<std::chrono::milliseconds>(sys_now.time_since_epoch());
			startSystime = (duration.count() - startEpoch) / 10;
		}

		// 注意不是线程安全，多线程使用请自行保证线程安全
		uint64_t nextId()
		{
			uint64_t currentTime = getCurrentElapsedTime();

			// 逻辑处理：如果当前时间等于上次时间
			if (currentTime == lastTime)
			{
				sequence = (sequence + 1) & 0xFF; // 8 bits: 0-255
				if (sequence == 0)
				{
					// 序列号溢出，当前 10ms 额度用完，强制进入下一个 10ms
					currentTime = waitNextTick(lastTime);
				}
			}
			else
			{
				// 时间前进，重置序列号
				sequence = 0;
			}

			lastTime = currentTime;

			// 组装 ID (1bit 0 + 39bit time + 8bit seq + 16bit machine)
			return (lastTime << 24) | (static_cast<uint64_t>(sequence) << 16) | machineId;
		}

	private:
		// 获取基于单调时钟计算的相对时间戳（单位：10ms）
		uint64_t getCurrentElapsedTime() const
		{
			auto now_steady = std::chrono::steady_clock::now();
			auto diff =
				std::chrono::duration_cast<std::chrono::milliseconds>(now_steady - startSteadyTime);
			return startSystime + (diff.count() / 10);
		}

		// 等待直到下一个 10ms
		uint64_t waitNextTick(uint64_t lastTime) const
		{
			uint64_t now = getCurrentElapsedTime();
			while (now <= lastTime)
			{
				std::this_thread::yield(); // 让出 CPU，不挂起线程，保持高响应
				now = getCurrentElapsedTime();
			}
			return now;
		}

	private:
		uint64_t lastTime;     //10ms单位
		uint64_t startSystime; //10ms单位
		uint16_t machineId;
		uint8_t  sequence;

		// 时间锚点
		std::chrono::steady_clock::time_point startSteadyTime;
	};
}
