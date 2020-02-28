#ifndef __WS_UTILS_PROFILER_H__
#define __WS_UTILS_PROFILER_H__

#include <stdlib.h>

namespace ws
{
	namespace core
	{
		class Profiler
		{
		public:
			//获取内存峰值（字节）
			static size_t getPeakRSS();

			//获取当前占用物理内存（字节）
			static size_t getCurrentRSS();

		private:
		};
	}
}

#endif