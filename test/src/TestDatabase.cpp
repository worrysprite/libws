#include <functional>
#include "ws/database/Database.h"

using namespace ws::database;

class TestQuery : public DBRequest
{
public:
	TestQuery(){}

	typedef std::function<void()> Callback;

	void testSomeOperate(Callback cb)
	{
		callback = cb;
	}

	virtual void onRequest(Database& db)
	{


		callback();
	}

	virtual void onFinish()
	{

	}

protected:
private:
	Callback callback;
};

bool testDataBase()
{
	MYSQL_CONFIG dbConfig;
	dbConfig.strHost = "localhost";
	dbConfig.nPort = 3306;
	dbConfig.strUser = "root";
	dbConfig.strPassword = "123456";
	dbConfig.strDB = "test";
#if defined(__linux__) || defined(__APPLE__)
	dbConfig.strUnixSock = "/tmp/mysql.sock";
#endif

	DBQueue queue;
	queue.addThread(1, dbConfig);

	bool isComplete = false;
	std::shared_ptr<TestQuery> query(new TestQuery);
	

	queue.addQueueMsg(query);

	while (!isComplete)
	{

	}
	return true;
}
