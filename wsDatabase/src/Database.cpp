#include "ws/database/Database.h"
#include "ws/core/Log.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <functional>

using namespace ws::database;

//===================== Recordset Implements ========================
Recordset::Recordset(MYSQL_RES* pMysqlRes) :mysqlRes(pMysqlRes)
{
	numFields = mysql_num_fields(mysqlRes);
	mysqlRow = NULL;
}

Recordset::~Recordset()
{
	mysql_free_result(mysqlRes);
	mysqlRes = NULL;
}

bool Recordset::nextRow()
{
	fieldIndex = 0x00;
	mysqlRow = mysql_fetch_row(mysqlRes);
	return (mysqlRow != NULL);
}

Recordset& Recordset::operator >> (int8_t& value)
{
	return getInt(value);
}

Recordset& Recordset::operator >> (uint8_t& value)
{
	return getInt(value);
}

Recordset& Recordset::operator >> (int16_t& value)
{
	return getInt(value);
}

Recordset& Recordset::operator >> (uint16_t& value)
{
	return getInt(value);
}

Recordset& Recordset::operator >> (int32_t& value)
{
	return getInt(value);
}

Recordset& Recordset::operator >> (uint32_t& value)
{
	return getInt(value);
}

Recordset& Recordset::operator>>(int64_t& value)
{
	if (mysqlRow && fieldIndex < numFields)
	{
		const char* szRow = mysqlRow[fieldIndex++];
		if (szRow)
		{
			value = atoll(szRow);
		}
	}
	else
	{
		Log::e("mysql fetch field out of range!");
	}
	return *this;
}

Recordset& Recordset::operator >> (uint64_t& value)
{
	if (mysqlRow && fieldIndex < numFields)
	{
		const char* szRow = mysqlRow[fieldIndex++];
		if (szRow)
		{
			value = strtoull(szRow, nullptr, 10);
		}
	}
	else
	{
		Log::e("mysql fetch field out of range!");
	}
	return *this;
}

Recordset& Recordset::operator >> (std::string& value)
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
		Log::e("mysql fetch field out of range!");
	}
	return *this;
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
		Log::e("mysql fetch field out of range!");
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
		Log::e("mysql fetch field out of range!");
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
		Log::e("mysql fetch field out of range!");
	}
	return *this;
}

template<typename T>
Recordset& Recordset::getInt(T& value)
{
	if (mysqlRow && fieldIndex < numFields)
	{
		const char* szRow = mysqlRow[fieldIndex++];
		if (szRow)
		{
			value = atoi(szRow);
		}
	}
	else
	{
		Log::e("mysql fetch field out of range!");
	}
	return *this;
}

void* Recordset::getBlob(unsigned long& datasize)
{
	if (mysqlRow && fieldIndex < numFields)
	{
		MYSQL_FIELD* field = mysql_fetch_field_direct(mysqlRes, fieldIndex);
		if (IS_BLOB(field->type))
		{
			datasize = field->max_length;
			if (datasize > 0)
			{
				const char* szRow = mysqlRow[fieldIndex++];
				void* data = malloc(datasize);
				memcpy(data, szRow, datasize);
				return data;
			}
		}
	}
	else
	{
		Log::e("mysql fetch field out of range!");
	}
	return nullptr;
}

void Recordset::skipFields(int num)
{
	fieldIndex += num;
}

//===================== MysqlStatement Implements ========================

DBStatement::DBStatement(const char* sql, MYSQL_STMT* mysql_stmt) :
	_strSQL(sql), stmt(mysql_stmt), paramBind(nullptr), paramIndex(0), _numParams(0), paramsBuffer(nullptr),
	_numResultFields(0), resultBind(nullptr), resultMetadata(nullptr), _numRows(0), _lastInsertId(0)
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

template<typename T>
DBStatement& DBStatement::bindNumberParam(T& value, enum_field_types type, bool isUnsigned)
{
	if (paramIndex < _numParams)
	{
		MYSQL_BIND& b = paramBind[paramIndex++];
		b.buffer_type = type;
		b.buffer = &value;
		b.is_unsigned = isUnsigned;
	}
	else
	{
		Log::e("mysql bind params out of range! sql=%s", _strSQL.c_str());
	}
	return *this;
}

DBStatement& DBStatement::operator<<(int8_t& value)
{
	return bindNumberParam(value, MYSQL_TYPE_TINY, false);
}

DBStatement& DBStatement::operator<<(uint8_t& value)
{
	return bindNumberParam(value, MYSQL_TYPE_TINY, true);
}

DBStatement& DBStatement::operator<<(int16_t& value)
{
	return bindNumberParam(value, MYSQL_TYPE_SHORT, false);
}

DBStatement& DBStatement::operator<<(uint16_t& value)
{
	return bindNumberParam(value, MYSQL_TYPE_SHORT, true);
}

DBStatement& DBStatement::operator<<(int32_t& value)
{
	return bindNumberParam(value, MYSQL_TYPE_LONG, false);
}

DBStatement& DBStatement::operator<<(uint32_t& value)
{
	return bindNumberParam(value, MYSQL_TYPE_LONG, true);
}

DBStatement& DBStatement::operator<<(int64_t& value)
{
	return bindNumberParam(value, MYSQL_TYPE_LONGLONG, false);
}

DBStatement& DBStatement::operator<<(uint64_t& value)
{
	return bindNumberParam(value, MYSQL_TYPE_LONGLONG, true);
}

DBStatement& DBStatement::operator<<(float& value)
{
	return bindNumberParam(value, MYSQL_TYPE_FLOAT, false);
}

DBStatement& DBStatement::operator<<(double& value)
{
	return bindNumberParam(value, MYSQL_TYPE_DOUBLE, false);
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
		Log::e("mysql bind params out of range! sql=%s", _strSQL.c_str());
	}
	return *this;
}

DBStatement& DBStatement::operator<<(ByteArray& value)
{
	if (paramIndex < _numParams)
	{
		MYSQL_BIND& b = paramBind[paramIndex];
		b.buffer_type = MYSQL_TYPE_BLOB;
		b.buffer = malloc(value.size());
		memcpy(b.buffer, value.data(), value.size());
		paramsBuffer[paramIndex] = b.buffer;
		b.buffer_length = (unsigned long)value.size();
		b.length = &b.buffer_length;
		++paramIndex;
	}
	else
	{
		Log::e("mysql bind params out of range! sql=%s", _strSQL.c_str());
	}
	return *this;
}

DBStatement& DBStatement::bindString(const char* value, unsigned long* length)
{
	if (paramIndex < _numParams)
	{
		MYSQL_BIND& b = paramBind[paramIndex];
		b.buffer_type = MYSQL_TYPE_VAR_STRING;
		b.buffer = (void*)value;
		b.buffer_length = *length;
		b.length = length;
		++paramIndex;
	}
	else
	{
		Log::e("mysql bind params out of range! sql=%s", _strSQL.c_str());
	}
	return *this;
}

void DBStatement::bindBlob(enum_field_types type, void* data, unsigned long* size)
{
	if (paramIndex < _numParams)
	{
		MYSQL_BIND& b = paramBind[paramIndex];
		if (data)
		{
			b.buffer_type = type;
			b.buffer = data;
			b.buffer_length = *size;
			b.length = size;
		}
		else
		{
			b.buffer_type = MYSQL_TYPE_NULL;
		}
		++paramIndex;
	}
	else
	{
		Log::e("mysql bind params out of range! sql=%s", _strSQL.c_str());
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
		Log::e("mysql execute failed: %s, sql=%s", mysql_stmt_error(stmt), strSQL());
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
		Log::w("mysql fetch truncated! sql=%s", strSQL());
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

template<typename T>
DBStatement& DBStatement::getNumberValue(T& value)
{
	if (resultIndex < _numResultFields)
	{
		if (IS_NUM(resultBind[resultIndex].buffer_type))
		{
			if (*resultBind[resultIndex].is_null)
			{
				value = 0;
			}
			else
			{
				value = *(T*)resultBind[resultIndex].buffer;
			}
		}
		++resultIndex;
	}
	else
	{
		Log::e("mysql get result out of range! sql=%s", strSQL());
	}
	return *this;
}

DBStatement& DBStatement::operator>>(int8_t& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(uint8_t& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(int16_t& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(uint16_t& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(int32_t& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(uint32_t& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(int64_t& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(uint64_t& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(float& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(double& value)
{
	return getNumberValue(value);
}

DBStatement& DBStatement::operator>>(std::string& value)
{
	if (resultIndex < _numResultFields)
	{
		if (*resultBind[resultIndex].is_null)
		{
			value = "";
		}
		else
		{
			value = std::string((char*)resultBind[resultIndex].buffer, *resultBind[resultIndex].length);
		}
		++resultIndex;
	}
	else
	{
		Log::e("mysql get result out of range! sql=%s", strSQL());
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
		Log::e("mysql get result out of range! sql=%s", strSQL());
	}
	return *this;
}

void* DBStatement::getBlob(unsigned long& datasize)
{
	datasize = *resultBind[resultIndex].length;
	if (resultIndex < _numResultFields)
	{
		void* data(nullptr);
		if (!(*resultBind[resultIndex].is_null) && datasize > 0)
		{
			data = malloc(datasize);
			memcpy(data, resultBind[resultIndex].buffer, datasize);
		}
		++resultIndex;
		return data;
	}
	else
	{
		Log::e("mysql get result out of range! sql=%s", strSQL());
		return nullptr;
	}
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
			for (auto stmt : *cache.second)
			{
				delete stmt;
			}
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
				record = std::make_unique<Recordset>(pMysqlRes);
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

DBStatement* Database::prepare(const char* strSQL)
{
	auto iter = stmtCache.find(std::string(strSQL));
	DBStatement* dbStmt(nullptr);
	if (iter != stmtCache.end())
	{
		auto pool = iter->second;
		if (pool->size())
		{
			dbStmt = pool->back();
			pool->pop_back();
		}
	}
	if (dbStmt)
	{
		return dbStmt;
	}
	MYSQL_STMT* stmt = mysql_stmt_init(mysql);
	if (0 != mysql_stmt_prepare(stmt, strSQL, (unsigned long)strlen(strSQL)))
	{
		Log::e("mysql prepare error, sql=%s, error=%s", strSQL, mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return NULL;
	}
	return new DBStatement(strSQL, stmt);
}

void Database::release(DBStatement* dbStmt)
{
	if (!dbStmt)
	{
		return;
	}
	std::string sql(dbStmt->strSQL());
	dbStmt->reset();
	auto iter = stmtCache.find(sql);
	StmtPool* pool(nullptr);
	if (iter == stmtCache.end())
	{
		pool = new StmtPool;
		stmtCache.insert(std::make_pair(sql, pool));
	}
	else
	{
		pool = iter->second;
	}
	pool->push_back(dbStmt);
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

void DBQueue::setThread(int numThread)
{
	if (workerThreads.size() < numThread)
	{
		//add more threads
		for (int i = workerThreads.size(); i < numThread; i++)
		{
			workerThreads.push_back(std::make_unique<std::thread>(std::bind(&DBQueue::DBWorkThread, this, (int)workerThreads.size() + 1)));
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

void DBQueue::addQueueMsg(PtrDBRequest request)
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
		PtrDBRequest request = tmpQueue.front();
		tmpQueue.pop_front();
		request->onFinish();
	}
}

PtrDBRequest DBQueue::getRequest()
{
	PtrDBRequest request(nullptr);
	std::lock_guard<std::mutex> lock(workMtx);
	if (!workQueue.empty())
	{
		request = workQueue.front();
		workQueue.pop_front();
		workQueueLength = workQueue.size();
	}
	return request;
}

void DBQueue::finishRequest(PtrDBRequest request)
{
	std::lock_guard<std::mutex> lock(finishMtx);
	finishQueue.push_back(request);
}

void DBQueue::DBWorkThread(int id)
{
	Database db(id);
	db.setDBConfig(config);
	PtrDBRequest request;
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
