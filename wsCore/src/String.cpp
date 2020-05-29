#include <openssl/md5.h>
#include <ctype.h>
#include <sstream>
#include <vector>
#include "ws/core/String.h"
#include "ws/core/Math.h"

namespace ws::core::String
{
	bool copy(char* dest, size_t size, const std::string& src, bool asString)
	{
		size_t srcLen = src.length();
		if (!srcLen)
			return true;

		if (!size)
			return false;

		if (asString)
		{
			size = size - 1;
		}
		if (size >= srcLen)	//enough
		{
			memcpy(dest, src.c_str(), srcLen);
			if (asString)
			{
				dest[srcLen] = '\0';
			}
			return true;
		}
		else
		{
			memcpy(dest, src.c_str(), size);
			if (asString)
			{
				dest[size] = '\0';
			}
			return false;
		}
	}

	void toLowercase(char* str)
	{
		while (*str != '\0')
		{
			*str = tolower(*str);
			++str;
		}
	}

	void toUppercase(char* str)
	{
		while (*str != '\0')
		{
			*str = toupper(*str);
			++str;
		}
	}

	bool isPrintableString(const char* str)
	{
		while (*str != '\0')
		{
			if (!isprint(*str))
			{
				return false;
			}
			++str;
		}
		return true;
	}

	time_t formatTime(const char* time)
	{
		std::vector<std::string> tmp, dates, times;
		split(time, " ", tmp);

		if (tmp[0].find('/') != std::string::npos)
		{
			split(tmp[0].c_str(), "/", dates);
		}
		else if (tmp[0].find('-') != std::string::npos)
		{
			split(tmp[0].c_str(), "-", dates);
		}
		if (dates.size() != 3)
		{
			return 0;	//invalid format
		}
		if (tmp.size() > 1)
		{
			split(tmp[1].c_str(), ":", times);
			if (times.size() != 3)
			{
				return 0;	//invalid format
			}
		}
		tm result = { 0 };
		result.tm_year = atoi(dates[0].c_str()) - 1900;
		result.tm_mon = atoi(dates[1].c_str()) - 1;
		result.tm_mday = atoi(dates[2].c_str());
		if (!times.empty())
		{
			result.tm_hour = atoi(times[0].c_str());
			result.tm_min = atoi(times[1].c_str());
			result.tm_sec = atoi(times[2].c_str());
		}
		return mktime(&result);
	}

	std::string formatTime(time_t time, const char* format)
	{
		tm date = { 0 };
#ifdef _WIN32
		localtime_s(&date, &time);
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || \
defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
		localtime_r(&time, &date);
#endif
		char buff[30] = { 0 };
		strftime(buff, sizeof(buff), format, &date);
		return std::string(buff);
	}

	std::string md5(const void* input, size_t length)
	{
		if (!length)
		{
			return std::string();
		}
		unsigned char digest[MD5_DIGEST_LENGTH] = { 0 };
		MD5(static_cast<const unsigned char*>(input), length, digest);
		return bin2Hex(static_cast<const uint8_t*>(digest), MD5_DIGEST_LENGTH);
	}

	static const char HEX_DIGIT[] = "0123456789ABCDEF";
	
	std::string bin2Hex(const uint8_t* input, size_t length)
	{
		std::string result;
		if (!length)
		{
			return result;
		}

		result.reserve(length * 2);
		for (size_t i = 0; i < length; ++i)
		{
			result += HEX_DIGIT[input[i] >> 4];
			result += HEX_DIGIT[input[i] & 0x0F];
		}
		return result;
	}

	std::string URLEncode(const char* input, size_t length)
	{
		std::string result;
		if (!length)
			return result;

		result.reserve(length * 3);
		for (size_t i = 0; i < length; ++i)
		{
			auto c = input[i];
			if (isalnum(c) || c == '-' || c == '_' || c =='.' || c == '~')
			{
				result += c;
			}
			else
			{
				result += '%';
				result += HEX_DIGIT[static_cast<uint8_t>(c) >> 4];
				result += HEX_DIGIT[c & 0x0F];
			}
		}
		return result;
	}

	char hex2char(const char& input)
	{
		if (isdigit(input))
			return input - '0';
		if (iswxdigit(input))
			return isupper(input) ? input - 'A' + 10 : input - 'a' + 10;
		return 0;
	}

	std::string URLDecode(const char* input, size_t length)
	{
		std::string result;
		for (size_t i = 0; i < length; i++)
		{
			if (input[i] == '%')
			{
				result += (hex2char(input[i + 1]) << 4) | hex2char(input[i + 2]);
				i += 2;
			}
			else
			{
				result += input[i];
			}
		}
		return result;
	}

	std::string random(uint16_t length, const char* chars)
	{
		std::string result;
		if (!length)
			return result;

		static const char rndChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
		if (!chars)
		{
			chars = rndChars;
		}
		result.resize(length);
		uint32_t maxLen = (uint32_t)strlen(chars);
		for (uint16_t i = 0; i < length; ++i)
		{
			result[i] = chars[Math::random(maxLen)];
		}
		return result;
	}
}
