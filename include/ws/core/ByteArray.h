#ifndef __WS_CORE_BYTEARRAY_H__
#define __WS_CORE_BYTEARRAY_H__

#include <string>
#include <stdexcept>
#include <string.h>

namespace ws
{
	namespace core
	{
		constexpr auto BYTES_DEFAULT_SIZE = 4096;

		class ByteArray
		{
		public:
			//构造一个字节数组
			ByteArray(size_t length = BYTES_DEFAULT_SIZE);
			//复制构造
			ByteArray(const ByteArray& other);
			//用一块内存构造，复制或只读
			ByteArray(const void* bytes, size_t length, bool copy = false);
			//移动构造
			ByteArray(ByteArray&& rvalue) noexcept;

			virtual ~ByteArray()
			{
				if (!isAttached)
					free(_data);
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
				_writePos = value > _capacity ? _capacity : value;
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

			//清空数据，若指定resetSize则重置capacity到resetSize
			void truncate(size_t resetSize = 0);

			//剪切头部length字节的数据，拷贝到out
			void cutHead(size_t length, char* out = nullptr);
			void cutHead(size_t length, ByteArray& out);

			//剪切尾部length字节的数据，拷贝到out
			void cutTail(size_t length, char* out = nullptr);
			void cutTail(size_t length, ByteArray& out);

			//以为十六进制字符输出
			void toHexString(char* dest, size_t length, bool upperCase = false) const;

			//获取管理的内存块
			inline void* data() { return _data; }
			inline const void* data() const { return _data; }

			//获取管理的内存块 + 偏移量
			inline void* data(int offset) { return (uint8_t*)_data + offset; }
			inline const void* data(int offset) const { return (uint8_t*)_data + offset; }

			//获取当前读位置的指针
			inline const void* readerPointer() const { return (void*)((intptr_t)_data + _readPos); }
			//获取当前写位置的指针
			inline void* writerPointer() { return (void*)((intptr_t)_data + _writePos); }
			
			//附加到一块内存
			void attach(void* bytes, size_t length, bool copy = false);

			//与另一个ByteArray交换数据
			void swap(ByteArray& other) noexcept;

			int8_t readInt8() const { return readNumber<int8_t>(); }
			uint8_t readUInt8() const { return readNumber<uint8_t>(); }
			int16_t readInt16() const { return readNumber<int16_t>(); }
			uint16_t readUInt16() const { return readNumber<uint16_t>(); }
			int32_t readInt32() const { return readNumber<int32_t>(); }
			uint32_t readUInt32() const { return readNumber<uint32_t>(); }
			int64_t readInt64() const { return readNumber<int64_t>(); }
			uint64_t readUInt64() const { return readNumber<uint64_t>(); }
			float readFloat() const { return readNumber<float>(); }
			double readDouble() const { return readNumber<double>(); }

			/************************************************************************/
			/* 将数据读到另一个ByteArray中                                           */
			/* outBytes	要读入的目标ByteArray，从outBytes末尾写入                     */
			/* length	要读取的内容大小                                             */
			/************************************************************************/
			size_t readBytes(ByteArray& outBytes, size_t length) const;
			/************************************************************************/
			/* 将剩余数据全部读到另一个ByteArray中                                    */
			/* outBytes	要读入的目标ByteArray，从outBytes末尾写入                     */
			/************************************************************************/
			size_t readBytes(ByteArray& outBytes) const { return readBytes(outBytes, readAvailable()); }

			/************************************************************************/
			/* 将数据读到一个内存块中                                                 */
			/* outBuff	要读到的内存块                                               */
			/* length	要读取的内容大小                                             */
			/* return	实际读取的大小                                               */
			/************************************************************************/
			size_t readData(void* outData, size_t length = 0) const;

			//读取length长度的内容作为字符串，若length=0则读完所有剩余内容
			std::string readString(size_t length) const;

			//先读取一个uint16_t作为字符串长度，再读取该长度的字符串
			std::string readString() const
			{
				return readString(readUInt16());
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
			typename std::enable_if<std::is_arithmetic<T>::value, T>::type
			readNumber() const
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

			const ByteArray& operator>>(int8_t& val) const { return readType(val); }
			const ByteArray& operator>>(uint8_t& val) const { return readType(val); }
			const ByteArray& operator>>(int16_t& val) const { return readType(val); }
			const ByteArray& operator>>(uint16_t& val) const { return readType(val); }
			const ByteArray& operator>>(int32_t& val) const { return readType(val); }
			const ByteArray& operator>>(uint32_t& val) const { return readType(val); }
			const ByteArray& operator>>(int64_t& val) const { return readType(val); }
			const ByteArray& operator>>(uint64_t& val) const { return readType(val); }
			const ByteArray& operator>>(float& val) const { return readType(val); }
			const ByteArray& operator>>(double& val) const { return readType(val); }
			//以2字节长度做前缀读取字符串
			const ByteArray& operator>>(std::string& val) const;
			template<class T>
			const ByteArray& operator>>(T& val) const { return readType(val); }

			//将capacity扩容到大于size尺寸，不影响内容（可能重新分配内存！）
			void expand(size_t size);

			//写入一段原始buffer
			void writeData(const void* inData, size_t length);
			//把另一个ByteArray的数据拷过来（从writePosition开始写入）
			void writeBytes(const ByteArray& other)
			{
				writeData(other.data(), other.size());
			}
			//写入length长度的空数据(\0)
			void writeEmptyData(size_t length);

			template<typename T>
			ByteArray& writeType(const T& val)
			{
				if (readOnly())
					throw std::logic_error("read only memory blocks!!");

				constexpr auto typeSize = sizeof(T);
				expand(_writePos + typeSize);
				memcpy((void*)(writerPointer()), &val, typeSize);
				_writePos += typeSize;
				return *this;
			}

			//输入操作符
			ByteArray& operator<<(const int8_t& val) { return writeType(val); }
			ByteArray& operator<<(const uint8_t& val) { return writeType(val); }
			ByteArray& operator<<(const int16_t& val) { return writeType(val); }
			ByteArray& operator<<(const uint16_t& val) { return writeType(val); }
			ByteArray& operator<<(const int32_t& val) { return writeType(val); }
			ByteArray& operator<<(const uint32_t& val) { return writeType(val); }
			ByteArray& operator<<(const int64_t& val) { return writeType(val); }
			ByteArray& operator<<(const uint64_t& val) { return writeType(val); }
			ByteArray& operator<<(const float& val) { return writeType(val); }
			ByteArray& operator<<(const double& val) { return writeType(val); }
			//以2字节长度做前缀写入字符串
			ByteArray& operator<<(const std::string& val);

			template<class T>
			ByteArray& operator<<(const T& val)
			{
				if (readOnly())
					throw std::logic_error("read only memory blocks!!");

				constexpr auto typeSize = sizeof(T);
				expand(_writePos + typeSize);
				memcpy((void*)(writerPointer()), &val, typeSize);
				_writePos += typeSize;

				return *this;
			}

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
		};
	}
}

#endif	//__BYTEARRAY_H__
