#include "ws/core/ByteArray.h"

namespace ws
{
	namespace core
	{
		ByteArray::ByteArray(size_t length) :isAttached(false), _readOnly(false), 
			_readPos(0), _writePos(0), _capacity(length)
		{
			if (!length)
			{
				_capacity = length = BYTES_DEFAULT_SIZE;
			}
			_data = malloc(length);
			if (!_data)
				throw std::bad_alloc();
			memset(_data, 0, length);
		}

		ByteArray::ByteArray(const ByteArray& other) :isAttached(other.isAttached), _readOnly(other._readOnly), 
			_readPos(other._readPos), _writePos(other._writePos), _capacity(other._capacity)
		{
			if (isAttached)
			{
				_data = other._data;
			}
			else
			{
				_data = malloc(_capacity);
				if (!_data)
					throw std::bad_alloc();
				memcpy(_data, other._data, _capacity);
			}
		}

		ByteArray::ByteArray(const void* bytes, size_t length, bool copy /*= false*/) :
			isAttached(!copy), _readOnly(!copy), _readPos(0), _writePos(length), _capacity(length)
		{
			if (copy)
			{
				_data = malloc(length);
				if (!_data)
					throw std::bad_alloc();
				memcpy(_data, bytes, length);
			}
			else
			{
				_data = const_cast<void*>(bytes);
			}
		}

		ByteArray& ByteArray::operator=(const ByteArray& other)
		{
			isAttached = other.isAttached;
			_readOnly = other._readOnly;
			_readPos = other._readPos;
			_writePos = other._writePos;
			_capacity = other._capacity;
			if (isAttached)
			{
				_data = other._data;
			}
			else
			{
				_data = malloc(_capacity);
				if (!_data)
					throw std::bad_alloc();
				memcpy(_data, other._data, _capacity);
			}
			return *this;
		}

		size_t ByteArray::readBytes(ByteArray& outBytes, size_t length /*= 0*/) const
		{
			if (length > 0)
			{
				if (length > readAvailable())	//读取数据量限制
				{
					length = readAvailable();
				}
				outBytes.expand(outBytes.size() + length);
				memcpy(outBytes.writerPointer(), readerPointer(), length);
				outBytes._writePos += length;
				_readPos += length;
			}
			return length;
		}

		size_t ByteArray::readData(void* outData, size_t length) const
		{
			if (!outData)
				return 0;

			if (length == 0 || length > readAvailable())
			{
				length = readAvailable();
			}
			if (length > 0)
			{
				memcpy(outData, readerPointer(), length);
				_readPos += length;
			}
			return length;
		}

		std::string ByteArray::readString(size_t length) const
		{
			if (length > readAvailable())
			{
				length = readAvailable();
			}
			if (length > 0)
			{
				auto cstr = (const char*)readerPointer();
				_readPos += length;
				return std::string(cstr, length);
			}
			return std::string();
		}

		void ByteArray::expand(size_t size)
		{
			size_t newCap = _capacity;
			while (size > newCap)
			{
				newCap *= 2;
			}
			if (newCap != _capacity)
			{
				void* newblock = realloc(_data, newCap);
				if (!newblock)
					throw std::bad_alloc();

				_data = newblock;
				_capacity = newCap;
			}
		}

		void ByteArray::writeData(const void* inData, size_t length)
		{
			if (readOnly() || !inData || !length)
				return;

			expand(_writePos + length);
			memcpy(writerPointer(), inData, length);
			_writePos += length;
		}

		void ByteArray::writeEmptyData(size_t length)
		{
			if (readOnly() || !length)
				return;

			expand(_writePos + length);
			memset(writerPointer(), 0, length);
			_writePos += length;
		}

		ByteArray& ByteArray::operator<<(const std::string& val)
		{
			uint16_t len = (uint16_t)val.size();
			*this << len;
			writeData(val.c_str(), len);
			return *this;
		}

		void ByteArray::truncate(size_t resetSize)
		{
			if (resetSize)
			{
				if (!isAttached)
				{
					free(_data);
					_data = malloc(resetSize);
					if (!_data)
						throw std::bad_alloc();
				}
			}
#ifdef _DEBUG
			memset(_data, 0, _capacity);
#endif
			_readPos = 0;
			_writePos = 0;
		}

		void ByteArray::cutHead(size_t length, char* out /*= nullptr*/)
		{
			if (!_writePos || !length)
				return;

			if (length >= _writePos)	//clear
			{
				if (out)
				{
					memcpy(out, _data, _writePos);
				}
				truncate();
			}
			else
			{
				if (out)
				{
					memcpy(out, _data, length);
				}
				_writePos -= length;
				memmove(_data, data(length), _writePos);
				if (_readPos > length)
				{
					_readPos -= length;
				}
				else
				{
					_readPos = 0;
				}
			}
		}

		void ByteArray::cutHead(size_t length, ByteArray& out)
		{
			if (!_writePos || !length)
				return;

			if (length >= _writePos)
			{
				out.writeData(_data, _writePos);
				truncate();
			}
			else
			{
				out.writeData(_data, length);
				_writePos -= length;
				memmove(_data, data(length), _writePos);
				if (_readPos > length)
				{
					_readPos -= length;
				}
				else
				{
					_readPos = 0;
				}
			}
		}

		void ByteArray::cutTail(size_t length, char* out /*= nullptr*/)
		{
			if (!_writePos)
				return;

			if (length >= _writePos)	//clear
			{
				if (out)
				{
					memcpy(out, _data, _writePos);
				}
				truncate();
			}
			else
			{
				_writePos -= length;
				if (out)
				{
					memcpy(out, data(_writePos), length);
				}
				if (_readPos > _writePos)
				{
					_readPos = _writePos;
				}
			}
		}

		void ByteArray::cutTail(size_t length, ByteArray& out /*= nullptr*/)
		{
			if (!_writePos)
				return;

			if (length >= _writePos)
			{
				out.writeData(_data, _writePos);
				truncate();
			}
			else
			{
				_writePos -= length;
				out.writeData(data(_writePos), length);
				if (_readPos > _writePos)
				{
					_readPos = _writePos;
				}
			}
		}

		std::string ByteArray::toHexString(bool upperCase /*= false*/) const
		{
			std::string result(_writePos * 2, '\0');
			const char* format(nullptr);
			if (upperCase)
			{
				format = "%02X";
			}
			else
			{
				format = "%02x";
			}
			auto bytes = (uint8_t*)_data;
			auto dest = result.data();
			for (size_t i = 0; i < _writePos; ++i)
			{
				sprintf(dest, format, bytes[i]);
				dest += 2;
			}
			return result;
		}

		void ByteArray::attach(const void* data, size_t length)
		{
			if (!isAttached)
			{
				free(_data);	//先释放之前管理的内存
			}
			_data = const_cast<void*>(data);
			isAttached = true;
			_readOnly = true;
			_capacity = length;
			_readPos = 0;
			_writePos = length;
		}

		void ByteArray::swap(ByteArray& other) noexcept
		{
			std::swap(_data, other._data);
			std::swap(isAttached, other.isAttached);
			std::swap(_readOnly, other._readOnly);
			std::swap(_readPos, other._readPos);
			std::swap(_writePos, other._writePos);
			std::swap(_capacity, other._capacity);
		}
	}
}
