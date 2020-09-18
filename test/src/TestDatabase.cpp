#include <functional>
#include <iostream>
#include "ws/database/Database.h"

using namespace ws::database;

enum class ItemType : uint8_t
{
	NORMAL = 1,
	EQUIP
};

struct ItemConfig
{
	ItemConfig() :id(0), type(ItemType::NORMAL) {}
	uint16_t		id;
	ItemType		type;
	std::string		name;
};

class TestQuery : public DBRequest
{
public:
	using Callback = std::function<void(const ItemConfig&)>;

	void testSomeOperate(Callback cb)
	{
		callback = cb;
	}

private:
	void onRequest(Database& db) override
	{
		if (db.query("SELECT id, name, type FROM item_config LIMIT 1;SELECT name FROM student LIMIT 1"))
		{
			auto record = db.getLastRecord();
			if (record && record->nextRow())
			{
				*record >> item.id >> item.name >> item.type;
			}

			if (db.nextRecordset())
			{
				record = db.getLastRecord();
				if (record && record->nextRow())
				{
					*record >> item.name;
				}
			}
		}

		if (db.query("CALL test_proc()"))
		{
			auto record = db.getLastRecord();
			if (record && record->nextRow())
			{
				uint64_t timestamp = 0;
				*record >> timestamp;
				std::cout << "call test_proc() : " << timestamp << std::endl;
			}
		}

		if (db.query("SELECT name, type FROM item_config LIMIT 1"))
		{
			auto record = db.getLastRecord();
			if (record && record->nextRow())
			{
				*record >> item.name;
			}
		}

		auto stmt = db.prepare("SELECT id, name, type FROM item_config WHERE id=? AND type=?");
		item.id = 1;
		*stmt << item.id << item.type;
		stmt->execute();
		if (stmt->nextRow())
		{
			*stmt >> item.id >> item.name >> item.type;
		}
	}

	void onFinish() override
	{
		callback(item);
	}

private:
	ItemConfig item;
	Callback callback;
};

bool testDatabase()
{
	std::cout << "====================Test Database====================" << std::endl;
	MySQLConfig dbConfig;
	dbConfig.host = "127.0.0.1";
	dbConfig.port = 3306;
	dbConfig.user = "root";
	dbConfig.password = "";
	dbConfig.database = "testdb";
#if defined(__linux__) || defined(__APPLE__)
	dbConfig.unixSock = "/tmp/mysql.sock";
#endif

	DBQueue queue(dbConfig);
	queue.setThread(1);

	bool isComplete = false;
	auto query = std::make_shared<TestQuery>();
	query->testSomeOperate([&isComplete](const ItemConfig& item){
		std::cout << "TestQuery id=" << item.id << ", type=" << (int)item.type << ", name=" << item.name << std::endl;
		isComplete = true;
	});
	queue.addRequest(query);

	while (!isComplete)
	{
		queue.update();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return true;
}
