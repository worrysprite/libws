#ifndef __WS_UTILS_STRING_H__
#define __WS_UTILS_STRING_H__

#include <string>
#include <time.h>
#include <string.h>
#include <sstream>

namespace ws::core::String
{
	/**
	 * 按seperator分割str，并将每段原始char*地址结果存入output
	 * 将会修改str（分割符改\0）
	 * 若str里不存在seperator，output包含原始字符串
	 */
	template<template<class ...> class Container, typename ...Args>
	void split(char* str, const char* seperator, Container<char*, Args...>& output)
	{
		size_t strLen = strlen(str);
		if (!strLen)
		{
			return;
		}
		size_t sepLength = strlen(seperator);
		output.push_back(str);
		while (*str != '\0' && strLen >= sepLength)
		{
			if (strncmp(str, seperator, sepLength) == 0)
			{
				*str = '\0';
				str += sepLength;
				strLen -= sepLength;
				output.push_back(str);
			}
			else
			{
				++str;
				--strLen;
			}
		}
	}

	/**
	 * 按seperator分割str，并将每段的副本存入output
	 * 不会修改原始str
	 * 若不包含seperator，output包含原始字符串
	 */
	template<template<class ...> class Container, typename ...Args>
	void split(const char* str, const char* seperator, Container<std::string, Args...>& output)
	{
		size_t strLen = strlen(str);
		if (!strLen)
		{
			return;
		}
		size_t sepLength = strlen(seperator);
		const char* ptr = str;
		while (*str != '\0' && strLen >= sepLength)
		{
			if (strncmp(str, seperator, sepLength) == 0)
			{
				output.push_back(std::string(ptr, str - ptr));
				str += sepLength;
				ptr = str;
				strLen -= sepLength;
			}
			else
			{
				++str;
				--strLen;
			}
		}
		output.push_back(std::string(ptr));
	}

	//拼接数组里的所有字符串，用glue参数拼接
	template<template<class ...> class Container, typename ...Args>
	std::string join(const Container<std::string, Args...>& input, const char* glue = "")
	{
		std::string output;	//RVO
		if (input.empty())
		{
			return output;
		}
		//先计算一次总长度
		size_t totalLength = (input.size() - 1) * strlen(glue);
		for (auto& str : input)
		{
			totalLength += str.size();
		}
		output.reserve(totalLength);
		//开始拼接
		auto iter = input.begin();
		output = *iter++;
		while (iter != input.end())
		{
			output += glue;
			output += *iter++;
		}
		return output;
	}

	template<template<class ...> class Container, typename ...Args>
	std::string join(const Container<Args...>& input, const char* glue = "")
	{
		std::stringstream ss;
		if (!input.empty())
		{
			auto iter = input.begin();
			ss << *iter++;
			while (iter != input.end())
			{
				ss << glue << *iter++;
			}
		}
		return ss.str();
	}

	/**
	 * 安全复制std::string内容到dest
	 * asString为true会在末尾添加\0
	 * 返回是否完整复制
	 */
	bool copy(char* dest, size_t size, const std::string& src, bool asString = true);
	
	//字符串转小写
	void toLowercase(char* str);
	//字符串转大写
	void toUppercase(char* str);

	/************************************************************************/
	/* 检测字符串是否为可打印ASCII码                                          */
	/************************************************************************/
	bool isPrintableString(const char* str);

	/************************************************************************/
	/* 将一个时间字符串格式化为unix timestamp，支持以下格式                    */
	/* yyyy/mm/dd hh:mm:ss                                                  */
	/* yyyy-mm-dd hh:mm:ss                                                  */
	/************************************************************************/
	time_t formatTime(const std::string& time);

	/**
	 * 将一个unix timestamp转化为时间字符串
	 * 指定格式详见strftime
	 * @see https://zh.cppreference.com/w/c/chrono/strftime
	 */
	std::string formatTime(time_t time, const char* format);

	/**
	 * 将当前时间转化为字符串，格式为yyyy-mm-dd hh:mm:ss
	 */
	inline std::string formatTime()
	{
		return formatTime(::time(nullptr), "%F %T");
	}

	/**
	 * 封装openssl计算md5，适合小量的数据
	 * 返回md5结果的十六进制字符串
	 */
	std::string md5(const void* input, size_t length);

	/**
	 * 二进制转十六进制字符串
	 */
	std::string bin2Hex(const uint8_t* input, size_t length);

	//同js的encodeURIComponent
	std::string URLEncode(const char* input, size_t length);
	//同js的decodeURIComponent
	std::string URLDecode(const char* input, size_t length);

	//生成随机字符串，length指定长度，chars指定取值内容，未指定则使用大小写字母和数字
	std::string random(uint16_t length, const char* chars = nullptr);
}

#endif
