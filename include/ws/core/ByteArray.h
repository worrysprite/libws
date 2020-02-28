#ifndef __WS_CORE_BYTEARRAY_H__
#define __WS_CORE_BYTEARRAY_H__

#include <string>
#include <mutex>
#include <memory.h>

constexpr auto DEFAULT_SIZE = 4096;
constexpr auto STEP_SIZE = 4096;

namespace ws
{
	namespace core
	{
		class ByteArray
		{
		public:
			ByteArray(size_t length = DEFAULT_SIZE);
			ByteArray(const ByteArray& ba);
			ByteArray(const void* bytes, size_t length, bool copy = false);
			ByteArray(ByteArray&& rvalue);

			virtual ~ByteArray()
			{
				if (!isAttached)
					free(_data);
				delete mtx;
			}

			//数据大小
			inline size_t size() const { return _writePos; }
			//容量大小
			inline size_t capacity() const { return _capacity; }

			//获取读位置
			inline size_t readPosition() const { return _readPos; }
			//设置读位置
			inline void readPosition(size_t value)
			{
				_readPos = value > _writePos ? _writePos : value;
			}
			//获取写位置
			inline size_t writePosition() const { return _writePos; }
			//设置写位置
			inline void writePosition(size_t value)
			{
				if (_readOnly) return;
				_writePos = (isAttached || value > _capacity) ?
					_writePos = _capacity : value;
			}

			//往前或往后移动读位置
			inline void seek(int pos)
			{
				if (pos > 0)
				{
					readPosition(_readPos + (size_t)pos);
				}
				else if (pos < 0)
				{
					pos = -pos;
					readPosition(_readPos - (size_t)pos);
				}
			}

			//可读字节数
			inline size_t readAvailable() const { return _writePos - _readPos; }
			//可写字节数
			inline size_t writeAvailable() const { return _capacity - _writePos; }

			//获取和设置只读
			inline bool readOnly() const { return isAttached || _readOnly; }
			inline void readOnly(bool v) { _readOnly = v; }

			//清空数据，不会释放内存
			void truncate();

			//剪切头部length字节的数据，拷贝到out
			void cutHead(size_t length = 0, char* out = nullptr);
			void cutHead(size_t length, ByteArray& out);

			//剪切尾部length字节的数据，拷贝到out
			void cutTail(size_t length, char* out = nullptr);
			void cutTail(size_t length, ByteArray& out);

			//以为十六进制字符输出
			void toHexString(char* dest, size_t length, bool upperCase = false) const;

			//获取管理的内存块
			inline void* data(){ return _data; }

			//获取当前读位置的指针
			inline const void* readerPointer() const { return (void*)((intptr_t)_data + _readPos); }
			//获取当前写位置的指针
			inline void* writerPointer() { return (void*)((intptr_t)_data + _writePos); }
			
			//附加到一块内存
			void attach(const void* bytes, size_t length, bool copy = false);

			//与另一个ByteArray交换数据
			void swap(ByteArray& other);

			char readByte() const
			{
				return readNumber<char>();
			}

			unsigned char readUnsignedByte() const
			{
				return readNumber<unsigned char>();
			}

			short readShort() const
			{
				return readNumber<short>();
			}

			unsigned short readUnsignedShort() const
			{
				return readNumber<unsigned short>();
			}

			int readInt() const
			{
				return readNumber<int>();
			}

			unsigned int readUnsignedInt() const
			{
				return readNumber<unsigned int>();
			}

			long long readInt64() const
			{
				return readNumber<long long>();
			}

			unsigned long long readUnsignedInt64() const
			{
				return readNumber<unsigned long long>();
			}

			float readFloat() const
			{
				return readNumber<float>();
			}

			double readDouble() const
			{
				return readNumber<double>();
			}
			/************************************************************************/
			/* 将数据读到另一个ByteArray中                                           */
			/* outBytes	要读入的目标ByteArray                                        */
			/* offset	从outBytes的offset位置开始写入                               */
			/* length	要读取的内容大小                                             */
			/************************************************************************/
			size_t readBytes(ByteArray& outBytes, unsigned int offset = 0, size_t length = 0) const;
			/************************************************************************/
			/* 将数据读到一个内存块中                                                 */
			/* outBuff	要读到的内存块                                               */
			/* length	要读取的内容大小                                             */
			/* return	实际读取的大小                                               */
			/************************************************************************/
			size_t readData(void* outData, size_t length = 0) const;

			//读取length长度的内容作为字符串，若length=0则读完所有剩余内容
			std::string&& readString(size_t length) const;

			//先读取一个uint16_t作为字符串长度，再读取该长度的字符串
			std::string&& readString() const
			{
				return readString(readUnsignedShort());
			}

			template <class T>
			const ByteArray& readType(T& val) const
			{
				constexpr auto typeSize = sizeof(T);
				if (typeSize <= readAvailable())
				{
					memcpy(&val, readerPointer(), typeSize);
					_readPos += typeSize;
				}
				return *this;
			}
			template <typename T>
			T readNumber() const
			{
				constexpr size_t typeSize = sizeof(T);
				if (typeSize <= readAvailable())
				{
					const T* value = (const T*)(readerPointer());
					_readPos += typeSize;
					return *value;
				}
				return 0;
			}
			const ByteArray& operator>>(char& val) const { return readType(val); }
			const ByteArray& operator>>(signed char& val) const { return readType(val); }
			const ByteArray& operator>>(unsigned char& val) const { return readType(val); }
			const ByteArray& operator>>(short& val) const { return readType(val); }
			const ByteArray& operator>>(unsigned short& val) const { return readType(val); }
			const ByteArray& operator>>(int& val) const { return readType(val); }
			const ByteArray& operator>>(unsigned int& val) const { return readType(val); }
			const ByteArray& operator>>(long& val) const { return readType(val); }
			const ByteArray& operator>>(unsigned long& val) const { return readType(val); }
			const ByteArray& operator>>(long long& val) const { return readType(val); }
			const ByteArray& operator>>(unsigned long long& val) const { return readType(val); }
			const ByteArray& operator>>(float& val) const { return readType(val); }
			const ByteArray& operator>>(double& val) const { return readType(val); }
			//以2字节长度做前缀读取字符串
			const ByteArray& operator>>(std::string& val) const;
			template<class T>
			const ByteArray& operator>>(T& val) const { return readType(val); }

			//从pos扩容到可写入size大小，不影响内容
			void expand(size_t size, size_t pos);
			//重新设置大小，不影响内容
			void resize(size_t size);

			void writeByte(const char& b)
			{
				writeType(b);
			}
			void writeUnsignedByte(const unsigned char& ub)
			{
				writeType(ub);
			}
			void writeShort(const short& s)
			{
				writeType(s);
			}
			void writeUnsignedShort(const unsigned short& us)
			{
				writeType(us);
			}
			void writeInt(const int& i)
			{
				writeType(i);
			}
			void writeUnsignedInt(const unsigned int& ui)
			{
				writeType(ui);
			}
			void writeInt64(const long long& ll)
			{
				writeType(ll);
			}
			void writeUnsignedInt64(const unsigned long long& ull)
			{
				writeType(ull);
			}
			void writeFloat(const float& f)
			{
				writeType(f);
			}
			void writeDouble(const double& d)
			{
				writeType(d);
			}
			/************************************************************************/
			/* 从另一个ByteArray写入数据                                             */
			/* inBytes	数据源                                                      */
			/* offset	从inBytes的offset位置开始读取                                */
			/* length	要写入的字节数                                               */
			/************************************************************************/
			void writeBytes(const ByteArray& inBytes, size_t offset = 0, size_t length = 0);
			//写入指定数据
			void writeData(const void* inData, size_t length);

			template<typename T>
			ByteArray& writeType(const T& val)
			{
				if (readOnly())
					return *this;

				constexpr auto typeSize = sizeof(T);
				expand(typeSize, _writePos);
				memcpy((void*)(writerPointer()), &val, typeSize);
				_writePos += typeSize;
				return *this;
			}

			//输入操作符
			ByteArray& operator<<(const char& val) { return writeType(val); }
			ByteArray& operator<<(const signed char& val) { return writeType(val); }
			ByteArray& operator<<(const unsigned char& val) { return writeType(val); }
			ByteArray& operator<<(const short& val) { return writeType(val); }
			ByteArray& operator<<(const unsigned short& val) { return writeType(val); }
			ByteArray& operator<<(const int& val) { return writeType(val); }
			ByteArray& operator<<(const unsigned int& val) { return writeType(val); }
			ByteArray& operator<<(const long& val) { return writeType(val); }
			ByteArray& operator<<(const unsigned long& val) { return writeType(val); }
			ByteArray& operator<<(const long long& val) { return writeType(val); }
			ByteArray& operator<<(const unsigned long long& val) { return writeType(val); }
			ByteArray& operator<<(const float& val) { return writeType(val); }
			ByteArray& operator<<(const double& val) { return writeType(val); }
			//以2字节长度做前缀写入字符串
			ByteArray& operator<<(const std::string& val);
			template<class T>
			ByteArray& operator<<(const T& val) { return writeType(val); }

		private:
			bool				isAttached;
			bool				_readOnly;
			void*				_data;			//数据内存块

			//当前读位置
			mutable size_t		_readPos;
			//当前写位置
			size_t				_writePos;
			//内存块大小
			size_t				_capacity;
			std::mutex*			mtx;
		};
	}
}

#endif	//__BYTEARRAY_H__
