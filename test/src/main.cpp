#include <assert.h>
#include <cstdlib>
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
extern bool testServerSocket();
extern bool testClientSocket();

static bool envTruthy(const char* name)
{
	const char* v = std::getenv(name);
	return v && v[0] && !(v[0] == '0' && v[1] == '\0');
}

int main()
{
#if _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif
	spdlog::set_level(spdlog::level::debug);
	spdlog::set_pattern("[%Y/%m/%d %H:%M:%S.%e] %^[%l] %v%$");

	bool ok = true;
	ok = testSignal() && ok;
	ok = testEvent() && ok;
	ok = testByteArray() && ok;
	ok = testMath() && ok;
	ok = testTimeTool() && ok;
	ok = testString() && ok;
	ok = testRingBuffer() && ok;
	ok = testEnum() && ok;
	ok = testTypeCheck() && ok;
	ok = testAStar() && ok;
	ok = testSonyflake() && ok;
	ok = testServerSocket() && ok;
	ok = testClientSocket() && ok;
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	ok = testPidfile() && ok;
#endif

	// 耗时较长的 Timer 默认跳过；设置 LIBWS_RUN_TIMER_TEST=1 启用
	if (envTruthy("LIBWS_RUN_TIMER_TEST"))
		ok = testTimer() && ok;
	else
		std::cout << "skip testTimer (set LIBWS_RUN_TIMER_TEST=1 to enable)" << std::endl;

	// DB 需要本机 MySQL；默认跳过，设 LIBWS_RUN_DB_TEST=1 启用（可用 LIBWS_MYSQL_* 覆盖连接）
	if (envTruthy("LIBWS_RUN_DB_TEST"))
		ok = testDatabase() && ok;
	else
		std::cout << "skip testDatabase (set LIBWS_RUN_DB_TEST=1 to enable)" << std::endl;

	if (ok)
	{
		std::cout << "all tests passed!" << std::endl;
		return 0;
	}
	std::cout << "some tests failed!" << std::endl;
	return 1;
}
