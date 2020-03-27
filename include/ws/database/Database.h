#ifndef __WS_DATABASE_H__
#define __WS_DATABASE_H__

#include <mysql.h>
#include <string>
#include <list>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <thread>
#include <memory>
#include "ws/core/ByteArray.h"

using namespace ws::core;

namespace ws
{
	namespace database
	{
		class Database;

		struct MYSQL_CONFIG
		{
			MYSQL_CONFIG() :nPort(3306), autoCommit(true) {}
			unsigned int nPort;
			bool		autoCommit;
			std::string strHost;
			std::string strUser;
			std::string strPassword;
			std::string strDB;
			std::string strUnixSock;
		};

		class DBStatement
		{
			friend class Database;
		public:
			DBStatement(const std::string& sql, MYSQL_STMT* mysql_stmt);
			~DBStatement();

			template<typename T>
			DBStatement& bindNumberParam(T& value, enum_field_types type, bool isUnsigned);
			DBStatement& operator<<(int8_t& value);
			DBStatement& operator<<(uint8_t& value);
			DBStatement& operator<<(int16_t& value);
			DBStatement& operator<<(uint16_t& value);
			DBStatement& operator<<(int32_t& value);
			DBStatement& operator<<(uint32_t& value);
			DBStatement& operator<<(int64_t& value);
			DBStatement& operator<<(uint64_t& value);
			DBStatement& operator<<(float& value);
			DBStatement& operator<<(double& value);
			DBStatement& operator<<(const std::string& value);
			DBStatement& operator<<(ByteArray& value);
			DBStatement& bindString(const char* value, unsigned long* length);
			void bindBlob(enum_field_types type, void* data, unsigned long* size);

			template<typename T>
			DBStatement& getNumberValue(T& value);
			DBStatement& operator>>(int8_t& value);
			DBStatement& operator>>(uint8_t& value);
			DBStatement& operator>>(int16_t& value);
			DBStatement& operator>>(uint16_t& value);
			DBStatement& operator>>(int32_t& value);
			DBStatement& operator>>(uint32_t& value);
			DBStatement& operator>>(int64_t& value);
			DBStatement& operator>>(uint64_t& value);
			DBStatement& operator>>(float& value);
			DBStatement& operator>>(double& value);
			DBStatement& operator>>(std::string& value);
			DBStatement& operator>>(ByteArray& value);
			void* getBlob(unsigned long& datasize);

			//执行准备的语句，执行时必须保证绑定的参数生命周期有效性！
			bool execute();
			bool nextRow();
			void reset();

			inline void skipFields(int num){ resultIndex += num; }
			inline int numParams(){ return _numParams; }
			inline int numResultFields(){ return _numResultFields; }
			inline my_ulonglong numRows(){ return _numRows; }
			inline my_ulonglong lastInsertId(){ return _lastInsertId; }
			inline const std::string& sql() const { return _sql; }

		private:
			int				paramIndex;
			int				_numParams;
			int				resultIndex;
			int				_numResultFields;
			my_ulonglong	_numRows;
			my_ulonglong	_lastInsertId;
			std::string		_sql;
			MYSQL_STMT*		stmt;
			MYSQL_BIND*		paramBind;
			MYSQL_BIND*		resultBind;
			MYSQL_RES*		resultMetadata;
			void**			paramsBuffer;
		};

		class Recordset
		{
		public:
			Recordset(MYSQL_RES* pMysqlRes);
			virtual ~Recordset();

			bool nextRow();
			Recordset& operator >> (int8_t& value);
			Recordset& operator >> (uint8_t& value);
			Recordset& operator >> (int16_t& value);
			Recordset& operator >> (uint16_t& value);
			Recordset& operator >> (int32_t& value);
			Recordset& operator >> (uint32_t& value);
			Recordset& operator >> (int64_t& value);
			Recordset& operator >> (uint64_t& value);
			Recordset& operator >> (float& value);
			Recordset& operator >> (double& value);
			Recordset& operator >> (std::string& value);
			Recordset& operator >> (ByteArray& value);
			void*				getBlob(unsigned long& datasize);
			void				skipFields(int num);

		protected:
			unsigned int		fieldIndex;
			unsigned int		numFields;
			MYSQL_ROW			mysqlRow;
			MYSQL_RES*			mysqlRes;

			template<typename T>
			Recordset&			getInt(T& value);
		};
		typedef std::unique_ptr<Recordset> RecordsetPtr;

		class DBRequest
		{
		public:
			virtual ~DBRequest(){};
			virtual void onRequest(Database& db) = 0;
			virtual void onFinish() = 0;
		};
		typedef std::shared_ptr<DBRequest> DBRequestPtr;

		class Database
		{
		public:
			Database() :mysql(nullptr), id(0), _hasError(false), numAffectedRows(0), numResultRows(0){}
			Database(int _id) :mysql(nullptr), id(_id), _hasError(false), numAffectedRows(0), numResultRows(0){}
			virtual ~Database();

			/************************************************************************/
			/* init db connection                                                   */
			/************************************************************************/
			void setDBConfig(const MYSQL_CONFIG& config);
			bool logon();
			void logoff();
			void changeDatabase(const char* db);

			inline bool isConnected()
			{
				if (mysql == NULL) return false;
				return (mysql_ping(mysql) == 0);
			}
			/************************************************************************/
			/* return num affected rows of query                                    */
			/************************************************************************/
			inline my_ulonglong				getAffectedRows() { return numAffectedRows; };
			/************************************************************************/
			/* return num result of rows                                            */
			/************************************************************************/
			inline my_ulonglong				getResultRows() { return numResultRows; };
			/************************************************************************/
			/* return last insert id of query                                       */
			/************************************************************************/
			inline my_ulonglong				getInsertId()
			{
				if (mysql == NULL) return 0;
				return mysql_insert_id(mysql);
			}
			inline bool						hasError(){ return _hasError; }

			/************************************************************************/
			/* prepare a statement for query                                        */
			/************************************************************************/
			DBStatement*					prepare(const std::string& sql);
			// dealloc a statement
			void							freeStatement(const std::string& sql);
			// execute sql query
			RecordsetPtr					query(const char* strSQL, int nCommit = 1);

		protected:
			static std::mutex								initMtx;
			MYSQL_CONFIG									dbConfig;
			MYSQL*											mysql;
			my_ulonglong									numAffectedRows;
			my_ulonglong									numResultRows;
			bool											_hasError;
			std::unordered_map<std::string, DBStatement*>	stmtCache;
			int												id;
		};

		//数据库队列
		class DBQueue
		{
		public:
			DBQueue(const MYSQL_CONFIG& cfg) : config(cfg), isExit(false), workQueueLength(0) {}
			virtual ~DBQueue();

			//设置数据库并发线程数
			void				setThread(int numThread);
			//添加一个数据库请求到队列
			void				addRequest(DBRequestPtr request);
			//业务线程通过update完成数据库请求，获取结果
			void				update();
			//获取队列长度
			inline size_t		getQueueLength(){ return workQueueLength; }

		private:
			typedef std::list<DBRequestPtr> DBRequestList;
			DBRequestPtr		getRequest();
			void				finishRequest(DBRequestPtr request);
			void				DBWorkThread(int id);

			MYSQL_CONFIG				config;
			std::mutex					workMtx;
			DBRequestList				workQueue;
			size_t						workQueueLength;
			std::mutex					finishMtx;
			DBRequestList				finishQueue;
			bool						isExit;
			std::vector<std::unique_ptr<std::thread>>	workerThreads;
		};
	}
}
#endif
