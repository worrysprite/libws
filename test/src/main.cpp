#include <assert.h>
#include <iostream>

extern bool testSignal();
extern bool testEvent();
extern bool testByteArray();

int main()
{
	assert(testSignal());
	assert(testEvent());
	assert(testByteArray());

	std::cout << "all tests passed!" << std::endl;

#ifdef _WIN32 && _DEBUG
	system("pause");
#endif
	return 0;
}

