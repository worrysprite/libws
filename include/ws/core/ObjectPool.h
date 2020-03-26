#ifndef __WS_UTILS_OBJECT_POOL_H__
#define __WS_UTILS_OBJECT_POOL_H__

#include <list>
#include <mutex>
#include <memory>
#include "Log.h"

namespace ws
{
	namespace core
	{
		template<class T, int capacity = 2000>
		class ObjectPool
		{
		public:
			ObjectPool(int init = 200)
			{
				for (int i = 0; i < init; ++i)
				{
					list.push_back(std::make_unique<T>());
				}
			}

			std::unique_ptr<T> alloc()
			{
				std::unique_ptr<T> obj;
				if (list.size() > 0)
				{
					obj = std::move(list.back());
					list.pop_back();
				}
				else
				{
					obj = std::make_unique<T>();
				}
				return obj;
			}

			void free(std::unique_ptr<T> &obj)
			{
				if (list.size() < capacity)
				{
					list.push_back(std::move(obj));
				}
				else
				{
					Log::w("object pool is full");
				}
			}

			void free(std::unique_ptr<T>&& obj)
			{
				if (list.size() < capacity)
				{
					list.push_back(std::move(obj));
				}
				else
				{
					Log::w("object pool is full");
				}
			}

			size_t			size() { return list.size(); }

		private:
			std::list<std::unique_ptr<T>>	list;
		};
	}
}
#endif