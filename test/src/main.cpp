#include <assert.h>
#include <iostream>
#include <spdlog/spdlog.h>

extern bool testSignal();
extern bool testEvent();
extern bool testByteArray();
extern bool testMath();
extern bool testTimeTool();
extern bool testAStar();
extern bool testEnum();
extern bool testTypeCheck();
extern bool testDatabase();
extern bool testString();
extern bool testRingBuffer();
extern bool testCallstack();
extern bool testSonyflake();
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
extern bool testPidfile();
#endif
extern bool testTimer();

int main()
{
#if _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif
	spdlog::set_level(spdlog::level::debug);
	spdlog::set_pattern("[%Y/%m/%d %H:%M:%S.%e] %^[%l] %v%$");

	if (//testSignal() &&
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
		testPidfile() &&
#endif
		//testEvent() &&
		//testByteArray() &&
		//testMath() &&
		//testTimeTool() &&
		//testString() &&
		//testRingBuffer() &&
		//testAStar() &&
		//testEnum() &&
		//testTypeCheck() &&
		testDatabase()
		//testTimer() &&
		//testCallstack() &&
		//testSonyflake()
		)
	{
		std::cout << "all tests passed!" << std::endl;
	}

#if _WIN32
	system("pause");
#endif
	return 0;
}
