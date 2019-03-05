#include "ws/core/Math.h"
#include <chrono>

namespace ws
{
	namespace core
	{
		std::mt19937 Math::randomGenerator((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());

	}
}

