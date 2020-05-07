#include "ws/database/Database.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <functional>

using namespace ws::database;

//===================== Recordset Implements ========================
Recordset::Recordset(MYSQL_RES* pMysqlRes, const std::string& sql) :
	fieldIndex(0), mysqlRow(nullptr), mysqlRes(pMysqlRes), sql(sql)
{
	numFields = mysql_num_fields(mysqlRes);
}

Recordset::~Recordset()
{
	mysql_free_result(mysqlRes);
	mysqlRes = NULL;
}

bool Recordset::nextRow()
{
	fieldIndex = 0;
	mysqlRow = mysql_fetch_row(mysqlRes);
	return (mysqlRow != NULL);
}

Recordset& Recordset::operator>>(float& value)
{
	if (mysqlRow && fieldIndex < numFields)
	{
		const char* szRow = mysqlRow[fieldIndex++];
		if (szRow)
		{
			value = strtof(szRow, NULL);
		}
	}
	else
	{
		Log::e("mysql fetch field out of range!, sql: %s", sql.c_str());
	}
	return *this;
}

Recordset& Recordset::operator>>(double& value)
{
	if (mysqlRow && fieldIndex < numFields)
	{
		const char* szRow = mysqlRow[fieldIndex++];
		if (szRow)
		{
			value = strtod(szRow, NULL);
		}
	}
	else
	{
		Log::e("mysql fetch field out of range!, sql: %s", sql.c_str());
	}
	return *this;
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
	}
	else
	{
		Log::e("mysql fetch field out of range!, sql: %s", sql.c_str());
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
		Log::e("mysql fetch field out of range!, sql: %s", sql.c_str());
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
		Log::e("mysql fetch field out of range!, sql: %s", sql.c_str());
	}
	return data;
}

//===================== MysqlStatement Implements ========================

DBStatement::DBStatement(const std::string& sql, MYSQL_STMT* mysql_stmt) : paramIndex(0),
	_numParams(0), resultIndex(0), _numResultFields(0), _numRows(0), _lastInsertId(0),
	_sql(sql), stmt(mysql_stmt), paramBind(nullptr), resultBind(nullptr),
	resultMetadata(nullptr), paramsBuffer(nullptr)
{
	// bind params
	_numParams = (int)mysql_stmt_param_count(stmt);
	if (_numParams > 0)
	{
		paramBind = new MYSQL_BIND[_numParams];
		memset(paramBind, 0, sizeof(MYSQL_BIND)* _numParams);
		paramsBuffer = new void*[_numParams];
		memset(paramsBuffer, 0, sizeof(void*)* _numParams);
	}

	// bind result
	resultMetadata = mysql_stmt_result_metadata(stmt);
	if (resultMetadata)
	{
		_numResultFields = mysql_num_fields(resultMetadata);
		resultBind = new MYSQL_BIND[_numResultFields];
		memset(resultBind, 0, sizeof(MYSQL_BIND)* _numResultFields);
		MYSQL_FIELD* fields = mysql_fetch_fields(resultMetadata);
		for (int i = 0; i < _numResultFields; ++i)
		{
			resultBind[i].buffer_type = fields[i].type;
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
				buffer_length = sizeof(MYSQL_TIME);
				break;
			default:
				buffer_length = fields[i].length;
				break;
			}
			resultBind[i].is_unsigned = (fields[i].flags & UNSIGNED_FLAG) > 0;
			resultBind[i].buffer = malloc(buffer_length);
			resultBind[i].buffer_length = buffer_length;
			resultBind[i].is_null = new my_bool(0);
			resultBind[i].length = new unsigned long(0);
		}
		if (mysql_stmt_bind_result(stmt, resultBind))
		{
			Log::e("mysql bind result failed: %s", mysql_stmt_error(stmt));
		}
	}
}

DBStatement::~DBStatement() 
{
	if (paramBind)
	{
		delete[] paramBind;
		paramBind = nullptr;
	}
	if (paramsBuffer)
	{
		for (int i = 0; i < _numParams; ++i)
		{
			free(paramsBuffer[i]);
		}
		delete[] paramsBuffer;
		paramsBuffer = nullptr;
	}
	if (resultBind)
	{
		for (int i = 0; i < _numResultFields; ++i)
		{
			free(resultBind[i].buffer);
			delete resultBind[i].is_null;
			delete resultBind[i].length;
		}
		delete[] resultBind;
		resultBind = nullptr;
	}
	mysql_free_result(resultMetadata);
	if (stmt)
	{
		mysql_stmt_close(stmt);
		stmt = nullptr;
	}
}

DBStatement& DBStatement::operator<<(const std::string& value)
{
	if (paramIndex < _numParams)
	{
		MYSQL_BIND& b = paramBind[paramIndex];
		b.buffer_type = MYSQL_TYPE_VAR_STRING;
		b.buffer = malloc(value.size());
		memcpy(b.buffer, value.data(), value.size());
		paramsBuffer[paramIndex] = b.buffer;
		b.buffer_length = (unsigned long)value.size();
		b.length = &b.buffer_length;
		++paramIndex;
	}
	else
	{
		Log::e("mysql bind params out of range! sql=%s", _sql.c_str());
	}
	return *this;
}

DBStatement& DBStatement::bindString(const char* value, unsigned long length)
{
	if (paramIndex < _numParams)
	{
		MYSQL_BIND& b = paramBind[paramIndex];
		b.buffer_type = MYSQL_TYPE_VAR_STRING;
		b.buffer = (void*)value;
		b.buffer_length = length;
		++paramIndex;
	}
	else
	{
		Log::e("mysql bind params out of range! sql=%s", _sql.c_str());
	}
	return *this;
}

DBStatement& DBStatement::operator<<(const ByteArray& value)
{
	if (paramIndex < _numParams)
	{
		MYSQL_BIND& b = paramBind[paramIndex];
		b.buffer_type = MYSQL_TYPE_BLOB;
		b.buffer = malloc(value.size());
		memcpy(b.buffer, value.data(), value.size());
		paramsBuffer[paramIndex] = b.buffer;
		b.buffer_length = (unsigned long)value.size();
		++paramIndex;
	}
	else
	{
		Log::e("mysql bind params out of range! sql=%s", _sql.c_str());
	}
	return *this;
}

void DBStatement::bindBlob(enum_field_types type, void* data, unsigned long size)
{
	if (paramIndex < _numParams)
	{
		MYSQL_BIND& b = paramBind[paramIndex];
		if (data)
		{
			b.buffer_type = type;
			b.buffer = data;
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
		Log::e("mysql bind params out of range! sql=%s", _sql.c_str());
	}
}

bool DBStatement::execute()
{
	_numRows = 0;
	_lastInsertId = 0;
	if (_numParams > 0)
	{
		if (mysql_stmt_bind_param(stmt, paramBind))
		{
			Log::e("mysql bind params error");
			return false;
		}
	}
	if (mysql_stmt_execute(stmt))
	{
		Log::e("mysql execute failed: %s, sql=%s", mysql_stmt_error(stmt), _sql.c_str());
		return false;
	}
	if (resultMetadata)
	{
		if (mysql_stmt_store_result(stmt))
		{
			Log::e("mysql store result failed: %s", mysql_stmt_error(stmt));
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
		Log::w("mysql fetch truncated! sql=%s", _sql.c_str());
		return true;
	default:
		Log::e("mysql fetch error: %s", mysql_stmt_error(stmt));
		return false;
	}
}

void DBStatement::reset()
{
	memset(paramBind, 0, sizeof(MYSQL_BIND)* _numParams);
	for (int i = 0; i < _numParams; ++i)
	{
		free(paramsBuffer[i]);
		paramsBuffer[i] = nullptr;
	}
	for (int i = 0; i < _numResultFields; ++i)
	{
		memset(resultBind[i].buffer, 0, resultBind[i].buffer_length);
		*resultBind[i].is_null = 0;
		*resultBind[i].length = 0;
	}
	paramIndex = 0;
	resultIndex = 0;
}

DBStatement& DBStatement::operator>>(std::string& value)
{
	if (resultIndex < _numResultFields)
	{
		if (*resultBind[resultIndex].is_null)
		{
			value.clear();
		}
		else
		{
			value.assign((char*)resultBind[resultIndex].buffer, *resultBind[resultIndex].length);
		}
		++resultIndex;
	}
	else
	{
		Log::e("mysql get result out of range! sql=%s", _sql.c_str());
	}
	return *this;
}

DBStatement& DBStatement::operator>>(ByteArray& value)
{
	if (resultIndex < _numResultFields)
	{
		value.truncate();
		if (!*resultBind[resultIndex].is_null)
		{
			value.writeData(resultBind[resultIndex].buffer, *resultBind[resultIndex].length);
		}
		++resultIndex;
	}
	else
	{
		Log::e("mysql get result out of range! sql=%s", _sql.c_str());
	}
	return *this;
}

void* DBStatement::getBlob(unsigned long& datasize)
{
	void* data = nullptr;
	datasize = *resultBind[resultIndex].length;
	if (resultIndex < _numResultFields)
	{
		if (!(*resultBind[resultIndex].is_null) && datasize > 0)
		{
			data = malloc(datasize);
			memcpy(data, resultBind[resultIndex].buffer, datasize);
		}
		++resultIndex;
	}
	else
	{
		Log::e("mysql get result out of range! sql=%s", _sql.c_str());
	}
	return data;
}

//===================== Database Implements ========================
Database::~Database()
{
	logoff();
}

std::mutex Database::initMtx;

void Database::setDBConfig(const MYSQL_CONFIG& config)
{
	dbConfig = config;
}

bool Database::logon()
{
	initMtx.lock();
	logoff();
	mysql = mysql_init(NULL);
	initMtx.unlock();
	if (mysql == NULL) return false;

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
		Log::i("mysql connect successful.");
		return true;
	}
	Log::e("Fail to connect to mysql: %s", mysql_error(mysql));
	return false;
}

void Database::logoff()
{
	if (mysql != NULL)
	{
		mysql_close(mysql);
		mysql = NULL;

		for (auto &cache : stmtCache)
		{
			delete cache.second;
		}
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
			Log::e("Change db failed! db=%s", db);
		}
	}
}

RecordsetPtr Database::query(const char* strSQL, int nCommit /*= 1*/)
{
	RecordsetPtr record;
	const char* pError = NULL;
	if (mysql_query(mysql, strSQL) == 0)
	{
		_hasError = false;
		numAffectedRows = mysql_affected_rows(mysql);
		MYSQL_RES* pMysqlRes = mysql_store_result(mysql);
		while (mysql_next_result(mysql) == 0x00);

		if (pMysqlRes != NULL)
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
			Log::e("DBError: %s, SQL: %s", pError, strSQL);
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
		dbStmt = iter->second;
		dbStmt->reset();
	}
	else
	{
		MYSQL_STMT* stmt = mysql_stmt_init(mysql);
		if (0 != mysql_stmt_prepare(stmt, sql.c_str(), (unsigned long)sql.length()))
		{
			Log::e("mysql prepare error, sql=%s, error=%s", sql.c_str(), mysql_stmt_error(stmt));
			mysql_stmt_close(stmt);
			return nullptr;
		}

		dbStmt = new DBStatement(sql, stmt);
		stmtCache.insert(std::make_pair(sql, dbStmt));
	}
	return dbStmt;
}

void Database::freeStatement(const std::string& sql)
{
	auto iter = stmtCache.find(sql);
	if (iter != stmtCache.end())
	{
		delete iter->second;
		stmtCache.erase(iter);
	}
}

//===================== DBRequestQueue Implements ========================
DBQueue::~DBQueue()
{
	while (workQueueLength > 0)
	{
		Log::d("DBQueue waiting for db requests complete, remaining: %d", workQueueLength);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	this->isExit = true;
	for (auto &th : workerThreads)
	{
		th->join();
		Log::d("DBQueue worker thread joined.");
	}
}

void DBQueue::setThread(uint32_t numThread)
{
	if (workerThreads.size() < numThread)
	{
		//add more threads
		for (uint32_t i = workerThreads.size(); i < numThread; i++)
		{
			workerThreads.push_back(std::make_unique<std::thread>(std::bind(&DBQueue::DBWorkThread, this)));
		}
	}
	else
	{
		//remove threads
		while (workerThreads.size() > numThread)
		{
			workerThreads.back()->detach();
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
	if (!finishQueue.empty())
	{
		std::lock_guard<std::mutex> lock(finishMtx);
		tmpQueue.swap(finishQueue);
	}
	while (!tmpQueue.empty())
	{
		DBRequestPtr request = tmpQueue.front();
		tmpQueue.pop_front();
		request->onFinish();
	}
}

DBRequestPtr DBQueue::getRequest()
{
	DBRequestPtr request(nullptr);
	std::lock_guard<std::mutex> lock(workMtx);
	if (!workQueue.empty())
	{
		request = workQueue.front();
		workQueue.pop_front();
		workQueueLength = workQueue.size();
	}
	return request;
}

void DBQueue::finishRequest(DBRequestPtr request)
{
	std::lock_guard<std::mutex> lock(finishMtx);
	finishQueue.push_back(request);
}

void DBQueue::DBWorkThread()
{
	Database db;
	db.setDBConfig(config);
	DBRequestPtr request;
	const std::chrono::milliseconds requestWait(100);
	const std::chrono::microseconds connectWait(500);
	while (!this->isExit)
	{
		if (!request && !(request = getRequest()))
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
		finishRequest(request);
		request.reset();
	}
}
