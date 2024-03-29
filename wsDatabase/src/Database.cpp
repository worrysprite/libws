#include "ws/database/Database.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <chrono>
#include <map>

using namespace ws::database;
using namespace std::chrono;

//typedef bool my_bool;

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
		auto lengths = mysql_fetch_lengths(mysqlRes);
		auto datasize = lengths[fieldIndex];
		const char* szRow = mysqlRow[fieldIndex++];
		if (szRow && datasize)
		{
			value.assign(szRow, datasize);
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
			auto lengths = mysql_fetch_lengths(mysqlRes);
			auto datasize = lengths[fieldIndex];
			if (datasize > 0)
			{
				const char* szRow = mysqlRow[fieldIndex];
				value.writeData(szRow, datasize);
			}
			++fieldIndex;
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
				const char* szRow = mysqlRow[fieldIndex];
				data = malloc(datasize);
				memcpy(data, szRow, datasize);
			}
			++fieldIndex;
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
	stmt(mysql_stmt), _sql(sql)
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
			b.is_null = new bool;
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

void Database::setDBConfig(const MySQLConfig& config)
{
	dbConfig = config;
}

bool Database::logon()
{
	static std::mutex initMtx;
	initMtx.lock();
	logoff();
	mysql = mysql_init(nullptr);
	initMtx.unlock();
	if (!mysql)
		return false;

	//此处不能使用自动连接，因为缓存了statement，连接断开时缓存失效必须清理重连
	if (mysql == mysql_real_connect(mysql, dbConfig.host.c_str(),
		dbConfig.user.c_str(), dbConfig.password.c_str(),
		dbConfig.database.c_str(), dbConfig.port, dbConfig.unixSock.c_str(),
		CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS))
	{
		mysql_set_character_set(mysql, dbConfig.characterset.c_str());
		mysql_autocommit(mysql, dbConfig.autoCommit);
		spdlog::info("mysql connect successful.");
		return true;
	}
	spdlog::error("Failed to connect to mysql: {}", mysql_error(mysql));
	return false;
}

void Database::logoff()
{
	if (mysql)
	{
		stmtCache.clear();
		mysql_close(mysql);
		mysql = nullptr;
	}
}

bool Database::changeDatabase(const std::string& db)
{
	if (mysql && !mysql_select_db(mysql, db.c_str()))
	{
		dbConfig.database = db;
		return true;
	}
	spdlog::error("Change db failed! db={}", db);
	return false;
}

DBStatement* Database::prepare(const std::string& sql)
{
	DBStatement* dbStmt = nullptr;
	auto iter = stmtCache.find(sql);
	if (iter != stmtCache.end())
	{
		dbStmt = iter->second.get();
		dbStmt->lastUseTime = std::chrono::steady_clock::now();
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

		if (stmtCache.size() >= dbConfig.maxStmtCache)
		{
			std::map<std::chrono::steady_clock::time_point, decltype(stmtCache)::iterator> sortedMap;
			for (auto iter = stmtCache.begin(); iter != stmtCache.end(); ++iter)
			{
				sortedMap.emplace(iter->second->lastUseTime, iter);
			}
			auto iter = sortedMap.begin();
			for (uint32_t i = 0; i < dbConfig.maxStmtCache >> 1; ++i)
			{
				stmtCache.erase(iter->second);
				++iter;
			}
		}

		dbStmt = new DBStatement(sql, stmt);
		dbStmt->lastUseTime = std::chrono::steady_clock::now();
		stmtCache.emplace(sql, DBStatementPtr(dbStmt));
	}
	return dbStmt;
}

bool Database::query(const std::string& strSQL)
{
	//忽略上一个查询的其他结果集
	while (mysql_next_result(mysql) == 0)
	{
		if (mysql_field_count(mysql) > 0)
		{
			MYSQL_RES* mysqlResult = mysql_store_result(mysql);
			if (mysqlResult)
			{
				mysql_free_result(mysqlResult);
			}
		}
	}

	if (mysql_query(mysql, strSQL.c_str()) == 0)
	{
		lastSQL = strSQL;
		if (mysql_field_count(mysql) > 0)
		{
			MYSQL_RES* mysqlResult = mysql_store_result(mysql);
			if (mysqlResult)
			{
				numResultRows = mysql_num_rows(mysqlResult);
				if (numResultRows > 0)
				{
					lastRecords = std::make_unique<Recordset>(mysqlResult, strSQL);
				}
				else
				{
					mysql_free_result(mysqlResult);
				}
			}
		}
		else
		{
			lastRecords.reset();
			numAffectedRows = mysql_affected_rows(mysql);
		}
		return true;
	}

	lastSQL.clear();
	lastRecords.reset();
	numResultRows = 0;
	numAffectedRows = 0;
	spdlog::error("query error: {}, SQL: {}", mysql_error(mysql), strSQL.c_str());
	return false;
}

bool Database::nextRecordset()
{
	if (mysql_next_result(mysql) == 0)
	{
		if (mysql_field_count(mysql) > 0)
		{
			MYSQL_RES* mysqlResult = mysql_store_result(mysql);
			if (mysqlResult)
			{
				numResultRows = mysql_num_rows(mysqlResult);
				if (numResultRows > 0)
				{
					lastRecords = std::make_unique<Recordset>(mysqlResult, lastSQL);
				}
				else
				{
					mysql_free_result(mysqlResult);
				}
			}
		}
		else
		{
			lastRecords.reset();
			numAffectedRows = mysql_affected_rows(mysql);
		}
		return true;
	}
	return false;
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
		th.isExit = true;
		th.thread.join();
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
			auto& worker = workerThreads.emplace_back();
			worker.thread = std::thread(&DBQueue::DBWorkThread, this, std::ref(worker));
		}
	}
	else
	{
		//remove threads
		while (workerThreads.size() > numThread)
		{
			auto &worker = workerThreads.back();
			//worker->thread->detach();
			worker.isExit = true;
			worker.thread.join();
			workerThreads.pop_back();
		}
	}
}

void DBQueue::addRequest(DBRequestPtr request)
{
	std::lock_guard<std::mutex> lock(workMtx);
	workQueue.push_back(request);
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
	
	for (auto &request : tmpQueue)
	{
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
		workQueue.pop_front();
		workQueueLength = workQueue.size();
	}
	return request;
}

void DBQueue::DBWorkThread(const WorkerThread& worker)
{
	Database db;
	db.setDBConfig(config);

	DBRequestPtr request;
	while (!worker.isExit)
	{
		if (!(request = getRequest()))
		{
			std::this_thread::sleep_for(100ms);
			continue;
		}
		int retries = 0;
		while (!db.isConnected())
		{
			if (retries > 0)
			{
				spdlog::error("unable to connect mysql host[{}:{}]!", config.host, config.port);
				std::this_thread::sleep_for(5s);
			}
			db.logon();
			++retries;
		}
		request->onRequest(db);
		std::lock_guard<std::mutex> lock(finishMtx);
		finishQueue.push_back(request);
	}
}
