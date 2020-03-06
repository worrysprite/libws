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
		template<class T>
		class ObjectPool
		{
		public:
			ObjectPool<T>(size_t cap = 0xFFFFFFFF) : capacity(cap) {}

			std::shared_ptr<T> alloc()
			{
				std::shared_ptr<T> obj;
				if (list.size() > 0)
				{
					obj = list.back();
					list.pop_back();
				}
				else
				{
					obj = std::make_shared<T>();
				}
				return obj;
			}

			void free(std::shared_ptr<T> &obj)
			{
				if (list.size() < capacity)
				{
					list.push_back(obj);
				}
				else
				{
					Log::w("object pool is full");
				}
				obj.reset();
			}

			typename std::list<std::shared_ptr<T>>::iterator begin()
			{
				return list.begin();
			}

			typename std::list<std::shared_ptr<T>>::iterator end()
			{
				return list.end();
			}

			size_t			size() { return list.size(); }

		private:
			std::list<std::shared_ptr<T>>	list;
			size_t							capacity;
		};
	}
}
#endif