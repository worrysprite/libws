#include <functional>
#include <iostream>
#include "ws/database/Database.h"

using namespace ws::database;

class TestQuery : public DBRequest
{
public:
	TestQuery() :result(0) {}
	typedef std::function<void(int)> Callback;

	void testSomeOperate(Callback cb)
	{
		callback = cb;
	}

private:
	virtual void onRequest(Database& db)
	{
		auto record = db.query("SELECT COUNT(*) FROM test_table");
		if (record && record->nextRow())
		{
			*record >> result;
		}
	}

	virtual void onFinish()
	{
		callback(result);
	}

private:
	int result;
	Callback callback;
};

bool testDatabase()
{
	std::cout << "====================Test Database====================" << std::endl;
	MYSQL_CONFIG dbConfig;
	dbConfig.strHost = "127.0.0.1";
	dbConfig.nPort = 3306;
	dbConfig.strUser = "root";
	dbConfig.strPassword = "123456";
	dbConfig.strDB = "testdb";
#if defined(__linux__) || defined(__APPLE__)
	dbConfig.strUnixSock = "/tmp/mysql.sock";
#endif

	DBQueue queue(dbConfig);
	queue.setThread(1);

	bool isComplete = false;
	auto query = std::make_shared<TestQuery>();
	query->testSomeOperate([&isComplete](int result){
		std::cout << "TestQuery result=" << result << std::endl;
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
