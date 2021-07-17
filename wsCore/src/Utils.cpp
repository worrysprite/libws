#include <spdlog/spdlog.h>
#include "ws/core/Utils.h"

#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp")
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#endif

namespace ws::core
{
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	bool createPidfile(const char* pidfile)
	{
		if (!pidfile) {
			return false;
		}
		int fd = open(pidfile, O_RDWR | O_CREAT, 0664);
		if (fd < 0)
			return false;

		flock lock;
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_pid = getpid();
		if (fcntl(fd, F_SETLK, &lock))
		{
			close(fd);
			return false;
		}
		char buf[32] = { 0 };
		sprintf(buf, "%d\n", lock.l_pid);
		auto pidlength = (ssize_t)strlen(buf);
		if (write(fd, buf, pidlength) != pidlength)
		{
			close(fd);
			return false;
		}
		//关闭fd会导致写锁失效
		return true;
	}
#endif

	uint32_t getNumDigitsOfDemical(uint32_t value)
	{
		uint32_t result = 1;
		if (value >= 100000000)
		{
			value /= 100000000;
			result += 8;
		}
		if (value >= 10000)
		{
			value /= 10000;
			result += 4;
		}
		if (value >= 100)
		{
			value /= 100;
			result += 2;
		}
		if (value >= 10)
		{
			value /= 10;
			++result;
		}
		return result;
	}

	std::vector<std::string> callstack(int skip)
	{
		constexpr int MAX_STACK = 128;
		void* stack[MAX_STACK] = { 0 };

#ifdef _WIN32
		//创建一块buffer用于保存符号信息
		char buffer[sizeof(SYMBOL_INFO) + sizeof(char) * MAX_SYM_NAME];
		SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;

		//获取堆栈帧
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		int numFrames = CaptureStackBackTrace(skip, MAX_STACK, stack, NULL);

		DWORD displacement = 0;
		IMAGEHLP_LINE64 line;

		std::vector<std::string> result(numFrames + 1);
		result[0] = fmt::format("Thread {:#x}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
		std::string format{ "[{:<" };
		format += std::to_string(getNumDigitsOfDemical(numFrames));
		format += "}] {:#016x} in {} at {}:{}";
		for (int i = 0; i < numFrames; ++i)
		{
			SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
			SymGetLineFromAddr64(process, (DWORD64)(stack[i]), &displacement, &line);

			result[i + 1] = fmt::format(format, i, symbol->Address, symbol->Name, line.FileName, line.LineNumber);
		}
		return result;
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
		// must compile with -rdynamic
		// 获取栈中各层调用函数地址
		int numFrames = backtrace(stack, MAX_STACK);
		std::vector<std::string> result;
		result.reserve(numFrames);

		//获取堆栈符号信息
		Dl_info info;
		result.push_back(fmt::format("Thread {:#x}", std::hash<std::thread::id>{}(std::this_thread::get_id())));
		std::string format{ "[{:<" };
		format += std::to_string(getNumDigitsOfDemical(numFrames));
		format += "}] {:#016x} in {} at {}";
		for (int i = skip; i < numFrames; ++i)
		{
			if (dladdr(stack[i], &info))
			{
				if (auto demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, nullptr))
				{
					result.push_back(fmt::format(format, i - skip, (uint64_t)stack[i], demangled, info.dli_fname));
					free(demangled);
					continue;
				}
			}

			if (!info.dli_sname)
				info.dli_sname = "??";
			if (!info.dli_fname)
				info.dli_fname = "??";
			result.push_back(fmt::format(format, i - skip, (uint64_t)stack[i], info.dli_sname, info.dli_fname));
		}
		return result;
#endif
	}
}
