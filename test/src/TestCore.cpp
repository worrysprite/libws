#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
#include "ws/core/Signal.h"
#include "ws/core/Event.h"
#include "ws/core/ByteArray.h"
#include "ws/core/Utils.h"
#include "ws/core/Math.h"
#include "ws/core/AStar.h"
#include "ws/core/TimeTool.h"
#include "ws/core/Timer.h"
#include "ws/core/String.h"
#include "ws/core/RingBuffer.h"

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
		//Listener must be copyable
		//Listener(const Listener&) = delete;

		//and member will be copied when convert to EventCallback
		int member = 100;

		virtual void onEvent(const Event& evt) override
		{
			std::cout << "Trigger event, type=" << evt.type <<
				", member=" << this->member << std::endl;
		}
	} listener;
	EventCallback cb2(listener);
	listener.member = 300;	//will not affect cb2
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
	std::cout << "hex: " << bytes.toHexString() << std::endl;

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

	ByteArray b2, b3;
	b2.swap(bytes);

	std::cout << "sizeof bytes after swap: " << bytes.size() << std::endl << std::endl;
	std::cout << "sizeof byte2: " << b2.size() << std::endl;
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

	b2.seek(-8);
	std::cout << "last 8 bytes data:" << b2.readUInt64() << std::endl;
	std::cout << std::endl;

	b3 = std::move(b2);	//move assign
	if (b2.data() != nullptr)
	{
		return false;
	}
	std::cout << b3.toHexString(true) << std::endl;

	if (!b3.fromHexString("86bd5c3fc5060000dd05013300000000330000000a0000009600010000001f000000010000000c00e6ada3e59ca8e8be93e585a501000096000f000000020000000200000078000000060073666764686a01000096000f00000003000000040000007800000004007273737401000096000f000200040000000500000078000000030073676802000096000f000000050000000600000078000000050072756a333503000096000f0000000600000007000000780000000600646768666a6b04000096000f00000007000000080000007800000004006166646801000096000f000000080000000900000078000000060032666466736601000096000f000000090000000a000000780000000900e998bfe890a8e5beb701000096000f0005000a00000001000000780000000600616467683334011500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c8596000f0000000b0000000b000000780000000500726566666401000096000f0000000c0000002a000000010000000600e99990e8b4ad0100006e00000000000d000000050000000100000002006464061500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c856a000b000a000e0000000b00000001000000050048616c6579031500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c856600060006000f00000003000000010000000300e59194050f00e99d93e4bb94e69da5e8bf99e9878c6000050006001000000002000000010000000900e79da1e5a4a7e8a197061500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c855800050005001100000006000000010000000d00e6889169e5a682e5a5bde4b985061500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c8557000500030012000000290000000100000004003231313106000056000b00040013000000090000000100000003007830310300005600040002001400000010000000010000000200743101000055000f00000015000000110000000100000004007465737401000055000f000300160000001c000000010000000600e79a84e6929206000055000300000017000000080000000100000002006332021500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c855500050004001800000013000000010000000600646a6669756501000055000f0006001900000018000000010000000900e5b08fe69cace69cac0600005500010004001a0000002400000001000000050046313231330200005500010000001b0000000d000000010000000900e5a4a7e694afe9878e010f00e99d93e4bb94e69da5e8bf99e9878c5300020000001c0000000a0000000100000005007978312d31050f00e99d93e4bb94e69da5e8bf99e9878c5300000004001d000000220000000100000005007978322d310100005200000000001e000000140000000100000003007a78720500005000000000001f0000001600000001000000050066617368690600005000000000002000000001000000010000000300594859061500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c854f0000000200210000000c0000000100000002003872041500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c854f0000000100220000001e00000001000000040031303031050300313131490000000000230000002b000000010000000900783031e588bae5aea2040000480000000400240000001d00000001000000040071776571031500e5a48fe99ba8e8bf98e6b2a1e58f91e7baa2e58c85480000000000250000002500000001000000050033323133310103003131314600000000002600000023000000010000000c00e6ada3e59ca8e692a4e59b9e0600004600000000002700000007000000010000000f00e5a48fe99ba8e58f91e7baa2e58c85020000460000000000280000000e000000010000000300616f730100003f0000000000290000000f0000000100000006004449444944490100003e00000000002a000000120000000100000003005048500100003d00000000002b0000001b000000010000000400333231330400003d00000003002c000000260000000100000005007978312d320400003d00000002002d000000040000000100000006003435343532340100003400000000002e00000021000000010000000c00e585ade68c87e790b4e9ad940300003200000000002f00000027000000010000000900e5988ee5988ee5988e020000320000000000300000002c000000010000000900e998bfe8a5bfe590a70200002e000000000031000000190000000100000006003635363534360200002d0000000200320000001500000001000000030051415a0100002800000000003300000017000000010000000c00e681ade5969ce58f91e8b4a2010000240000000000"))
	{
		return false;
	}

	std::cout << b3.size() << ": " << b3.toHexString(true) << std::endl;
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

bool testTimeTool()
{
	std::cout << "timezone offset: " << TimeTool::getTimeZoneOffset() << std::endl;
	std::cout << "day of week: " << TimeTool::getDayOfWeek() << std::endl;
	std::cout << "day of month: " << TimeTool::getDayOfMonth() << std::endl;
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
		virtual bool isBlock(uint32_t x, uint32_t y, void* userdata) const override
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
			if (map.isBlock(x, y, nullptr))
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
		for (auto node : path)
		{
			graph[node->y][node->x + 4] = '*';
		}
		for (auto& y : graph)
		{
			std::cout << y << std::endl;
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

bool testTypeCheck()
{
	std::shared_ptr<int> a;
	std::unique_ptr<std::string> b;

	std::cout << "check is_smart_ptr: " << std::endl;
	std::cout << std::boolalpha << is_smart_ptr<int>::value << std::endl;
	std::cout << std::boolalpha << is_smart_ptr<const int*>::value << std::endl;
	std::cout << std::boolalpha << is_smart_ptr<std::shared_ptr<int>>::value << std::endl;
	std::cout << std::boolalpha << is_smart_ptr<decltype(a)>::value << std::endl;

	std::cout << std::boolalpha << is_smart_ptr<std::string>::value << std::endl;
	std::cout << std::boolalpha << is_smart_ptr<std::unique_ptr<std::string>>::value << std::endl;
	std::cout << std::boolalpha << is_smart_ptr<decltype(b)>::value << std::endl;

	std::cout << std::endl;
	return true;
}

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
bool testPidfile()
{
	static const char PID_FILE[] = "/var/run/test.pid";
	if (createPidfile(PID_FILE))
	{
		std::cout << "lock pidfile success!" << std::endl;
		return true;
	}
	return false;
}
#endif

bool testTimer()
{
	Timer timer;
	bool isFinish = false;

	timer.addTimeCall(2000ms, [&isFinish]() {
		spdlog::debug("2000ms callback");
		static int count = 0;
		if (++count == 10)
		{
			isFinish = true;
		}
	});
	timer.delayCall(1ms, []() {
		spdlog::debug("1ms callback");
	});
	timer.delayCall(255ms, []() {
		spdlog::debug("255ms callback");
	});
	timer.delayCall(256ms, []() {
		spdlog::debug("256ms callback");
	});
	timer.delayCall(511ms, []() {
		spdlog::debug("511ms callback");
	});
	timer.delayCall(512ms, []() {
		spdlog::debug("512ms callback");
	});
	timer.delayCall(16383ms, []() {
		spdlog::debug("16383ms callback");
	});
	timer.delayCall(16384ms, []() {
		spdlog::debug("16384ms callback");
	});
	while (!isFinish)
	{
		timer.update();
		std::this_thread::sleep_for(1ms);
	}
	return true;
}

bool testString()
{
	const std::string teststr("The quick brown fox jump sover the lazy dog.");
	char input[1024] = { 0 };
	teststr.copy(input, teststr.size());

	std::vector<std::string> result1;
	String::split(input, " ", result1);
	if (result1.size() != 9)
		return false;

	std::vector<char*> result2;
	String::split(input, " ", result2);
	if (result2.size() != 9)
		return false;

	if (teststr != String::join(result1, " "))
		return false;

	std::list<std::string> strlist{ "5", "4", "3", "2", "1" };
	if (String::join(strlist, ",") != "5,4,3,2,1")
		return false;

	std::vector<int> nums = { 1, 2, 3, 4, 5, 6 };
	if (std::string("1,2,3,4,5,6") != String::join(nums, ","))
		return false;

	auto now = time(nullptr);
	auto timeStr = String::formatTime(now, "%F %T");
	if (now != String::formatTime(timeStr))
		return false;

	if (1546444800 != String::formatTime("2019/01/03 00:00"))
		return false;
	
	return 12005313033 == String::formatTime("2350/6/8 17:10:33");
}

bool testRingBuffer()
{
	RingBuffer ring;
	auto cap = ring.available();
	while (ring.used() < cap)	//一直填充到发生扩容
	{
		ring << fmt::format("测试：{}", Math::random(1, 999999));
	}
	//输出填充的内容
	while (ring.used())
	{
		spdlog::debug(ring.readString());
	}

	//先输入部分数据
	ring.truncate(true);
	cap = cap * 0.7;
	int num = 0;
	while (ring.used() < cap)
	{
		ring << fmt::format("测试测试测试测试测试测试测试测试测试测试：{}", ++num);
	}

	++num;
	//边输入边输出
	for (int i = 0; i < 10000; ++i)
	{
		ring << fmt::format("测试测试测试测试测试测试测试测试测试测试：{}", num + i);
		spdlog::debug(ring.readString());
	}
	return true;
}

struct cstest
{
	int hello(int val)
	{
		auto stacks = callstack();
		for (auto& str : stacks)
		{
			spdlog::debug(str);
		}
		return val;
	}
};

bool testCallstack()
{
	return [](int val)
	{
		cstest t;
		return t.hello(val);
	}(666) == 666;
}
