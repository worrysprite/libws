#include <iostream>
#include "ws/core/Signal.h"
#include "ws/core/Event.h"
#include "ws/core/ByteArray.h"
#include "ws/core/Utils.h"
#include "ws/core/Math.h"
#include "ws/core/Log.h"
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

	// test with function
	std::function<void(const Event&)> cb = [](const Event& evt)
	{
		std::cout << "Trigger event, type=" << evt.type << std::endl;
	};

	dispatcher.addEventListener(1, &cb);
	Event evt(1);
	dispatcher.dispatchEvent(evt);
	dispatcher.removeEventListener(1, &cb);

	// test with interface class
	struct Listener : public EventListener
	{
		virtual void onEvent(const Event& evt) override
		{
			std::cout << "Trigger event, type=" << evt.type << std::endl;
		}
	} listener;
	EventCallback cb2(listener);
	dispatcher.addEventListener(2, &cb2);

	evt.type = 2;
	dispatcher.dispatchEvent(evt);
	dispatcher.removeEventListener(2, &cb2);
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
	ts1.i8 = -110;
	ts1.u8 = 155;
	ts1.i16 = -31234;
	ts1.u16 = 41234;
	ts1.i32 = -2123456789;
	ts1.u32 = 3123456789;
	ts1.i64 = -0x7ffffffffffffff0ll;
	ts1.u64 = 0xfffffffffffffff0ull;

	ByteArray bytes;
	bytes << ts1.i8 << ts1.u8 << ts1.i16 << ts1.u16 << ts1.i32 << ts1.u32 << ts1.i64 << ts1.u64;
	std::cout << "sizeof bytes: " << bytes.size() << std::endl;

	bytes >> ts2;
	std::cout << "unpack data: " << std::endl <<
		(int)ts2.i8 << std::endl <<
		(int)ts2.u8 << std::endl <<
		ts2.i16 << std::endl <<
		ts2.u16 << std::endl <<
		ts2.i32 << std::endl << 
		ts2.u32 << std::endl << 
		ts2.i64 << std::endl << 
		ts2.u64 << std::endl;

	ByteArray b2;
	b2.swap(bytes);

	std::cout << "sizeof bytes: " << bytes.size() << "sizeof byte2: " << b2.size() << std::endl;
	b2.readPosition(0);
	b2 >> ts2;
	std::cout << "unpack data: " << std::endl <<
		(int)ts2.i8 << std::endl <<
		(int)ts2.u8 << std::endl <<
		ts2.i16 << std::endl <<
		ts2.u16 << std::endl <<
		ts2.i32 << std::endl <<
		ts2.u32 << std::endl <<
		ts2.i64 << std::endl <<
		ts2.u64 << std::endl;
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

bool testLog()
{
	std::cout << "====================Test Log====================" << std::endl;
	Log::level = LogLevel::_DEBUG_;

	Log::v("verbost log...");
	Log::d("debug log with fmt string: %s", "variable");
	Log::e("error1 log..");

	Log::level = LogLevel::_WARN_;

	Log::d("do not print debug log");
	Log::e("error2 log..");
	return true;
}

bool testAStar()
{
	constexpr uint32_t WIDTH = 75;
	constexpr uint32_t HEIGHT = 100;

	bool blockData[HEIGHT][WIDTH] = {
#include "testmap"
	};

	class GameMap : public AbstractMap
	{
	public:
		GameMap(bool* blocks, uint32_t w, uint32_t h) : map(blocks), width(w), height(h) {}

		//地图宽
		virtual uint32_t getWidth() const override { return width; }
		//地图高
		virtual uint32_t getHeight() const override { return height; }
		//点x,y是否阻挡
		virtual bool isBlock(uint32_t x, uint32_t y) const override
		{
			if (x >= width || y >= height)
				return true;	//out of range

			return map[y * width + x];
		}

	private:
		bool* map;
		uint32_t width;
		uint32_t height;
	};

	GameMap map((bool*)blockData, WIDTH, HEIGHT);

	char graph[HEIGHT][WIDTH + 5] = { 0 };
	for (uint32_t y = 0; y < HEIGHT; ++y)
	{
		sprintf(graph[y], "%03d ", y);
		for (uint32_t x = 0; x < WIDTH; ++x)
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
		for (uint32_t y = 0; y < HEIGHT; ++y)
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

enum class Permissions : uint8_t
{
	READ = 0x4,
	WRITE = 0x2,
	EXECUTE = 0x1,

	ENABLE_BITMASK = 1
};
ENABLE_BITMASK_OPERATORS(Permissions);

enum class FileOperation : uint8_t
{
	MIN,
	READ = MIN,
	WRITE,
	MAX
};

bool testEnum()
{
	Permissions p = Permissions::READ;
	p = p | Permissions::WRITE;
	p |= Permissions::EXECUTE;
	std::cout << "enum Permissions bitmask operation result: " << (int)p << std::endl;

	std::cout << "enum FileOperation validate 1 result: " << validateEnum<FileOperation>(1) << std::endl;
	std::cout << "enum FileOperation validate 5 result: " << validateEnum<FileOperation>(5) << std::endl;
	return true;
}