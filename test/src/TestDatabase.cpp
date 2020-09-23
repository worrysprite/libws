#include <functional>
#include <iostream>
#include <variant>
#include <chrono>
#include "ws/database/Database.h"

using namespace ws::database;
using namespace std::chrono;

enum class ItemType : uint8_t
{
	NORMAL = 1,
	EQUIP
};

struct ItemConfig
{
	uint32_t		id = 0;
	ItemType		type = (ItemType)0;
	std::string		name;
};

class TestQuery : public DBRequest
{
	enum class QueryType
	{
		INIT_TABLE = 1,
		GET_ITEM_CONFIG,
		SAVE_ITEM_CONFIG,
		CALL_PROC,
	};

public:
	void initTables(const std::function<void()>& cb)
	{
		if (cb)
		{
			type = QueryType::INIT_TABLE;
			callback = cb;
		}
	}

	void getItemConfig(uint32_t id, const std::function<void(const ItemConfig&)>& cb)
	{
		if (cb)
		{
			type = QueryType::GET_ITEM_CONFIG;
			item.id = id;
			callback = cb;
		}
	}

	void saveItemConfig(const ItemConfig& data, const std::function<void()>& cb)
	{
		if (cb)
		{
			type = QueryType::SAVE_ITEM_CONFIG;
			item = data;
			callback = cb;
		}
	}

	void callproc(const std::function<void()>& cb)
	{
		if (cb)
		{
			type = QueryType::CALL_PROC;
			callback = cb;
		}
	}

private:
	void onRequest(Database& db) override
	{
		switch (type)
		{
		case TestQuery::QueryType::INIT_TABLE:
		{
			db.query("DROP TABLE IF EXISTS item_config");
			db.query("CREATE TABLE item_config(`id` int unsigned not null, `name` varchar(10) null, "
				"`type` tinyint unsigned null, primary key(id)) default charset=utf8");
			db.query("INSERT INTO item_config VALUES(1, '《奇门遁甲》', 1), (2, '时光流韵', 2)");
			
			db.query("DROP PROCEDURE IF EXISTS test_proc");
			db.query("CREATE PROCEDURE `test_proc`() BEGIN select unix_timestamp(); END");
			break;
		}
		case TestQuery::QueryType::GET_ITEM_CONFIG:
		{
			std::string sql{ "SELECT `name`,`type` FROM item_config WHERE id=" };
			sql += std::to_string(item.id);
			if (db.query(sql))
			{
				auto record = db.getLastRecord();
				if (record && record->nextRow())
				{
					*record >> item.name >> item.type;
				}
			}
			auto stmt = db.prepare("SELECT `name`, `type` FROM item_config WHERE id=?");
			if (stmt)
			{
				*stmt << item.id;
				stmt->execute();
				if (stmt->nextRow())
				{
					*stmt >> item.name >> item.type;
				}
			}
			break;
		}
		case TestQuery::QueryType::SAVE_ITEM_CONFIG:
		{
			auto stmt = db.prepare("UPDATE item_config SET `name`=?, `type`=? WHERE id=?");
			if (stmt)
			{
				*stmt << item.name << item.type << item.id;
				stmt->execute();
			}
			break;
		}
		case TestQuery::QueryType::CALL_PROC:
		{
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
			break;
		}
		default:
			break;
		}
	}

	void onFinish() override
	{
		switch (type)
		{
		case TestQuery::QueryType::INIT_TABLE:
		case TestQuery::QueryType::SAVE_ITEM_CONFIG:
		case TestQuery::QueryType::CALL_PROC:
			std::get<std::function<void()>>(callback)();
			break;
		case TestQuery::QueryType::GET_ITEM_CONFIG:
			std::get<std::function<void(const ItemConfig&)>>(callback)(item);
			break;
		default:
			break;
		}
	}

private:
	QueryType type = (QueryType)0;
	ItemConfig item;
	std::variant<
		std::function<void(const ItemConfig&)>,
		std::function<void()>>	callback;
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
	//init tables
	auto query = std::make_shared<TestQuery>();
	query->initTables([&isComplete]() {
		std::cout << "init talbe complete" << std::endl;
		isComplete = true;
	});
	queue.addRequest(query);
	while (!isComplete)
	{
		queue.update();
		std::this_thread::sleep_for(1ms);
	}

	//get item
	isComplete = false;
	query = std::make_shared<TestQuery>();
	query->getItemConfig(1, [&isComplete](const ItemConfig& item) {
		std::cout << "get item result: id=" << item.id << ", type=" << (int)item.type << ", name=" << item.name << std::endl;
		isComplete = true;
	});
	queue.addRequest(query);
	while (!isComplete)
	{
		queue.update();
		std::this_thread::sleep_for(1ms);
	}

	//save item
	isComplete = false;
	ItemConfig data{1, ItemType::EQUIP, "妖邪必败"};
	query = std::make_shared<TestQuery>();
	query->saveItemConfig(data, [&isComplete]() {
		std::cout << "save item complete" << std::endl;
		isComplete = true;
	});
	queue.addRequest(query);
	while (!isComplete)
	{
		queue.update();
		std::this_thread::sleep_for(1ms);
	}

	//test proc
	isComplete = false;
	query = std::make_shared<TestQuery>();
	query->callproc([&isComplete]() {
		isComplete = true;
	});
	queue.addRequest(query);
	while (!isComplete)
	{
		queue.update();
		std::this_thread::sleep_for(1ms);
	}
	return true;
}
