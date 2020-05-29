#ifndef __WS_UTILS_H__
#define __WS_UTILS_H__

#include <cstring>
#include <type_traits>
#include <memory>

#define GET_FIELD_SIZE(from, to) ((uintptr_t)&to-(uintptr_t)&from+sizeof(to))
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
#endif
}

#endif
