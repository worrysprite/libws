#ifndef __WS_DATABASE_H__
#define __WS_DATABASE_H__

#include <mysql.h>
#include <string>
#include <queue>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <thread>
#include <memory>
#include <spdlog/spdlog.h>
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

			//绑定整型参数，需要保证value的生命周期在execute()之后！
			template<typename T>
			typename std::enable_if<std::is_integral<T>::value, DBStatement>::type&
				operator<<(T& value)
			{
				if (paramIndex < _numParams)
				{
					MYSQL_BIND& b = paramBind[paramIndex++];
					if constexpr (sizeof(T) == sizeof(int8_t))
					{
						b.buffer_type = MYSQL_TYPE_TINY;
					}
					else if constexpr (sizeof(T) == sizeof(int16_t))
					{
						b.buffer_type = MYSQL_TYPE_SHORT;
					}
					else if constexpr (sizeof(T) == sizeof(int32_t))
					{
						b.buffer_type = MYSQL_TYPE_LONG;
					}
					else if constexpr (sizeof(T) == sizeof(int64_t))
					{
						b.buffer_type = MYSQL_TYPE_LONGLONG;
					}
					b.buffer = &value;
					b.is_unsigned = std::is_unsigned<T>::value;
				}
				else
				{
					spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

			//绑定枚举类型参数，需要保证value的生命周期在execute()之后！
			template<typename T>
			typename std::enable_if<std::is_enum<T>::value, DBStatement>::type&
				operator<<(T& value)
			{
				return operator<<((typename std::underlying_type<T>::type&)value);
			}

			//绑定浮点数参数，需要保证value的生命周期在execute()之后！
			DBStatement& operator<<(float& value) { return bindNumberParam(value, MYSQL_TYPE_FLOAT, false); }
			DBStatement& operator<<(double& value) { return bindNumberParam(value, MYSQL_TYPE_DOUBLE, false); }

			//绑定字符串参数，会复制字符串内容！
			DBStatement& operator<<(const std::string& value);
			//绑定字符串参数，不会复制内容，需要保证value的生命周期在execute()之后！
			DBStatement& bindString(const char* value, unsigned long length);

			//绑定二进制字节块，会复制内容！
			DBStatement& operator<<(const ByteArray& value);
			//绑定二进制数据，不会复制内容，需要保证data的生命周期在execute()之后！
			void bindBlob(enum_field_types type, void* data, unsigned long size);

			//获取算术类型字段值
			template<typename T>
			typename std::enable_if<std::is_arithmetic<T>::value, DBStatement>::type&
				operator>>(T& value)
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
					spdlog::error("mysql get result out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

			//获取枚举类型字段值
			template<typename T>
			typename std::enable_if<std::is_enum<T>::value, DBStatement>::type&
				operator>>(T& value)
			{
				return operator>>((typename std::underlying_type<T>::type&)value);
			}

			//获取字符串字段值
			DBStatement& operator>>(std::string& value);
			//获取二进制字段值，会清空原ByteArray的数据！
			DBStatement& operator>>(ByteArray& value);
			//获取二进制字段值，数据长度会返回到datasize
			void* getBlob(unsigned long& datasize);

			//执行准备的语句，执行时必须保证绑定的参数生命周期有效性！
			bool execute();
			//将查询结果指针移动到下一行
			bool nextRow();
			//清空所有绑定和查询结果
			void reset();
			//跳过num个字段
			inline void skipFields(int num){ resultIndex += num; }
			//参数个数
			inline int numParams(){ return _numParams; }
			//查询结果的字段数量
			inline int numResultFields(){ return _numResultFields; }
			//结果行数或影响行数
			inline my_ulonglong numRows(){ return _numRows; }
			//最后一次插入id（自增id字段）
			inline my_ulonglong lastInsertId(){ return _lastInsertId; }
			//关联的sql语句
			inline const std::string& sql() const { return _sql; }

		private:
			template<typename T>
			typename std::enable_if<std::is_arithmetic<T>::value, DBStatement>::type&
				bindNumberParam(T& value, enum_field_types type, bool isUnsigned)
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
					spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

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
			Recordset(MYSQL_RES* pMysqlRes, const std::string& sql);
			virtual ~Recordset();

			//将查询结果指针移动到下一行
			bool nextRow();

			template<typename T>
			typename std::enable_if<std::is_integral<T>::value, Recordset>::type&
				operator>>(T& value)
			{
				if (mysqlRow && fieldIndex < numFields)
				{
					const char* szRow = mysqlRow[fieldIndex++];
					if (szRow)
					{
						if constexpr (std::is_same<T, int64_t>::value)
						{
							value = atoll(szRow);
						}
						else if constexpr (std::is_same<T, uint64_t>::value)
						{
							value = strtoull(szRow, nullptr, 10);
						}
						else
						{
							value = atoi(szRow);
						}
					}
				}
				else
				{
					spdlog::error("mysql fetch field out of range!, sql: {}", sql.c_str());
				}
				return *this;
			}

			template<typename T>
			typename std::enable_if<std::is_enum<T>::value, Recordset>::type&
				operator>>(T& value)
			{
				return operator>>((typename std::underlying_type<T>::type&)value);
			}

			Recordset& operator>>(float& value);
			Recordset& operator>>(double& value);
			Recordset& operator>>(std::string& value);
			Recordset& operator>>(ByteArray& value);

			void*				getBlob(unsigned long& datasize);
			void				skipFields(int num) { fieldIndex += num; }

		private:
			uint32_t			fieldIndex;
			uint32_t			numFields;
			MYSQL_ROW			mysqlRow;
			MYSQL_RES*			mysqlRes;
			std::string			sql;
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
			Database() :mysql(nullptr), numAffectedRows(0), numResultRows(0), _hasError(false) {}
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
		};

		//数据库队列
		class DBQueue
		{
		public:
			DBQueue(const MYSQL_CONFIG& cfg) : config(cfg) {}
			virtual ~DBQueue();

			//设置数据库并发线程数
			void				setThread(size_t numThread);
			//添加一个数据库请求到队列
			void				addRequest(DBRequestPtr request);
			//业务线程通过update完成数据库请求，获取结果
			void				update();
			//获取队列长度
			inline size_t		getQueueLength()
			{
				std::lock_guard<std::mutex> lock(workMtx);
				return workQueueLength;
			}

		private:
			struct WorkerThread
			{
				bool							isExit = false;
				std::unique_ptr<std::thread>	thread;
			};
			using WorkerThreadPtr = std::shared_ptr<WorkerThread>;

			using DBRequestList = std::queue<DBRequestPtr>;
			DBRequestPtr		getRequest();
			void				DBWorkThread(WorkerThreadPtr worker);

			MYSQL_CONFIG				config;
			std::mutex					workMtx;
			DBRequestList				workQueue;
			size_t						workQueueLength = 0;
			std::mutex					finishMtx;
			DBRequestList				finishQueue;
			std::deque<WorkerThreadPtr>	workerThreads;
		};
	}
}
#endif
