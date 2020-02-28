#ifndef __WS_UTILS_OBJECT_POOL_H__
#define __WS_UTILS_OBJECT_POOL_H__

#include <list>
#include <mutex>
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
			
			~ObjectPool<T>()
			{
				for (T* element : list)
				{
					delete element;
				}
			}

			T* alloc()
			{
				T* pT = nullptr;
				if (list.size() > 0)
				{
					pT = list.back();
					list.pop_back();
				}
				else
				{
					pT = new T;
				}
				return pT;
			}

			void free(T* pT)
			{
				if (list.size() < capacity)
				{
					list.push_back(pT);
				}
				else
				{
					Log::w("object pool is full");
				}
			}

			typename std::list<T*>::iterator begin()
			{
				return list.begin();
			}

			typename std::list<T*>::iterator end()
			{
				return list.end();
			}

			size_t			size() { return list.size(); }

		private:
			std::list<T*>	list;
			size_t			capacity;
		};
	}
}
#endif