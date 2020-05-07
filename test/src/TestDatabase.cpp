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
	typedef std::function<void(const ItemConfig&)> Callback;

	void testSomeOperate(Callback cb)
	{
		callback = cb;
	}

private:
	virtual void onRequest(Database& db)
	{
		auto record = db.query("SELECT id, name, type FROM item_config LIMIT 1");
		if (record && record->nextRow())
		{
			*record >> item.id >> item.name >> item.type;
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

	virtual void onFinish()
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
	MYSQL_CONFIG dbConfig;
	dbConfig.strHost = "127.0.0.1";
	dbConfig.nPort = 3306;
	dbConfig.strUser = "root";
	dbConfig.strPassword = "";
	dbConfig.strDB = "testdb";
#if defined(__linux__) || defined(__APPLE__)
	dbConfig.strUnixSock = "/tmp/mysql.sock";
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
