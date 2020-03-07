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

			typedef std::function<void(Args...)> Callback;

			void add(const Callback* callback)
			{
				listeners.insert(callback);
			}

			void remove(const Callback* callback)
			{
				listeners.erase(callback);
			}
			
			void notify(Args... data)
			{
				//必须要copy一份临时列表再回调
				//因为回调过程中listeners可能会被修改
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