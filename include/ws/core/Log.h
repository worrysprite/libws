#ifndef __WS_CORE_LOG_H__
#define __WS_CORE_LOG_H__

#include <stdarg.h>

namespace ws
{
	namespace core
	{
		enum class LogLevel
		{
			_VERBOSE_,
			_DEBUG_,
			_INFO_,
			_WARN_,
			_ERROR_
		};

		/************************************************************************/
		/* 打印LOG信息，内容最多1KB                                              */
		/************************************************************************/
		class Log
		{
		public:
			static LogLevel level;

			static void v(const char* format, ...);
			static void d(const char* format, ...);
			static void i(const char* format, ...);
			static void w(const char* format, ...);
			static void e(const char* format, ...);

		private:
			static void printOut(const char* format, const char* level, va_list valist);
		};
	}
}
#endif