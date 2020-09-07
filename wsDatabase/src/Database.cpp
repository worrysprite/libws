#include "ws/database/Database.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <functional>

using namespace ws::database;

//===================== Recordset Implements ========================
Recordset::Recordset(MYSQL_RES* res, const std::string& sql) :
	fieldIndex(0), mysqlRow(nullptr), mysqlRes(res), sql(sql)
{
	numFields = mysql_num_fields(mysqlRes);
}

Recordset::~Recordset()
{
	mysql_free_result(mysqlRes);
	mysqlRes = nullptr;
}

bool Recordset::nextRow()
{
	fieldIndex = 0;
	mysqlRow = mysql_fetch_row(mysqlRes);
	return mysqlRow != nullptr;
}

Recordset& Recordset::operator>>(std::string& value)
{
	if (mysqlRow && fieldIndex < numFields)
	{
		const char* szRow = mysqlRow[fieldIndex++];
		if (szRow)
		{
			value = szRow;
		}
		else
		{
			value.clear();
		}
	}
	else
	{
		spdlog::error("mysql fetch field out of range!, sql: {}", sql.c_str());
	}
	return *this;
}

Recordset& Recordset::operator>>(ByteArray& value)
{
	if (mysqlRow && fieldIndex < numFields)
	{
		MYSQL_FIELD* field = mysql_fetch_field_direct(mysqlRes, fieldIndex);
		if (IS_BLOB(field->type))
		{
			unsigned long datasize = field->max_length;
			if (datasize > 0)
			{
				const char* szRow = mysqlRow[fieldIndex++];
				value.writeData(szRow, datasize);
			}
		}
	}
	else
	{
		spdlog::error("mysql fetch field out of range!, sql: {}", sql.c_str());
	}
	return *this;
}

void* Recordset::getBlob(unsigned long& datasize)
{
	void* data = nullptr;
	if (mysqlRow && fieldIndex < numFields)
	{
		MYSQL_FIELD* field = mysql_fetch_field_direct(mysqlRes, fieldIndex);
		if (IS_BLOB(field->type))
		{
			datasize = field->max_length;
			if (datasize > 0)
			{
				const char* szRow = mysqlRow[fieldIndex++];
				data = malloc(datasize);
				memcpy(data, szRow, datasize);
			}
		}
	}
	else
	{
		spdlog::error("mysql fetch field out of range!, sql: {}", sql.c_str());
	}
	return data;
}

//===================== MysqlStatement Implements ========================

DBStatement::DBStatement(const std::string& sql, MYSQL_STMT* mysql_stmt) :
	_sql(sql), stmt(mysql_stmt)
{
	// bind params
	auto numParams = mysql_stmt_param_count(stmt);
	if (numParams)
	{
		paramBind.resize(numParams);
		memset(paramBind.data(), 0, sizeof(MYSQL_BIND) * numParams);
		for (auto &b : paramBind)
		{
			b.length = &b.buffer_length;
		}
	}

	// bind result
	auto resultMetadata = mysql_stmt_result_metadata(stmt);
	if (resultMetadata)
	{
		uint32_t numResultFields = mysql_num_fields(resultMetadata);
		resultBind.resize(numResultFields);
		memset(resultBind.data(), 0, sizeof(MYSQL_BIND) * numResultFields);

		MYSQL_FIELD* fields = mysql_fetch_fields(resultMetadata);
		for (uint32_t i = 0; i < numResultFields; ++i)
		{
			auto& b = resultBind[i];
			b.buffer_type = fields[i].type;
			unsigned long buffer_length(0);
			switch (fields[i].type)
			{
			case MYSQL_TYPE_TINY:
				buffer_length = 1;
				break;
			case MYSQL_TYPE_SHORT:
				buffer_length = 2;
				break;
			case MYSQL_TYPE_LONG:
				buffer_length = 4;
				break;
			case MYSQL_TYPE_LONGLONG:
				buffer_length = 8;
				break;
			case MYSQL_TYPE_FLOAT:
				buffer_length = 4;
				break;
			case MYSQL_TYPE_DOUBLE:
				buffer_length = 8;
				break;
			case MYSQL_TYPE_TIME:
			case MYSQL_TYPE_DATE:
			case MYSQL_TYPE_DATETIME:
			case MYSQL_TYPE_TIMESTAMP:
				buffer_length = sizeof(MYSQL_TIME);
				break;
			default:
				buffer_length = fields[i].length;
				break;
			}
			b.is_unsigned = (fields[i].flags & UNSIGNED_FLAG) > 0;
			b.buffer = malloc(buffer_length);
			b.buffer_length = buffer_length;
			b.is_null = new my_bool;
			b.length = new unsigned long;
		}
		if (mysql_stmt_bind_result(stmt, resultBind.data()))
		{
			spdlog::error("mysql bind result failed: {}", mysql_stmt_error(stmt));
		}
		mysql_free_result(resultMetadata);
	}
}

DBStatement::~DBStatement() 
{
	for (auto& bind : resultBind)
	{
		free(bind.buffer);
		delete bind.is_null;
		delete bind.length;
	}
	if (stmt)
	{
		mysql_stmt_close(stmt);
		stmt = nullptr;
	}
}

DBStatement& DBStatement::operator<<(const std::string& value)
{
	if (paramIndex < numParams())
	{
		auto& b = paramBind[paramIndex];
		if (value.empty())
		{
			b.buffer_type = MYSQL_TYPE_NULL;
		}
		else
		{
			b.buffer_type = MYSQL_TYPE_STRING;
			b.buffer = (void*)value.data();
			b.buffer_length = (unsigned long)value.size();
		}
		++paramIndex;
	}
	else
	{
		spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
	}
	return *this;
}

DBStatement& DBStatement::bindString(const char* value, size_t length)
{
	if (paramIndex < numParams())
	{
		auto& b = paramBind[paramIndex];
		if (value && length)
		{
			b.buffer_type = MYSQL_TYPE_STRING;
			auto& buffer = paramsBuffer.emplace_back(value, length);
			b.buffer = buffer.data();
			b.buffer_length = (unsigned long)length;
		}
		else
		{
			b.buffer_type = MYSQL_TYPE_NULL;
		}
		++paramIndex;
	}
	else
	{
		spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
	}
	return *this;
}

DBStatement& DBStatement::operator<<(const ByteArray& value)
{
	if (paramIndex < numParams())
	{
		auto& b = paramBind[paramIndex];
		if (value.size())
		{
			b.buffer_type = MYSQL_TYPE_BLOB;
			b.buffer = (void*)value.data();
			b.buffer_length = (unsigned long)value.size();
		}
		else
		{
			b.buffer_type = MYSQL_TYPE_NULL;
		}
		++paramIndex;
	}
	else
	{
		spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
	}
	return *this;
}

void DBStatement::bindBlob(void* data, unsigned long size)
{
	if (paramIndex < numParams())
	{
		auto& b = paramBind[paramIndex];
		if (data)
		{
			auto& buffer = paramsBuffer.emplace_back((const char*)data, size);
			b.buffer_type = MYSQL_TYPE_BLOB;
			b.buffer = buffer.data();
			b.buffer_length = size;
		}
		else
		{
			b.buffer_type = MYSQL_TYPE_NULL;
		}
		++paramIndex;
	}
	else
	{
		spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
	}
}

bool DBStatement::execute()
{
	_numRows = 0;
	_lastInsertId = 0;
	if (!paramBind.empty())
	{
		if (mysql_stmt_bind_param(stmt, paramBind.data()))
		{
			spdlog::error("mysql bind params error");
			return false;
		}
	}
	if (mysql_stmt_execute(stmt))
	{
		spdlog::error("mysql execute failed: {}, sql={}", mysql_stmt_error(stmt), _sql.c_str());
		return false;
	}
	if (!resultBind.empty())
	{
		if (mysql_stmt_store_result(stmt))
		{
			spdlog::error("mysql store result failed: {}", mysql_stmt_error(stmt));
			return false;
		}
	}
	_numRows = mysql_stmt_affected_rows(stmt);
	_lastInsertId = mysql_stmt_insert_id(stmt);
	return true;
}

bool DBStatement::nextRow()
{
	resultIndex = 0;
	int code = mysql_stmt_fetch(stmt);
	switch (code)
	{
	case 0:
		return true;
	case MYSQL_NO_DATA:
		return false;
	case MYSQL_DATA_TRUNCATED:
		spdlog::warn("mysql fetch truncated! sql={}", _sql.c_str());
		return true;
	default:
		spdlog::error("mysql fetch error: {}", mysql_stmt_error(stmt));
		return false;
	}
}

void DBStatement::clear()
{
	memset(paramBind.data(), 0, sizeof(MYSQL_BIND) * paramBind.size());
	for (auto& b : paramBind)
	{
		b.length = &b.buffer_length;
	}
	paramsBuffer.clear();

	for (auto &bind : resultBind)
	{
		memset(bind.buffer, 0, bind.buffer_length);
	}
	paramIndex = 0;
	resultIndex = 0;
}

DBStatement& DBStatement::operator>>(std::string& value)
{
	if (resultIndex < numResultFields())
	{
		auto& b = resultBind[resultIndex];
		if (*b.is_null)
		{
			value.clear();
		}
		else
		{
			value.assign((char*)b.buffer, *b.length);
		}
		++resultIndex;
	}
	else
	{
		spdlog::error("mysql get result out of range! sql={}", _sql.c_str());
	}
	return *this;
}

DBStatement& DBStatement::operator>>(ByteArray& value)
{
	if (resultIndex < numResultFields())
	{
		auto& b = resultBind[resultIndex];
		if (!*b.is_null)
		{
			value.writeData(b.buffer, *b.length);
		}
		++resultIndex;
	}
	else
	{
		spdlog::error("mysql get result out of range! sql={}", _sql.c_str());
	}
	return *this;
}

void* DBStatement::getBlob(size_t& datasize)
{
	void* data = nullptr;
	if (resultIndex < numResultFields())
	{
		auto& b = resultBind[resultIndex];
		datasize = *b.length;
		if (!(*b.is_null) && datasize > 0)
		{
			data = malloc(datasize);
			memcpy(data, b.buffer, datasize);
		}
		++resultIndex;
	}
	else
	{
		spdlog::error("mysql get result out of range! sql={}", _sql.c_str());
	}
	return data;
}

//===================== Database Implements ========================
Database::~Database()
{
	logoff();
}

std::mutex Database::initMtx;

void Database::setDBConfig(const MySQLConfig& config)
{
	dbConfig = config;
}

bool Database::logon()
{
	initMtx.lock();
	logoff();
	mysql = mysql_init(nullptr);
	initMtx.unlock();
	if (!mysql)
		return false;

	MYSQL* pMysql = mysql_real_connect(mysql, dbConfig.strHost.c_str(),
		dbConfig.strUser.c_str(), dbConfig.strPassword.c_str(),
		dbConfig.strDB.c_str(), dbConfig.nPort, dbConfig.strUnixSock.c_str(), CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS);

	if (pMysql)
	{
		mysql_set_character_set(pMysql, "utf8mb4");
		if (dbConfig.autoCommit)
		{
			mysql_autocommit(pMysql, 1);
		}
		spdlog::info("mysql connect successful.");
		return true;
	}
	spdlog::error("Fail to connect to mysql: {}", mysql_error(mysql));
	return false;
}

void Database::logoff()
{
	if (mysql)
	{
		mysql_close(mysql);
		mysql = nullptr;
		stmtCache.clear();
	}
}

void Database::changeDatabase(const char* db)
{
	dbConfig.strDB = db;
	if (mysql)
	{
		if (mysql_select_db(mysql, db))
		{
			spdlog::error("Change db failed! db={}", db);
		}
	}
}

RecordsetPtr Database::query(const char* strSQL, int nCommit /*= 1*/)
{
	RecordsetPtr record;
	const char* pError = nullptr;
	if (mysql_query(mysql, strSQL) == 0)
	{
		_hasError = false;
		numAffectedRows = mysql_affected_rows(mysql);
		MYSQL_RES* pMysqlRes = mysql_store_result(mysql);
		while (mysql_next_result(mysql) == 0x00);

		if (pMysqlRes)
		{
			numResultRows = mysql_num_rows(pMysqlRes);
			if (numResultRows > 0)
			{
				record = std::make_unique<Recordset>(pMysqlRes, strSQL);
			}
			else
			{
				mysql_free_result(pMysqlRes);
			}
			if (nCommit)
			{
				mysql_commit(mysql);
			}
		}
	}
	else
	{
		_hasError = true;
		numResultRows = 0;
		numAffectedRows = 0;
		pError = mysql_error(mysql);
		if (pError)
		{
			spdlog::error("DBError: {}, SQL: {}", pError, strSQL);
		}
	}
	return record;
}

DBStatement* Database::prepare(const std::string& sql)
{
	DBStatement* dbStmt = nullptr;
	auto iter = stmtCache.find(sql);
	if (iter != stmtCache.end())
	{
		dbStmt = iter->second.get();
		dbStmt->clear();
	}
	else
	{
		MYSQL_STMT* stmt = mysql_stmt_init(mysql);
		if (0 != mysql_stmt_prepare(stmt, sql.c_str(), (unsigned long)sql.length()))
		{
			spdlog::error("mysql prepare error, sql={}, error={}", sql.c_str(), mysql_stmt_error(stmt));
			mysql_stmt_close(stmt);
			return nullptr;
		}

		dbStmt = new DBStatement(sql, stmt);
		stmtCache.insert(std::make_pair(sql, std::unique_ptr<DBStatement>(dbStmt)));
	}
	return dbStmt;
}

//===================== DBRequestQueue Implements ========================
DBQueue::~DBQueue()
{
	while (size_t queueLength = getQueueLength())
	{
		spdlog::debug("DBQueue waiting for db requests complete, remaining: {}", queueLength);
		update();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	for (auto &th : workerThreads)
	{
		th->isExit = true;
		th->thread->join();
		spdlog::debug("DBQueue worker thread joined.");
	}
	//finishQueue maybe not empty after workers joined.
	update();
}

void DBQueue::setThread(size_t numThread)
{
	if (workerThreads.size() < numThread)
	{
		//add more threads
		for (size_t i = workerThreads.size(); i < numThread; i++)
		{
			auto worker = std::make_shared<WorkerThread>();
			worker->thread = std::make_unique<std::thread>(std::bind(&DBQueue::DBWorkThread, this, std::placeholders::_1), worker);
			workerThreads.push_back(worker);
		}
	}
	else
	{
		//remove threads
		while (workerThreads.size() > numThread)
		{
			auto &worker = workerThreads.back();
			//worker->thread->detach();
			worker->isExit = true;
			worker->thread->join();
			workerThreads.pop_back();
		}
	}
}

void DBQueue::addRequest(DBRequestPtr request)
{
	std::lock_guard<std::mutex> lock(workMtx);
	workQueue.push(request);
	workQueueLength = workQueue.size();
}

// main thread
void DBQueue::update()
{
	DBRequestList tmpQueue;
	finishMtx.lock();
	if (!finishQueue.empty())
	{
		tmpQueue.swap(finishQueue);
	}
	finishMtx.unlock();
	
	while (!tmpQueue.empty())
	{
		DBRequestPtr request = tmpQueue.front();
		tmpQueue.pop();
		request->onFinish();
	}
}

DBRequestPtr DBQueue::getRequest()
{
	DBRequestPtr request;
	std::lock_guard<std::mutex> lock(workMtx);
	if (!workQueue.empty())
	{
		request = workQueue.front();
		workQueue.pop();
		workQueueLength = workQueue.size();
	}
	return request;
}

void DBQueue::DBWorkThread(WorkerThreadPtr worker)
{
	Database db;
	db.setDBConfig(config);
	DBRequestPtr request;
	const std::chrono::milliseconds requestWait(100);
	const std::chrono::microseconds connectWait(500);
	
	while (!worker->isExit)
	{
		if (!(request = getRequest()))
		{
			std::this_thread::sleep_for(requestWait);
			continue;
		}
		while (!db.isConnected())
		{
			db.logon();
			std::this_thread::sleep_for(connectWait);
		}
		request->onRequest(db);
		std::lock_guard<std::mutex> lock(finishMtx);
		finishQueue.push(request);
	}
}
