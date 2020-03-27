#include "ws/core/Log.h"
#include "ws/core/String.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace ws
{
	namespace core
	{
		LogLevel Log::level = LogLevel::_ERROR_;

		void Log::v(const char* format, ...)
		{
			if (level <= LogLevel::_VERBOSE_)
			{
				va_list args;
				va_start(args, format);
				printOut("[V]", format, args);
				va_end(args);
			}
		}

		void Log::d(const char* format, ...)
		{
			if (level <= LogLevel::_DEBUG_)
			{
				va_list args;
				va_start(args, format);
				printOut("[D]", format, args);
				va_end(args);
			}
		}

		void Log::i(const char* format, ...)
		{
			if (level <= LogLevel::_INFO_)
			{
				va_list args;
				va_start(args, format);
				printOut("[I]", format, args);
				va_end(args);
			}
		}

		void Log::w(const char* format, ...)
		{
			if (level <= LogLevel::_WARN_)
			{
				va_list args;
				va_start(args, format);
				printOut("[W]", format, args);
				va_end(args);
			}
		}

		void Log::e(const char* format, ...)
		{
			if (level <= LogLevel::_ERROR_)
			{
				va_list args;
				va_start(args, format);
				printOut("[E]", format, args);
				va_end(args);
			}
		}

		void Log::printOut(const char* level, const char* format, va_list valist)
		{
			std::string timeStr(String::formatTime());
			char buffer[1024] = {0};
#ifdef __APPLE__
			sprintf(buffer, "\r[%s]%s %s\n", timeStr.c_str(), level, format);
#else
			sprintf(buffer, "\r[%s]%s %s\n", timeStr.c_str(), level, format);
#endif
			vprintf(buffer, valist);
			fflush(stdout);
		}
	}
}
