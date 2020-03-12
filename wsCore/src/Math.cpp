#include "ws/core/Math.h"
#include <chrono>

namespace ws
{
	namespace core
	{
		const double Math::PI = 3.14159265358979323846;
		const double Math::HALF_PI = PI * 0.5;
		const double Math::RAD_PER_ANGLE = 0.01745329251994329577;

		std::mt19937 Math::randomGenerator((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
	}
}

