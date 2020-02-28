#ifndef __WS_SIGNAL_H__
#include <type_traits>
#include <functional>
#include <unordered_set>

namespace ws
{
	namespace core
	{
		template<class ...Args>
		class Signal
		{
		public:

			typedef std::function<void(const Args&...)> Callback;

			void add(const Callback* callback)
			{
				listeners.insert(callback);
			}

			void remove(const Callback* callback)
			{
				listeners.erase(callback);
			}
			
			void notify(const Args&... data)
			{
				std::unordered_set<const Callback*> tmpList = listeners;
				for (const Callback* cb : tmpList)
				{
					(*cb)(data...);
				}
			}

		protected:
			std::unordered_set<const Callback*>		listeners;
		};


	}
}

#define __WS_SIGNAL_H__
#endif