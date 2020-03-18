#include <iostream>
#include "ws/core/Signal.h"
#include "ws/core/Event.h"
#include "ws/core/ByteArray.h"
#include "ws/core/Utils.h"
#include "ws/core/Math.h"
#include "ws/core/AStar.h"

using namespace ws::core;

bool testSignal()
{
	std::cout << "====================Test Signal====================" << std::endl;
	Signal<> sig1;
	std::function<void()> cb1 = []()
	{
		std::cout << "no params callback" << std::endl;
	};

	sig1.add(&cb1);
	sig1.notify();

	Signal<const int&, const float&> sig2;
	std::function<void(const int& a, const float& b)> cb2 = [](int a, float b)
	{
		std::cout << "a=" << a << ", b=" << b << std::endl;
	};

	sig2.add(&cb2);
	sig2.notify(1, 0.5f);
	std::cout << std::endl;
	return true;
}

bool testEvent()
{
	std::cout << "====================Test Event====================" << std::endl;
	EventDispatcher dispatcher;
	std::function<void(const Event&)> cb = [](const Event& evt)
	{
		std::cout << "Trigger event, type=" << evt.type << std::endl;
	};

	dispatcher.addEventListener(1, &cb);
	Event evt(1);
	dispatcher.dispatchEvent(evt);

	dispatcher.removeEventListener(1, &cb);
	dispatcher.dispatchEvent(evt);
	std::cout << std::endl;
	return true;
}

#pragma pack(push, 1)
struct TestStruct
{
	TestStruct() { ZERO_INIT(i8, u64); }

	int8_t				i8;
	uint8_t				u8;
	int16_t				i16;
	uint16_t			u16;
	int32_t				i32;
	uint32_t			u32;
	int64_t				i64;
	uint64_t			u64;
};
#pragma pack(pop)

bool testByteArray()
{
	std::cout << "====================Test ByteArray====================" << std::endl;
	TestStruct ts1, ts2;
	ts1.i8 = -0x70;
	ts1.u8 = 0xf0;
	ts1.i16 = -0x7ff0;
	ts1.u16 = 0xfff0;
	ts1.i32 = -0x7ffffff0;
	ts1.u32 = 0xfffffff0;
	ts1.i64 = -0x7ffffffffffffff0ll;
	ts1.u64 = 0xfffffffffffffff0ull;

	ByteArray bytes;
	bytes << ts1.i8 << ts1.u8 << ts1.i16 << ts1.u16 << ts1.i32 << ts1.u32 << ts1.i64 << ts1.u64;
	std::cout << "sizeof bytes: " << bytes.size() << std::endl;
	std::cout << std::endl;
	return true;
}

bool testMath()
{
	std::cout << "====================Test Math====================" << std::endl;
	auto num1 = Math::random();
	std::cout << "random of [0, 1) =" << num1 << std::endl;

	auto num2 = Math::random(1, 100);
	std::cout << "random of [1, 100] =" << num2 << std::endl;

	std::cout << "half PI =" << Math::HALF_PI << std::endl;

	Vector2D<double> v1(3, 4);
	std::cout << "length of Vector2D(3, 4) =" << v1.getLength() << std::endl;

	Vector2D<double> v2(-4, 3);
	std::cout << "angle between Vector2D(3, 4) and Vector2D(-4, 3) =" << Vector2D<double>::angleBetween(v1, v2) << std::endl;


	

	std::cout << std::endl;
	return true;
}

bool testAStar()
{
	constexpr int WIDTH = 75;
	constexpr int HEIGHT = 100;

	bool blockData[HEIGHT][WIDTH] = {
#include "testmap"
	};

	class GameMap : public AbstractMap
	{
	public:
		GameMap(bool* blocks, int w, int h) : map(blocks), width(w), height(h) {}

		//地图宽
		virtual int getWidth() const override { return width; }
		//地图高
		virtual int getHeight() const override { return height; }
		//点x,y是否阻挡
		virtual bool isBlock(int x, int y) const override
		{
			if (x < 0 || x >= width || y < 0 || y >= height)
				return false;	//out of range

			return map[y * width + x];
		}

	private:
		int width;
		int height;
		bool* map;
	};

	GameMap map((bool*)blockData, WIDTH, HEIGHT);

	char graph[HEIGHT][WIDTH + 5] = { 0 };
	for (int y = 0; y < HEIGHT; ++y)
	{
		sprintf(graph[y], "%03d ", y);
		for (int x = 0; x < WIDTH; ++x)
		{
			if (map.isBlock(x, y))
			{
				graph[y][x + 4] = '#';
			}
			else
			{
				graph[y][x + 4] = ' ';
			}
		}
		//std::cout << graph[y] << std::endl;
	}

	graph[71][28 + 4] = 'S';
	graph[64][51 + 4] = 'E';

	AStar astar;
	if (astar.findPath(map, 28, 71, 51, 64))
	{
		auto& path = astar.getLastPath();
		std::cout << "find path success, num path nodes: " << path.size() << std::endl;

		for (auto node : path)
		{
			graph[node->y][node->x + 4] = '*';
		}
		for (int y = 0; y < HEIGHT; ++y)
		{
			std::cout << graph[y] << std::endl;
		}
	}
	else
	{
		std::cout << "find path failed!" << std::endl;
		return false;
	}

	if (!astar.findPath(map, 28, 71, 30, 52))	//can not move from start to end
	{
		auto& path = astar.getLastPath();
		std::cout << "path not found, num path nodes: " << path.size() << std::endl;
		return true;
	}
	return false;
}
