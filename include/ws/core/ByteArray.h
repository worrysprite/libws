#pragma once
#include <string>
#include <string.h>
#include <type_traits>
#include <stdexcept>

namespace ws
{
	namespace core
	{
		constexpr auto BYTES_DEFAULT_SIZE = 4096;

		class ByteArray
		{
		public:
			//构造一个字节数组
			explicit ByteArray(size_t length = BYTES_DEFAULT_SIZE);
			//复制构造
			ByteArray(const ByteArray& other);
			//用一块内存构造，复制或只读
			ByteArray(const void* bytes, size_t length, bool copy = false);
			//移动构造
			constexpr ByteArray(ByteArray&& rvalue) noexcept : isAttached(rvalue.isAttached),
				_readOnly(rvalue._readOnly), _data(rvalue._data), _readPos(rvalue._readPos),
				_writePos(rvalue._writePos), _capacity(rvalue._capacity)
			{
				rvalue._data = nullptr;
				rvalue._capacity = 0;
				rvalue._readPos = rvalue._writePos = 0;
				rvalue._readOnly = false;
				rvalue.isAttached = false;
			}
			//赋值
			ByteArray& operator=(const ByteArray& other);
			//移动赋值
			constexpr ByteArray& operator=(ByteArray&& rvalue) noexcept
			{
				if (!isAttached) free(_data);
				isAttached = rvalue.isAttached;
				_readOnly = rvalue._readOnly;
				_data = rvalue._data;
				_readPos = rvalue._readPos;
				_writePos = rvalue._writePos;
				_capacity = rvalue._capacity;
				rvalue._data = nullptr;
				rvalue._capacity = 0;
				rvalue._readPos = rvalue._writePos = 0;
				rvalue._readOnly = false;
				rvalue.isAttached = false;
				return *this;
			}

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
			inline void readPosition(size_t value) const
			{
				_readPos = value > _writePos ? _writePos : value;
			}
			//获取写位置
			inline size_t writePosition() const { return _writePos; }
			//设置写位置，若超过总容量会先扩容
			inline void writePosition(size_t value)
			{
				if (_readOnly) return;
				expand(value);
				_writePos = value;
			}

			//往前或往后移动读位置
			inline void seek(int pos) const
			{
				if (pos > 0)
				{
					readPosition(_readPos + (size_t)pos);
				}
				else if (pos < 0)
				{
					readPosition(_readPos - (size_t)(-pos));
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
			std::string toHexString(bool upperCase = false) const;

			//获取管理的内存块
			inline void* data() { return _data; }
			inline const void* data() const { return _data; }

			//获取管理的内存块 + 偏移量
			inline void* data(size_t offset) { return (uint8_t*)_data + offset; }
			inline const void* data(size_t offset) const { return (uint8_t*)_data + offset; }

			//获取当前读位置的指针
			inline const void* readerPointer() const { return (void*)((intptr_t)_data + _readPos); }
			//获取当前写位置的指针
			inline void* writerPointer() { return (void*)((intptr_t)_data + _writePos); }
			
			//附加到一块内存，只能用于读数据，不负责释放该内存
			void attach(const void* data, size_t length);

			//与另一个ByteArray交换数据
			void swap(ByteArray& other) noexcept;

			template <typename T>
			typename std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, T>
				readNumber() const
			{
				constexpr size_t typeSize = sizeof(T);
				if (typeSize <= readAvailable())
				{
					const T* value = (const T*)readerPointer();
					_readPos += typeSize;
					return *value;
				}
				return T(0);
			}

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

			/**
			 * @brief 将数据读到另一个ByteArray中
			 * @param outBytes 要读入的目标ByteArray，从outBytes末尾写入
			 * @param length 要读取的内容大小
			 * @return 实际读取的大小
			*/
			size_t readBytes(ByteArray& outBytes, size_t length) const;

			/**
			 * @brief 将数据读到一个内存块中
			 * @param outData 要读到的内存块
			 * @param length 要读取的内容大小
			 * @return 实际读取的大小
			*/
			size_t readData(void* outData, size_t length = 0) const;

			//读取length长度的内容作为字符串
			std::string readString(size_t length) const;
			std::string readString() const { return readString(readUInt16()); }

			template<class T>
#ifdef _WIN32
			std::enable_if_t<std::is_trivially_copyable_v<T>, const ByteArray&>
#else
			//gcc和clang对于std::pair<int, int>类型判定为non-trivially-copyable
			//https://stackoverflow.com/questions/58283694/why-is-pair-of-const-trivially-copyable-but-pair-is-not
			const ByteArray&
#endif
			operator>>(T& val) const 
			{
				constexpr auto typeSize = sizeof(T);
				if (typeSize <= readAvailable())
				{
					memcpy(&val, readerPointer(), typeSize);
					_readPos += typeSize;
				}
				return *this;
			}

			//以2字节长度做前缀读取字符串
			const ByteArray& operator>>(std::string& val) const
			{
				uint16_t len = readUInt16();
				if (len && readAvailable() >= len)
				{
					val.assign((char*)readerPointer(), len);
					_readPos += len;
				}
				return *this;
			}

			//把所有剩余可读取内容读入outBytes，从outBytes末尾写入
			const ByteArray& operator>>(ByteArray& outBytes) const
			{
				readBytes(outBytes, readAvailable());
				return *this;
			}

			//将capacity扩容到大于size尺寸，不影响内容（可能重新分配内存！）
			void expand(size_t size);

			//写入一段原始buffer
			void writeData(const void* inData, size_t length);

			//写入length长度的空数据(\0)
			void writeEmptyData(size_t length);

			/**
			 * @brief 在当前写入位置构造一个T类型对象，并返回该对象的指针，只读则返回nullptr
			 * @tparam T 指定对象的类型
			 * @return 新构造对象的指针
			*/
			template<typename T>
			T* emplace()
			{
				T* result = nullptr;
				if (!readOnly())
				{
					constexpr size_t length = sizeof(T);
					expand(_writePos + length);
					result = new (writerPointer())T;
					_writePos += length;
				}
				return result;
			}

			template<class T>
#ifdef _WIN32
			std::enable_if_t<std::is_trivially_copyable_v<T>, ByteArray&>
#else
			//gcc和clang对于std::pair<int, int>类型判定为non-trivially-copyable
			//https://stackoverflow.com/questions/58283694/why-is-pair-of-const-trivially-copyable-but-pair-is-not
			ByteArray&
#endif
			operator<<(const T& val)
			{
				if (readOnly())
					throw std::runtime_error("read only memory blocks!!");

				constexpr auto typeSize = sizeof(T);
				expand(_writePos + typeSize);
				memcpy((void*)(writerPointer()), &val, typeSize);
				_writePos += typeSize;
				return *this;
			}

			//写入另外一个ByteArray的全部数据
			ByteArray& operator<<(const ByteArray& other)
			{
				writeData(other.data(), other.size());
				return *this;
			}

			//以2字节长度做前缀写入字符串
			ByteArray& operator<<(const std::string& val);

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
