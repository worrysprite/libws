#include <openssl/md5.h>
#include <ctype.h>
#include <sstream>
#include <vector>
#include <format>
#include <chrono>
#include <iomanip>
#include "ws/core/String.h"
#include "ws/core/Math.h"
#ifdef _WIN32
#include <combaseapi.h>
#elif defined(__linux__)
#include <uuid/uuid.h>
#endif

using namespace std::chrono;

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

	void toLowercase(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), (int (*)(int))tolower);
	}

	std::string toLowercase(std::string_view src)
	{
		std::string output(src);
		std::transform(src.begin(), src.end(), output.begin(), (int (*)(int))tolower);
		return output;
	}

	void toUppercase(char* str)
	{
		while (*str != '\0')
		{
			*str = toupper(*str);
			++str;
		}
	}

	void toUppercase(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), (int (*)(int))toupper);
	}

	std::string toUppercase(std::string_view src)
	{
		std::string output(src);
		std::transform(src.begin(), src.end(), output.begin(), (int (*)(int))toupper);
		return output;
	}

	bool isPrintableString(std::string_view str)
	{
		for (auto c : str)
		{
			if (!isprint(c))
			{
				return false;
			}
		}
		return true;
	}

	time_t formatTime(const std::string& time, const char* format)
	{
		if (time.empty())
			return 0;

#ifdef _WIN32
		std::stringstream ss(time);
		local_seconds tp;
		ss >> std::chrono::parse(format, tp);
		if (ss.fail())
			return 0;

		return current_zone()->to_sys(tp).time_since_epoch().count();
#elif defined(__linux__) || defined(__unix__)
		tm datetime{};
		std::stringstream ss(time);
		ss >> std::get_time(&datetime, format);
		if (ss.fail())
			return 0;

		return mktime(&datetime);
#endif
	}

	time_t formatTime(const std::string& time)
	{
		if (time.empty())
			return 0;

		constexpr const char* supportedFormats[] = { "%Y-%m-%d %T", "%Y/%m/%d %T" };
		constexpr int numFormats = sizeof(supportedFormats) / sizeof(supportedFormats[0]);

		for (int i = 0; i < numFormats; ++i)
		{
			time_t t = formatTime(time, supportedFormats[i]);
			if (t != 0)
				return t;
		}
		return 0;
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
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
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

	std::string generateUUID()
	{
#ifdef _WIN32
		GUID guid;
		if (S_OK == CoCreateGuid(&guid))
		{
			return std::format(
				"{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
				guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
				guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		}
		return std::string();
#elif defined(__linux__)
		char   buf[64] = { 0 };
		uuid_t uuid;
		uuid_generate(uuid);
		uuid_unparse(uuid, buf);
		return std::string(buf);
#endif
	}
}
