#include <assert.h>
#include <iostream>

extern bool testSignal();
extern bool testEvent();

int main()
{
	assert(testSignal());
	assert(testEvent());

	std::cout << "all tests passed!" << std::endl;

#ifdef _WIN32 && _DEBUG
	system("pause");
#endif
	return 0;
}

