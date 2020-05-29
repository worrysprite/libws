#include <assert.h>
#include <iostream>

extern bool testSignal();
extern bool testEvent();
extern bool testByteArray();
extern bool testMath();
extern bool testAStar();
extern bool testEnum();
extern bool testTypeCheck();
extern bool testDatabase();
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
extern bool testPidfile();
#endif

int main()
{
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	assert(testPidfile());
#endif
	assert(testSignal());
	assert(testEvent());
	assert(testByteArray());
	assert(testMath());
	assert(testAStar());
	assert(testEnum());
	assert(testTypeCheck());
	assert(testDatabase());

	std::cout << "all tests passed!" << std::endl;

#if _WIN32 && _DEBUG
	system("pause");
#endif
	return 0;
}

