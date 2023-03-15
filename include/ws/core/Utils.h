#pragma once
#include <cstring>
#include <type_traits>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <charconv>
#include <set>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "ByteArray.h"

//获取从from字段（包含）到to字段（包含）的大小
#define GET_FIELD_SIZE(from, to) ((uintptr_t)&to-(uintptr_t)&from+sizeof(to))
//从from字段到to字段0值初始化
#define ZERO_INIT(from, to) memset(&from, 0, GET_FIELD_SIZE(from, to))

/**
 * 以下模板用于开启强类型枚举的位操作符
 * @see http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
 */
template<typename Enum>
struct EnableBitMaskOperators {};

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator &(Enum lhs, Enum rhs)
{
	return static_cast<Enum> (
		static_cast<typename std::underlying_type<Enum>::type>(lhs) &
		static_cast<typename std::underlying_type<Enum>::type>(rhs)
		);
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator |(Enum lhs, Enum rhs)
{
	return static_cast<Enum> (
		static_cast<typename std::underlying_type<Enum>::type>(lhs) |
		static_cast<typename std::underlying_type<Enum>::type>(rhs)
		);
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator ^(Enum lhs, Enum rhs)
{
	return static_cast<Enum> (
		static_cast<typename std::underlying_type<Enum>::type>(lhs) ^
		static_cast<typename std::underlying_type<Enum>::type>(rhs)
		);
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator ~(Enum rhs)
{
	return static_cast<Enum>(
		~static_cast<typename std::underlying_type<Enum>::type>(rhs)
		);
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type&
operator &=(Enum& lhs, Enum rhs)
{
	lhs = static_cast<Enum> (
		static_cast<typename std::underlying_type<Enum>::type>(lhs) &
		static_cast<typename std::underlying_type<Enum>::type>(rhs)
		);
	return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type&
operator |=(Enum& lhs, Enum rhs)
{
	lhs = static_cast<Enum> (
		static_cast<typename std::underlying_type<Enum>::type>(lhs) |
		static_cast<typename std::underlying_type<Enum>::type>(rhs)
		);
	return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type&
operator ^=(Enum& lhs, Enum rhs)
{
	lhs = static_cast<Enum> (
		static_cast<typename std::underlying_type<Enum>::type>(lhs) ^
		static_cast<typename std::underlying_type<Enum>::type>(rhs)
		);
	return lhs;
}

#define ENABLE_BITMASK_OPERATORS(x)	\
template<>							\
struct EnableBitMaskOperators<x>	\
{static constexpr bool enable = true;}

namespace ws::core
{
	/**
		 * 验证枚举值的合法性，需要枚举类型提供MIN和MAX边界，有效区间[MIN, MAX)
		 */
	template<typename Enum>
	typename std::enable_if<std::is_enum<Enum>::value, bool>::type
		validateEnum(Enum rhs)
	{
		using underlying = typename std::underlying_type<Enum>::type;
		return static_cast<underlying>(rhs) >= static_cast<underlying>(Enum::MIN) &&
			static_cast<underlying>(rhs) < static_cast<underlying>(Enum::MAX);
	}

	/**
	 * 验证枚举值的合法性，需要枚举类型提供MIN和MAX边界，有效区间[MIN, MAX)
	 */
	template<typename Enum, typename IntType>
	typename std::enable_if<std::is_enum<Enum>::value&& std::is_integral<IntType>::value, bool>::type
		validateEnum(IntType rhs)
	{
		using underlying = typename std::underlying_type<Enum>::type;
		return static_cast<underlying>(rhs) >= static_cast<underlying>(Enum::MIN) &&
			static_cast<underlying>(rhs) < static_cast<underlying>(Enum::MAX);
	}

	/**
	 * 用于检查类型是否是std::shared_ptr
	 */
	template<typename T>
	struct is_shared_ptr : std::false_type {};

	template<template<typename E> class Tmpl, typename E>
	struct is_shared_ptr<Tmpl<E>>
	{
		using element_type = E;
		static constexpr bool value = std::is_same<Tmpl<E>, std::shared_ptr<E>>::value;
	};

	/**
	 * 用于检查类型是否是std::unique_ptr
	 * 只能用于检测使用default_delete的unique_ptr
	 */
	template<typename T>
	struct is_unique_ptr : std::false_type {};

	template<template<typename E> class Tmpl, typename E>
	struct is_unique_ptr<Tmpl<E>>
	{
		using element_type = E;
		static constexpr bool value = std::is_same<Tmpl<E>, std::unique_ptr<E, std::default_delete<E>>>::value;
	};

	template<template<typename E, typename D> class Tmpl, typename E, typename D>
	struct is_unique_ptr<Tmpl<E, D>>
	{
		using element_type = E;
		static constexpr bool value = std::is_same<Tmpl<E, D>, std::unique_ptr<E, D>>::value;
	};

	/**
	 * 用于检查类型是否是std::shared_ptr或std::unique_ptr
	 * 只能用于检测使用default_delete的unique_ptr
	 */
	template<typename T>
	struct is_smart_ptr
	{
		static constexpr bool value = is_shared_ptr<T>::value || is_unique_ptr<T>::value;
	};

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	bool createPidfile(const char* pidfile);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"
	//方便使用1字节和2字节的整数字面值（MSVC支持而linux不支持）
	constexpr int8_t operator "" i8(unsigned long long i) { return (int8_t)i; }
	constexpr uint8_t operator "" ui8(unsigned long long i) { return (uint8_t)i; }
	constexpr int16_t operator "" i16(unsigned long long i) { return (int16_t)i; }
	constexpr uint16_t operator "" ui16(unsigned long long i) { return (uint16_t)i; }
#pragma GCC diagnostic pop

#endif

	// from boost hash_combine
	template<typename T> void hash_combine(size_t& seed, T const& v)
	{
		static std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	//获取value的下一个2的n次幂
	inline uint32_t nextPowOf2(uint32_t value)
	{
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;
		return value + 1;
	}

	//获取十进制数字的位数
	uint32_t getNumDigitsOfDemical(uint32_t value);

	/**
	 * 获取调用堆栈，支持windows和linux
	 * windows 需要链接DbgHelp.lib
	 * linux 需要添加-rdynamic编译选项，需要链接-ldl
	 * @param skip 跳过堆栈帧数，默认跳过本函数
	 */
	std::vector<std::string> callstack(int skip = 1);

	template<typename U>
	struct MemberChecker
	{
		template<typename T, void(T::*)(ByteArray&) const = &T::pack>
		static constexpr bool checkPack(T*) { return true; };
		static constexpr bool checkPack(...) { return false; };
		static constexpr bool hasPack = checkPack(static_cast<U*>(0));

		template<typename T, void(T::*)(const ByteArray&) = &T::unpack>
		static constexpr bool checkUnpack(T*) { return true; };
		static constexpr bool checkUnpack(...) { return false; };
		static constexpr bool hasUnpack = checkUnpack(static_cast<U*>(0));
	};

	template<typename T>
	void packItem(ByteArray& packet, const T& item)
	{
		if constexpr (MemberChecker<T>::hasPack)
		{
			item.pack(packet);
		}
		else
		{
			packet << item;
		}
	}

	template<typename T>
	void unpackItem(const ByteArray& packet, T& item)
	{
		if constexpr (MemberChecker<T>::hasUnpack)
		{
			item.unpack(packet);
		}
		else
		{
			packet >> item;
		}
	}

	//通用打包列表、数组等结构
	//注意map和unordered_map不能用此方法打包！
	template<typename T>
	void packContainer(ByteArray& packet, const T& container)
	{
		using VType = typename T::value_type;
		packet << (uint16_t)container.size();
		for (auto& element : container)
		{
			if constexpr (std::is_pointer<VType>::value || is_smart_ptr<VType>::value)
			{
				//成员是原始指针或智能指针
				packItem(packet, *element);
			}
			else
			{
				packItem(packet, element);
			}
		}
	}

	//vector<bool>特例化
	template<>
	inline void packContainer(ByteArray& packet, const std::vector<bool>& container)
	{
		packet << (uint16_t)container.size();
		for (auto item : container)
		{
			packet << (uint8_t)item;
		}
	}

	//通用打包map, unordered_map
	template<class T>
	void packMap(ByteArray& packet, const T& container)
	{
		using MType = typename T::mapped_type;
		packet << (uint16_t)container.size();
		for (auto& p : container)
		{
			packet << p.first;
			if constexpr (std::is_pointer<MType>::value || is_smart_ptr<MType>::value)
			{
				//mapped是原始指针或智能指针
				packItem(packet, *p.second);
			}
			else
			{
				packItem(packet, p.second);
			}
		}
	}

	template<typename T>
	void unpackElement(const ByteArray& packet, T& value)
	{
		if constexpr (std::is_pointer_v<T>)
		{
			using EType = typename std::remove_pointer_t<T>;
			if (!value) value = new EType();
			unpackItem(packet, *value);
		}
		else if constexpr (is_shared_ptr<T>::value)
		{
			using EType = typename is_shared_ptr<T>::element_type;
			if (!value) value = std::make_shared<EType>();
			unpackItem(packet, *value);
		}
		else if constexpr (is_unique_ptr<T>::value)
		{
			using EType = typename is_unique_ptr<T>::element_type;
			if (!value) value = std::make_unique<EType>();
			unpackItem(packet, *value);
		}
		else
		{
			unpackItem(packet, value);
		}
	}

	//通用解包map, unordered_map
	template<class T>
	void unpackMap(const ByteArray& packet, T& container, bool cleanOld = false)
	{
		using KType = typename T::key_type;
		using MType = typename T::mapped_type;
		if (cleanOld)
		{
			container.clear();
		}
		uint16_t len = packet.readUInt16();
		for (int i = 0; i < len; ++i)
		{
			KType key;
			packet >> key;
			unpackElement<MType>(packet, container[key]);
		}
	}

	//通用解包vector和list
	template<typename VType, template<typename...>typename Container, typename...Args>
	void unpackContainer(const ByteArray& packet, Container<VType, Args...>& container)
	{
		uint16_t len = packet.readUInt16();
		if constexpr (std::is_same_v<std::vector<VType>, std::remove_cvref_t<decltype(container)>>)
		{
			container.reserve(container.size() + len);
		}
		for (int i = 0; i < len; ++i)
		{
			if constexpr (std::is_same_v<std::vector<VType>, std::remove_cvref_t<decltype(container)>> ||
				std::is_same_v<std::list<VType>, std::remove_cvref_t<decltype(container)>>)
			{
				unpackElement<VType>(packet, container.emplace_back());
			}
			else if constexpr (std::is_same_v<std::set<VType>, std::remove_cvref_t<decltype(container)>> ||
				std::is_same_v<std::unordered_set<VType>, std::remove_cvref_t<decltype(container)>>)
			{
				VType entry;
				unpackElement<VType>(packet, entry);
				container.insert(entry);
			}
		}
	}

	//通用解包array特例化
	template<class VType, size_t SZ>
	inline void unpackContainer(const ByteArray& packet, std::array<VType, SZ>& arr)
	{
		uint16_t len = packet.readUInt16();
		if (len > SZ)
		{
			spdlog::error("unpack array data out of range!");
			len = SZ;
		}
		for (int i = 0; i < len; ++i)
		{
			unpackElement<VType>(packet, arr[i]);
		}
	}

	//vector<bool>特例化
	inline void unpackContainer(const ByteArray& packet, std::vector<bool>& vec)
	{
		uint16_t len = packet.readUInt16();
		vec.resize(len);
		for (int i = 0; i < len; ++i)
		{
			vec[i] = packet.readUInt8();
		}
	}

	//字符串转数字
	template<typename T>
	std::enable_if_t<std::is_arithmetic_v<T>, bool>
		str2num(const std::string& str, T& value)
	{
		return std::from_chars(str.data(), str.data() + str.size(), value).ec == std::errc();
	}
}
