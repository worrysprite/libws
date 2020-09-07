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
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
extern bool testPidfile();
#endif
extern bool testTimer();

int main()
{
#if _WIN32 && _DEBUG
	system("chcp 65001");
#endif
	spdlog::set_level(spdlog::level::debug);
	spdlog::set_pattern("[%Y/%m/%d %H:%M:%S.%e] %^[%l] %v%$");

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	assert(testPidfile());
#endif
	assert(testSignal());
	assert(testEvent());
	assert(testByteArray());
	assert(testMath());
	assert(testTimeTool());
	assert(testString());
	//assert(testAStar());
	assert(testEnum());
	assert(testTypeCheck());
	//assert(testDatabase());
	assert(testTimer());

	std::cout << "all tests passed!" << std::endl;

#if _WIN32 && _DEBUG
	system("pause");
#endif
	return 0;
}

