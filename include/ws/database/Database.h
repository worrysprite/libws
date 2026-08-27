#pragma once
#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <memory>
#include <chrono>
#include <spdlog/spdlog.h>
#include "ws/core/ByteArray.h"
#include "ws/core/TimeTool.h"

namespace ws
{
	namespace database
	{
		struct MySQLConfig
		{
			MySQLConfig() : host("localhost"), user("root"), characterset("utf8mb4") {}
			uint16_t		port = 3306;
			bool			autoCommit = true;
			uint32_t		maxStmtCache = -1;	//语句缓存上限
			std::string		host;
			std::string		user;
			std::string		password;
			std::string		database;
			std::string		unixSock;
			std::string		characterset;
		};

		//查询结果集
		class Recordset
		{
		public:
			Recordset(MYSQL_RES* res, const std::string& sql);
			virtual ~Recordset();

			//将查询结果指针移动到下一行
			bool nextRow();

			//输出为算术数值
			template<typename T> requires std::is_arithmetic_v<T>
			Recordset& operator>>(T& value)
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
						else if constexpr (std::is_same<T, float>::value)
						{
							value = strtof(szRow, nullptr);
						}
						else if constexpr (std::is_same<T, double>::value)
						{
							value = strtod(szRow, nullptr);
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

			//输出为枚举
			template<typename T> requires std::is_enum_v<T>
			Recordset& operator>>(T& value)
			{
				return operator>>((typename std::underlying_type<T>::type&)value);
			}

			//输出为字符串
			Recordset& operator>>(std::string& value);
			//输出为ByteArray，必须是blob类型
			Recordset& operator>>(ws::core::ByteArray& value);

			//获取二进制数据，必须是blob类型，需要外部释放返回的数据
			void* getBlob(unsigned long& datasize);
			//跳过字段
			void skipFields(int num) { fieldIndex += num; }

		private:
			uint32_t			fieldIndex;
			uint32_t			numFields;
			MYSQL_ROW			mysqlRow;
			MYSQL_RES* mysqlRes;
			std::string			sql;
		};
		using RecordsetPtr = std::unique_ptr<Recordset>;

		class Database;
		class DBStatement
		{
			friend class Database;
		public:
			DBStatement(MYSQL_STMT* mysql_stmt, const std::string& sql);
			~DBStatement();

			//绑定整型参数，需要保证value的生命周期在execute()之后！
			template<typename T> requires std::is_integral_v<T>
			DBStatement& operator<<(const T& value)
			{
				if (paramIndex < numParams())
				{
					auto& b = paramBind[paramIndex++];
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
					b.buffer = const_cast<T*>(&value);
					b.buffer_length = sizeof(T);
					b.is_unsigned = std::is_unsigned<T>::value;
				}
				else
				{
					spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
				}
				return *this;
			}
			//绑定整型右值参数
			template<typename T> requires std::is_integral_v<T>
			DBStatement& operator<<(T&& value)
			{
				if (paramIndex < numParams())
				{
					auto& b = paramBind[paramIndex++];
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

					auto& buffer = paramsBuffer.emplace_back((const char*)&value, sizeof(T));
					b.buffer = buffer.data();
					b.buffer_length = sizeof(T);
					b.is_unsigned = std::is_unsigned<T>::value;
				}
				else
				{
					spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

			//绑定浮点数参数，需要保证value的生命周期在execute()之后！
			template<typename T> requires std::is_floating_point_v<T>
			DBStatement& operator<<(const T& value)
			{
				if (paramIndex < numParams())
				{
					auto& b = paramBind[paramIndex++];
					if constexpr (std::is_same<T, float>::value)
					{
						b.buffer_type = MYSQL_TYPE_FLOAT;
					}
					else if constexpr (std::is_same<T, double>::value)
					{
						b.buffer_type = MYSQL_TYPE_DOUBLE;
					}
					b.buffer = const_cast<T*>(&value);
					b.buffer_length = sizeof(T);
					b.is_unsigned = false;
				}
				else
				{
					spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

			//绑定浮点数右值参数
			template<typename T> requires std::is_floating_point_v<T>
			DBStatement& operator<<(T&& value)
			{
				if (paramIndex < numParams())
				{
					auto& b = paramBind[paramIndex++];
					if constexpr (std::is_same<T, float>::value)
					{
						b.buffer_type = MYSQL_TYPE_FLOAT;
					}
					else if constexpr (std::is_same<T, double>::value)
					{
						b.buffer_type = MYSQL_TYPE_DOUBLE;
					}
					auto& buffer = paramsBuffer.emplace_back((const char*)&value, sizeof(T));
					b.buffer = buffer.data();
					b.buffer_length = sizeof(T);
					b.is_unsigned = false;
				}
				else
				{
					spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

			//绑定枚举类型参数，需要保证value的生命周期在execute()之后！
			template<typename T> requires std::is_enum_v<T>
			DBStatement& operator<<(const T& value)
			{
				return operator<<((const std::underlying_type_t<T>&)(value));
			}

			//绑定枚举类型右值参数
			template<typename T> requires std::is_enum_v<T>
			DBStatement& operator<<(T&& value)
			{
				return operator<<(static_cast<std::underlying_type_t<T>>(std::forward<T>(value)));
			}

			//绑定字符串参数，不会复制内容，需要保证value的生命周期在execute()之后！
			DBStatement& operator<<(const std::string& value);
			//绑定字符串参数，会复制字符串内容！
			DBStatement& bindString(const char* value, size_t length);
			//绑定时间参数，DATE类型
			DBStatement& operator<<(std::chrono::year_month_day&& value);
			//绑定时间参数，DATETIME或TIMESTAMP类型
			template<class C, class D = std::chrono::microseconds>
			DBStatement& operator<<(const std::chrono::time_point<C, D>& value)
			{
				using namespace std::chrono;
				if (paramIndex < numParams())
				{
					auto& b = paramBind[paramIndex];
					b.buffer_type = MYSQL_TYPE_DATETIME;
					auto& buffer = paramsBuffer.emplace_back(sizeof(MYSQL_TIME), '\0');
					MYSQL_TIME* mytime = (MYSQL_TIME*)buffer.data();
					b.buffer = buffer.data();
					b.buffer_length = (unsigned long)buffer.size();

					auto date = floor<days>(value);
					year_month_day ymd{ date };
					hh_mm_ss hms{ value - date };

					mytime->time_type = MYSQL_TIMESTAMP_DATETIME;
					mytime->year = static_cast<int>(ymd.year());
					mytime->month = static_cast<unsigned>(ymd.month());
					mytime->day = static_cast<unsigned>(ymd.day());
					mytime->hour = static_cast<unsigned>(hms.hours().count());
					mytime->minute = static_cast<unsigned>(hms.minutes().count());
					mytime->second = static_cast<unsigned>(hms.seconds().count());
					mytime->second_part = static_cast<unsigned>(hms.subseconds().count());
					++paramIndex;
				}
				else
				{
					spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

			//绑定时间参数，TIME类型
			template<typename Duration>
				requires ws::core::TimeTool::is_duration_v<Duration>
			DBStatement& operator<<(Duration&& value)
			{
				using namespace std::chrono;
				if (paramIndex < numParams())
				{
					auto& b = paramBind[paramIndex];
					b.buffer_type = MYSQL_TYPE_TIME;
					auto& buffer = paramsBuffer.emplace_back(sizeof(MYSQL_TIME), '\0');
					MYSQL_TIME* mytime = (MYSQL_TIME*)buffer.data();
					b.buffer = buffer.data();
					b.buffer_length = (unsigned long)buffer.size();

					microseconds::rep usec = duration_cast<microseconds>(value).count();
					mytime->neg = usec < 0;
					if (mytime->neg) usec = -usec;

					constexpr auto USEC_PER_HOUR = 3600'000'000LL;
					constexpr auto USEC_PER_MIN = 60'000'000LL;
					constexpr auto USEC_PER_SEC = 1'000'000LL;

					mytime->time_type = MYSQL_TIMESTAMP_TIME;
					mytime->hour = static_cast<unsigned>(usec / USEC_PER_HOUR);
					usec = usec % USEC_PER_HOUR;
					mytime->minute = static_cast<unsigned>(usec / USEC_PER_MIN);
					usec = usec % USEC_PER_MIN;
					mytime->second = static_cast<unsigned>(usec / USEC_PER_SEC);
					mytime->second_part = static_cast<unsigned>(usec % USEC_PER_SEC);
					++paramIndex;

					if (mytime->hour > 838)
					{
						spdlog::warn("mysql bind time out of range(hours greater than 838)! sql={}", _sql.c_str());
					}
				}
				else
				{
					spdlog::error("mysql bind params out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

			//绑定二进制字节块，不会复制内容，需要保证value的生命周期在execute()之后！
			DBStatement& operator<<(const ws::core::ByteArray& value);
			//绑定二进制数据，会复制内容！
			void bindBlob(void* data, unsigned long size);
			//绑定NULL值
			DBStatement& operator<<(std::nullptr_t);

			//获取算术类型字段值，数据库NULL值会得到0
			template<typename T> requires std::is_arithmetic_v<T>
			DBStatement& operator>>(T& value)
			{
				if (resultIndex < numResultFields())
				{
					auto& b = resultBind[resultIndex];
					if (IS_NUM(b.buffer_type))
					{
						if (*b.is_null)
						{
							value = 0;
						}
						else if (b.buffer_type == MYSQL_TYPE_DECIMAL ||
								b.buffer_type == MYSQL_TYPE_NEWDECIMAL)
						{
							value = static_cast<T>(std::strtod((const char*)b.buffer, nullptr));
						}
						else
						{
							value = *(T*)b.buffer;
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

			//获取枚举类型字段值，数据库NULL值会得到0
			template<typename T> requires std::is_enum_v<T>
			DBStatement& operator>>(T& value)
			{
				return operator>>((typename std::underlying_type<T>::type&)value);
			}

			//获取字符串字段值，数据库NULL值会得到空字符串！
			DBStatement& operator>>(std::string& value);

			//获取日期时间字段值，数据库NULL值不会修改value
			template<class C, class D = std::chrono::microseconds>
			DBStatement& operator>>(std::chrono::time_point<C, D>& value)
			{
				using namespace std::chrono;
				if (resultIndex < numResultFields())
				{
					auto& b = resultBind[resultIndex];
					if (!*b.is_null && (b.buffer_type == MYSQL_TYPE_DATE ||
						b.buffer_type == MYSQL_TYPE_DATETIME ||
						b.buffer_type == MYSQL_TYPE_TIMESTAMP))
					{
						auto mytime = (const MYSQL_TIME*)b.buffer;
						auto ymd = year(mytime->year) / month(mytime->month) / day(mytime->day);
						value = floor<D>(time_point<C, days>{ ymd } + hours{ mytime->hour } + minutes{ mytime->minute } + seconds{ mytime->second } + microseconds{ mytime->second_part });
					}
					++resultIndex;
				}
				else
				{
					spdlog::error("mysql get result out of range! sql={}", _sql.c_str());
				}
				return *this;
			}

			//获取时间字段值，数据库NULL值会得到0
			DBStatement& operator>>(std::chrono::microseconds& value);
			//获取二进制字段值，会清空原ByteArray的数据！
			DBStatement& operator>>(ws::core::ByteArray& value);
			//获取二进制字段值，数据长度会返回到datasize
			void* getBlob(size_t& datasize);

			//执行准备的语句，执行时必须保证绑定的参数生命周期有效性！
			bool execute();
			//将查询结果指针移动到下一行
			bool nextRow();
			//清空所有绑定和查询结果
			void clear();
			//跳过num个字段
			inline void skipFields(int num) { resultIndex += num; }
			//参数个数
			inline uint32_t numParams() { return static_cast<uint32_t>(paramBind.size()); }
			//查询结果的字段数量
			inline uint32_t numResultFields() { return static_cast<uint32_t>(resultBind.size()); }
			//结果行数或影响行数
			inline my_ulonglong numRows() { return _numRows; }
			//最后一次插入id（自增id字段）
			inline my_ulonglong lastInsertId() { return _lastInsertId; }
			//关联的sql语句
			inline const std::string& sql() const { return _sql; }

		private:
			uint32_t		paramIndex = 0;
			uint32_t		resultIndex = 0;
			my_ulonglong	_numRows = 0;
			my_ulonglong	_lastInsertId = 0;
			MYSQL_STMT* stmt;

			std::chrono::steady_clock::time_point	lastUseTime;
			std::string								_sql;
			std::vector<MYSQL_BIND>					paramBind;
			std::vector<MYSQL_BIND>					resultBind;
			std::list<std::string>					paramsBuffer;	//用于储存复制的参数的buffer
		};

		class DBRequest
		{
		public:
			virtual ~DBRequest() {};
			virtual void onRequest(Database& db) = 0;
			virtual void onFinish() = 0;
		};
		using DBRequestPtr = std::shared_ptr<DBRequest>;

		class Database
		{
		public:
			Database() :mysql(nullptr), numAffectedRows(0), numResultRows(0) {}
			virtual ~Database();

			/************************************************************************/
			/* init db connection                                                   */
			/************************************************************************/
			void setDBConfig(const MySQLConfig& config);
			bool logon();	//连接mysql服务端
			void logoff();	//断开mysql服务端

			//更换db
			bool changeDatabase(const std::string& db);

			//检测连接状态
			inline bool isConnected() const
			{
				return mysql && mysql_ping(mysql) == 0;
			}

			//上次查询影响行数
			inline my_ulonglong getAffectedRows() { return numAffectedRows; }

			//上次查询结果行数
			inline my_ulonglong getResultRows() { return numResultRows; }

			//上次插入的自增id
			inline my_ulonglong getInsertId()
			{
				if (mysql == nullptr) return 0;
				return mysql_insert_id(mysql);
			}

			//上次查询结果集
			inline RecordsetPtr getLastRecord() { return std::move(lastRecords); }

			/**
			 * @brief 准备一个DBStatement供查询并缓存，可防注入并提高查询效率
			 * @param sql 要查询的语句，变量用占位符?填充
			 * @return 返回准备好的语句指针，出错返回nullptr
			*/
			DBStatement* prepare(const std::string& sql);

			//已缓存的DBStatement数量
			inline size_t cacheSize() const { return stmtCache.size(); }

			//释放已缓存的DBStatement
			inline void freeStatement(const std::string& sql)
			{
				stmtCache.erase(sql);
			}

			//清空已缓存的DBStatement
			inline void clearCache() { stmtCache.clear(); }

			/**
			 * 执行一个sql语句，返回是否成功
			 * 用getResultRows获取查询结果行数
			 * 用getLastRecord获取查询结果内容
			 * 用getAffectedRows获取影响行数（insert,update,delete等语句）
			 * 若字符串包含多条语句，需要用nextRecordset()获取下个语句的结果
			 */
			bool query(const std::string& strSQL);

			//获取下一个结果集，返回是否有结果集
			bool nextRecordset();

			//提交事务，返回是否成功
			bool commit() { return !mysql_commit(mysql); }
			//回滚事务，返回是否成功
			bool rollback() { return !mysql_rollback(mysql); }

		protected:
			MySQLConfig										dbConfig;
			MYSQL* mysql;
			my_ulonglong									numAffectedRows;
			my_ulonglong									numResultRows;
			RecordsetPtr									lastRecords;
			std::string										lastSQL;
			std::unordered_map<std::string, DBStatement>	stmtCache;
		};

		//数据库队列
		class DBQueue
		{
		public:
			DBQueue() {}
			DBQueue(const MySQLConfig& cfg) : config(cfg) {}
			virtual ~DBQueue();

			//获取数据库连接配置
			const MySQLConfig& getConfig() const { return config; }
			//设置数据库连接配置，修改后不会影响现有连接
			void setConfig(const MySQLConfig& cfg) { config = cfg; }

			//设置数据库并发线程数，小于当前线程数时会使删除的线程完成正在查询的请求后退出
			void setThread(size_t numThread);

			//添加一个数据库请求到队列
			void addRequest(DBRequestPtr request);
			//业务线程通过update完成数据库请求，获取结果
			void update();
			//获取队列长度
			inline size_t getQueueLength()
			{
				std::lock_guard<std::mutex> lock(workMtx);
				return workQueueLength;
			}

		private:
			struct WorkerThread
			{
				bool			isExit = false;
				std::thread		thread;
			};

			using DBRequestList = std::deque<DBRequestPtr>;
			DBRequestPtr				getRequest();
			void						DBWorkThread(const WorkerThread& worker);

			MySQLConfig					config;
			std::mutex					workMtx;
			DBRequestList				workQueue;
			size_t						workQueueLength = 0;
			std::mutex					finishMtx;
			DBRequestList				finishQueue;
			std::list<WorkerThread>		workerThreads;
		};
	}
}
