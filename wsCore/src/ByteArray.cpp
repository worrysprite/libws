#include "ws/core/ByteArray.h"
#include <assert.h>

namespace ws
{
	namespace core
	{
		ByteArray::ByteArray(size_t length) :_capacity(length), _readOnly(false),
			_readPos(0), _writePos(0), isAttached(false)
		{
			if (!length)
			{
				length = BYTES_DEFAULT_SIZE;
			}
			_data = malloc(length);
			if (_data)
			{
				memset(_data, 0, length);
			}
		}

		ByteArray::ByteArray(const ByteArray& ba) :_capacity(ba._capacity),
			_readPos(ba._readPos), _writePos(ba._writePos), _readOnly(false),
			isAttached(false)
		{
			_data = malloc(_capacity);
			if (_data)
			{
				memcpy(_data, ba._data, _capacity);
			}
		}

		ByteArray::ByteArray(const void* bytes, size_t length, bool copy /*= false*/) :
			_capacity(length), _readPos(0), _writePos(length),
			isAttached(!copy), _readOnly(!copy)
		{
			if (copy)
			{
				_data = malloc(length);
				if (_data)
				{
					memcpy(_data, bytes, length);
				}
			}
			else
			{
				_data = const_cast<void*>(bytes);
			}
		}

		ByteArray::ByteArray(ByteArray&& rvalue) noexcept
		{
			_data = rvalue._data;
			_capacity = rvalue._capacity;
			_readPos = rvalue._readPos;
			_writePos = rvalue._writePos;
			isAttached = rvalue.isAttached;
			_readOnly = rvalue._readOnly;

			rvalue._data = malloc(BYTES_DEFAULT_SIZE);
			rvalue._capacity = BYTES_DEFAULT_SIZE;
			rvalue._readPos = rvalue._writePos = 0;
			rvalue._readOnly = false;
			rvalue.isAttached = false;
		}

		size_t ByteArray::readBytes(ByteArray& outBytes, size_t length /*= 0*/) const
		{
			if (length == 0 || length > readAvailable())	//读取数据量限制
			{
				length = readAvailable();
			}
			if (length > 0)
			{
				outBytes.expand(length, outBytes._writePos);
				memcpy(outBytes.writerPointer(), readerPointer(), length);
				outBytes._writePos += length;
				_readPos += length;
			}
			return length;
		}

		size_t ByteArray::readData(void* outData, size_t length) const
		{
			if (outData == NULL)
			{
				return 0;
			}
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
			if (length == 0 || length > readAvailable())
			{
				length = readAvailable();
			}
			if (length > 0)
			{
				std::string str((const char*)readerPointer(), length);
				_readPos += length;
				return std::move(str);
			}
			return std::string();
		}

		const ByteArray& ByteArray::operator>>(std::string& val) const
		{
			uint16_t len = readUnsignedShort();
			if (len && readAvailable() >= len)
			{
				val.assign((char*)readerPointer(), len);
				_readPos += len;
			}
			return *this;
		}

		void ByteArray::expand(size_t size, size_t pos)
		{
			size_t newCap(_capacity);
			while (pos + size > newCap)
			{
				newCap = _capacity * 2;
			}
			if (newCap != _capacity)
			{
				void* newblock = realloc(_data, newCap);
				if (newblock)
				{
					_data = newblock;
					_capacity = newCap;
				}
			}
		}

		void ByteArray::resize(size_t size)
		{
			if (size > _capacity)
			{
				expand(size - _capacity, _writePos);
			}
			_writePos = size;
			if (_readPos > size)
			{
				_readPos = size;
			}
		}

		void ByteArray::writeBytes(const ByteArray& inBytes, size_t offset /*= 0*/, size_t length /*= 0*/)
		{
			if (readOnly() || offset >= inBytes._writePos)
				return;

			if (length == 0 || offset + length > inBytes._writePos)
			{
				length = inBytes._writePos - offset;
			}
			if (length > 0)
			{
				expand(length, _writePos);
				void* pSrc = (void*)((intptr_t)inBytes._data + offset);
				memcpy(writerPointer(), pSrc, length);
				_writePos += length;
			}
		}

		void ByteArray::writeData(const void* inData, size_t length)
		{
			if (readOnly() || !inData || !length)
				return;

			expand(length, _writePos);
			memcpy(writerPointer(), inData, length);
			_writePos += length;
		}

		void ByteArray::writeEmptyData(size_t length)
		{
			if (readOnly() || !length)
				return;

			expand(length, _writePos);
			memset(writerPointer(), 0, length);
			_writePos += length;
		}

		ByteArray& ByteArray::operator<<(const std::string& val)
		{
			uint16_t len = (uint16_t)val.size();
			writeUnsignedShort(len);
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
				}
			}
			if (_data)
			{
				memset(_data, 0, _capacity);
			}
			_readPos = 0;
			_writePos = 0;
		}

		void ByteArray::cutHead(size_t length, char* out /*= nullptr*/)
		{
			if (!_writePos)
				return;

			if (length >= _writePos)
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
				memmove(_data, (void*)((intptr_t)_data + length), _writePos);
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
			if (!_writePos)
				return;

			if (length >= _writePos)
			{
				out.writeBytes(*this, 0, _writePos);
				truncate();
			}
			else
			{
				out.writeBytes(*this, 0, length);
				_writePos -= length;
				memmove(_data, (void*)((intptr_t)_data + length), _writePos);
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

			if (length >= _writePos)
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
					memcpy(out, (void*)((intptr_t)_data + _writePos), length);
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
				out.writeBytes(*this, 0, _writePos);
				truncate();
			}
			else
			{
				_writePos -= length;
				out.writeBytes(*this, _writePos, length);
				if (_readPos > _writePos)
				{
					_readPos = _writePos;
				}
			}
		}

		void ByteArray::toHexString(char* dest, size_t length, bool toUpperCase /*= false*/) const
		{
			unsigned char* bytes = (unsigned char*)_data;
			const char* format(nullptr);
			if (toUpperCase)
			{
				format = "%02X";
			}
			else
			{
				format = "%02x";
			}
			for (size_t i = 0; i < _writePos; ++i)
			{
				sprintf(dest, format, bytes[i]);
				dest += 2;
			}
		}

		void ByteArray::attach(void* bytes, size_t length, bool copy /*= false*/)
		{
			if (!isAttached)
			{
				free(_data);	//先释放之前管理的内存
			}
			if (copy)
			{
				_data = malloc(length);
				if (_data)
				{
					memcpy(_data, bytes, length);
				}
				isAttached = false;
			}
			else
			{
				_data = bytes;
				isAttached = true;
			}
			_capacity = length;
			_readPos = 0;
			_writePos = length;
		}

		void ByteArray::swap(ByteArray& other)
		{
			auto tmp = _data;
			_data = other._data;
			other._data = tmp;

			auto pos = _readPos;
			_readPos = other._readPos;
			other._readPos = pos;

			pos = _writePos;
			_writePos = other._writePos;
			other._writePos = pos;

			pos = _capacity;
			_capacity = other._capacity;
			other._capacity = pos;

			auto tmp2 = isAttached;
			isAttached = other.isAttached;
			other.isAttached = tmp2;

			tmp2 = _readOnly;
			_readOnly = other._readOnly;
			other._readOnly = tmp2;
		}
	}
}