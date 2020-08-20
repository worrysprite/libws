#ifndef __WS_UTILS_MATH_H__
#define __WS_UTILS_MATH_H__

#include <stdint.h>
#include <random>
#include <math.h>
#include <algorithm>

#ifdef _WIN32
#pragma warning(disable:26451)
#endif

namespace ws
{
	namespace core
	{

		class Math
		{
		public:
			static const double PI;
			static const double HALF_PI;
			static const double RAD_PER_ANGLE;

			/************************************************************************/
			/* 产生一个在[0,1)区间的随机浮点数                                        */
			/************************************************************************/
			inline static double						random(){ return randomGenerator() / (0xFFFFFFFF + 1.0); }
			/************************************************************************/
			/* 产生一个在[0,range)区间的随机整数                                    */
			/************************************************************************/
			inline static uint32_t						random(uint32_t range){ return (uint32_t)(random() * range); }

			/************************************************************************/
			/* 产生一个在[start,end]区间的随机整数                                    */
			/************************************************************************/
			inline static int32_t						random(int32_t start, int32_t end)
			{
				if (start <= end)
					return start + (int32_t)random(end - start + 1);
				else
					return end + (int32_t)random(start - end + 1);
			}
			/************************************************************************/
			/* 判断一个概率是否触发，默认按万分比计算                                  */
			/************************************************************************/
			inline static bool							isTrigger(uint32_t chance, uint32_t totalChance = 10000){ return chance > random(totalChance); }

		private:
			//std::mt19937::tempering_d
			static std::mt19937							randomGenerator;
		};

		template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
		class Vector2D
		{
		public:
			Vector2D(T x_ = 0, T y_ = 0) :x(x_), y(y_){}
			Vector2D(const Vector2D&) = default;
			T				x;
			T				y;

			//清零
			inline void zero() { x = 0; y = 0; }

			//获取长度
			inline double getLength() const { return sqrt(x * x + y * y); }
			//设置长度
			inline Vector2D& setLength(double value)
			{
				double angle(getAngle());
				x = cos(angle) * value;
				y = sin(angle) * value;
				return *this;
			}

			//获取角度
			inline double getAngle() const { return atan2(y, x); }
			//设置角度
			inline Vector2D& setAngle(double value)
			{
				double length(getLength());
				x = cos(value) * length;
				y = sin(value) * length;
				return *this;
			}

			//标准化
			Vector2D& normalize()
			{
				double length(getLength());
				if (length == 0.0)
				{
					x = 1;
				}
				else
				{
					x /= length;
					y /= length;
				}
				return *this;
			}

			//截断
			inline Vector2D& truncate(double value)
			{
				return setLength(std::min<double>(value, getLength()));
			}

			//反向
			inline Vector2D& reverse()
			{
				x = -x;
				y = -y;
				return *this;
			}

			inline Vector2D operator+(const Vector2D& other) const { return Vector2D(x + other.x, y + other.y); }
			inline Vector2D operator-(const Vector2D& other) const { return Vector2D(x - other.x, y - other.y); }
			inline Vector2D operator*(double value) const { return Vector2D(x * value, y * value); }
			inline Vector2D operator/(double value) const { return Vector2D(x / value, y / value); }

			inline bool operator==(const Vector2D& other) const { return x == other.x && y == other.y; }
			inline bool operator!=(const Vector2D& other) const { return x != other.x && y != other.y; }

			Vector2D& operator+=(const Vector2D& other)
			{
				x += other.x;
				y += other.y;
				return *this;
			}
			Vector2D& operator-=(const Vector2D& other)
			{
				x -= other.x;
				y -= other.y;
				return *this;
			}

			//点乘
			double dotProduct(const Vector2D& other) const { return x * other.x + y * other.y; }

			//叉乘
			double crossProduct(const Vector2D& other) const { return x * other.y - y * other.x; }

			//获取与另一个向量的角度关系，平行返回0，正旋转角度<180返回1，正旋转角度>180返回-1
			int sign(const Vector2D& other) const
			{
				double cp = crossProduct(other);
				if (cp == 0)
				{
					return 0;
				}
				return cp < 0 ? -1 : 1;
			}
			//获取垂直向量
			Vector2D getPerpendicular() const { return Vector2D(-y, x); }

			//获取与另一个向量（点）的距离
			double distance(const Vector2D& other) const
			{
				T dx = other.x - x;
				T dy = other.y - y;
				return sqrt(dx * dx + dy * dy);
			}

			static double distance(T x1, T y1, T x2, T y2)
			{
				T dx = x2 - x1;
				T dy = y2 - y1;
				return sqrt(dx * dx + dy * dy);
			}

			//获取两向量的夹角
			static double angleBetween(Vector2D v1, Vector2D v2)
			{
				v1.normalize();
				v2.normalize();
				return acos(v1.dotProduct(v2));
			}
		};

		template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
		class Rectangle
		{
		public:
			Rectangle(T x = 0, T y = 0, T width = 0, T height = 0) :
				x(x), y(y), width(width), height(height) {}

			//获取矩形范围内的一个随机点
			Vector2D<T> randomPoint() const
			{
				if constexpr (std::is_integral<T>::value)
				{
					return Vector2D<T>(
						x + static_cast<T>(Math::random(0, width)),
						y + static_cast<T>(Math::random(0, height))
					);
				}
				else if constexpr (std::is_floating_point<T>::value)
				{
					return Vector2D<T>(
						x + static_cast<T>(Math::random() * width),
						y + static_cast<T>(Math::random() * height)
					);
				}
			}

		public:
			T		x;
			T		y;
			T		width;
			T		height;
		};
	}
}
#ifdef _WIN32
#pragma warning(default: 26451)
#endif

#endif
