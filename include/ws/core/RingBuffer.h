#pragma once
#include <string>
#include <string.h>
#include <type_traits>
#include <cstdint>

namespace ws
{
	namespace core
	{
		class RingBuffer
		{
		public:
			RingBuffer();
			//复制构造
			RingBuffer(const RingBuffer& other) : _capacity(other._capacity),
				_readPos(other._readPos), _writePos(other._writePos)
			{
				_data = (uint8_t*)malloc(_capacity);
				if (!_data)
					throw std::bad_alloc();
				memcpy(_data, other._data, _capacity);
			}
			
			//移动构造
			constexpr RingBuffer(RingBuffer&& rvalue) noexcept : _data(rvalue._data),
				_capacity(rvalue._capacity), _readPos(rvalue._readPos), _writePos(rvalue._writePos)
			{
				rvalue._data = nullptr;
				rvalue._capacity = rvalue._readPos = rvalue._writePos = 0;
			}
			//赋值
			RingBuffer& operator=(const RingBuffer& other)
			{
				if (this == &other)
					return *this;
				RingBuffer tmp(other);
				swap(tmp);
				return *this;
			}
			//移动赋值
			RingBuffer& operator=(RingBuffer&& rvalue) noexcept
			{
				if (this == &rvalue)
					return *this;
				free(_data);
				_data = rvalue._data;
				_readPos = rvalue._readPos;
				_writePos = rvalue._writePos;
				_capacity = rvalue._capacity;
				rvalue._data = nullptr;
				rvalue._capacity = 0;
				rvalue._readPos = rvalue._writePos = 0;
				return *this;
			}

			virtual ~RingBuffer() { free(_data); }

			//已用字节数
			inline size_t used() const
			{
				if (_readPos <= _writePos)
					return _writePos - _readPos;
				return _capacity - _readPos + _writePos;
			}
			//可用字节数
			inline size_t available() const
			{
				return _capacity - used() - 1;
			}
			
			//清空数据，resize是否恢复原始大小
			inline void truncate(bool resize = false)
			{
				if (resize)
				{
					free(_data);
					_capacity = 4096;
					_data = (uint8_t*)malloc(_capacity);
				}
				_readPos = _writePos = 0;
			}
			
			//与另一个RingBuffer交换数据
			void swap(RingBuffer& other) noexcept
			{
				std::swap(_data, other._data);
				std::swap(_readPos, other._readPos);
				std::swap(_writePos, other._writePos);
				std::swap(_capacity, other._capacity);
			}

			/**
			 * @brief 将数据读到一个内存块中
			 * @param outData 要读到的内存块
			 * @param length 要读取的内容大小
			 * @return 实际读取的大小
			*/
			size_t readData(void* outData, size_t length = 0) const;

			template <typename T>
			typename std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, T>
				readNumber() const
			{
				T value(0);
				if (used() < sizeof(T))
					return value;
				readData(&value, sizeof(T));
				return value;
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

			//读取length长度的内容作为字符串，若length=0则读完所有剩余内容
			std::string readString(size_t length) const
			{
				std::string value(length, '\0');
				value.resize(readData(value.data(), length));
				return value;
			}
			std::string readString() const
			{
				if (used() < sizeof(uint16_t))
					return {};
				const size_t savedPos = _readPos;
				uint16_t len = readUInt16();
				// readString(size_t)/readData 把 length==0 当作“读光剩余”，空串前缀不可走那条路径
				if (len == 0)
					return {};
				if (used() < len)
				{
					_readPos = savedPos;
					return {};
				}
				return readString(len);
			}

			template<class T>
			const RingBuffer& operator>>(T& val) const
			{
				if (used() < sizeof(T))
					return *this;
				readData(&val, sizeof(T));
				return *this;
			}

			//以2字节长度做前缀读取字符串；不足时不推进游标
			const RingBuffer& operator>>(std::string& val) const
			{
				if (used() < sizeof(uint16_t))
					return *this;
				const size_t savedPos = _readPos;
				uint16_t len = readUInt16();
				if (len == 0)
				{
					val.clear();
					return *this;
				}
				if (used() < len)
				{
					_readPos = savedPos;
					return *this;
				}
				val = readString(len);
				return *this;
			}

			//将容量扩大到大于size指定的大小，内容保持不变
			void expand(size_t size);

			//写入一段原始buffer
			void writeData(const void* inData, size_t length);

			template<class T>
			RingBuffer& operator<<(const T& val)
			{
				writeData(&val, sizeof(T));
				return *this;
			}

			//以2字节长度做前缀写入字符串
			RingBuffer& operator<<(const std::string& val)
			{
				auto length = (uint16_t)val.size();
				writeData(&length, sizeof(length));
				writeData(val.data(), val.size());
				return *this;
			}

		private:
			uint8_t*		_data;			//数据内存块
			//内存块大小
			size_t			_capacity;
			//当前读位置
			mutable size_t	_readPos;
			//当前写位置
			size_t			_writePos;
		};
	}
}
