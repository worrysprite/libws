#include "ws/core/RingBuffer.h"

namespace ws
{
	namespace core
	{
		RingBuffer::RingBuffer() : _capacity(4096), _readPos(0), _writePos(0)
		{
			_data = (uint8_t*)malloc(_capacity);
			if (!_data)
				throw std::bad_alloc();
			memset(_data, 0, _capacity);
		}

		size_t RingBuffer::readData(void* outData, size_t length /*= 0*/) const
		{
			if (!outData)
				return 0;

			auto max = used();
			if (length == 0 || length > max)
			{
				length = max;
			}
			if (length > 0)
			{
				size_t tmp = std::min(length, _capacity - _readPos);
				memcpy(outData, _data + _readPos, tmp);
				if (length > tmp)
					memcpy((uint8_t*)outData + tmp, _data, length - tmp);

				_readPos = (_readPos + length) % _capacity;
			}
			return length;
		}

		void RingBuffer::expand(size_t size)
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

				_data = (uint8_t*)newblock;
				if (_readPos > _writePos)
				{
					auto tmp = _capacity - _readPos;
					memmove(_data + newCap - tmp, _data + _readPos, tmp);
					_readPos = newCap - tmp;
				}
				_capacity = newCap;
			}
		}

		void RingBuffer::writeData(const void* inData, size_t length)
		{
			if (!inData || !length)
				return;

			auto ava = available();
			if (length > ava)
			{
				expand(length - ava + _capacity);
			}

			size_t tmp = std::min(length, _capacity - _writePos);
			memcpy(_data + _writePos, inData, tmp);
			if (length > tmp)
				memcpy(_data, (const uint8_t*)inData + tmp, length - tmp);

			_writePos = (_writePos + length) % _capacity;
		}
	}
}

