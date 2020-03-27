#ifndef __WS_UTILS_H__
#define __WS_UTILS_H__

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

/**
 * 验证枚举值的合法性，需要枚举类型提供MIN和MAX边界，有效区间[MIN, MAX)
 */
template<typename Enum, typename IntType>
typename std::enable_if<std::is_enum<Enum>::value &&
	(std::is_integral<IntType>::value || std::is_enum<IntType>::value), bool>::type
validateEnum(IntType rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<underlying>(rhs) >= static_cast<underlying>(Enum::MIN) &&
		static_cast<underlying>(rhs) < static_cast<underlying>(Enum::MAX);
}

#endif
