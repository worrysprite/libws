#include <iostream>
#include "ws/core/Signal.h"

using namespace ws::core;

bool testSignal()
{
	Signal<> sig1;
	std::function<void()> cb1 = []()
	{
		std::cout << "no params callback" << std::endl;
	};

	sig1.add(&cb1);
	sig1.notify();

	Signal<int, float> sig2;
	std::function<void(const int& a, const float& b)> cb2 = [](int a, float b)
	{
		std::cout << "a=" << a << ", b=" << b << std::endl;
	};

	sig2.add(&cb2);
	sig2.notify(1, 0.5f);
	return true;
}

bool testEvent()
{
	return true;
}

