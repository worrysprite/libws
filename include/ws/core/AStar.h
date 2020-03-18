#ifndef __WS_CORE_ASTAR_H__
#define __WS_CORE_ASTAR_H__

#include <list>
#include <set>
#include <functional>
#include "Math.h"

namespace ws
{
	namespace core
	{
		//抽象地图类，游戏地图实现这个类的接口，就可以使用AStar寻路
		class AbstractMap
		{
		public:
			//地图宽
			virtual int getWidth() const = 0;
			//地图高
			virtual int getHeight() const = 0;
			//点x,y是否阻挡
			virtual bool isBlock(int x, int y) const = 0;
		};

		class PathNode
		{
		public:
			PathNode(int x = 0, int y = 0) : isClosed(false),
				parent(nullptr), x(x), y(y), g(0), f(0), h(0) {}

			int x;
			int y;
			double h;
			double g;
			double f;
			bool isClosed;

			PathNode* parent;
		};

		class AStar
		{
		public:
			AStar(int max = 512) :count(0), maxCount(max) {}
			~AStar();
			//寻路，返回是否成功
			bool findPath(const AbstractMap& map, int startX, int startY, int endX, int endY);
			//获取上次寻路的结果
			const std::list<const PathNode*>& getLastPath() const { return lastPath; }

			//计算估值函数，参数起点x,y 终点x,y 返回估值
			std::function<double(int, int, int, int)>			heuristic;
			//计算代价函数，参数当前x,y 目标x,y 返回代价
			std::function<double(int, int, int, int)>			getCost;

		private:
			int					count;		//搜索次数
			int					maxCount;	//搜索次数上限

			std::list<PathNode*>		openList;
			std::list<const PathNode*>	lastPath;

			std::vector<PathNode*>	mapNodes;

			//计算地图尺寸，不够就要扩容
			void resizeMap(int w, int h);
			//获取x,y对应的节点
			PathNode* getNode(int x, int y, int width);

			std::list<PathNode*>::const_iterator findPlace(const std::list<PathNode*>& list, double f);

			//哈曼顿估值函数
			static double manhattanDistance(int x, int y, int endX, int endY)
			{
				return (double)abs(x - endX) + abs(y - endY);
			}

			//普通计算代价函数
			static double commonCost(int x1, int y1, int x2, int y2)
			{
				if (x1 != x2 && y1 != y2)
				{
					return 1.4;	//斜线方向代价1.4
				}
				return 1.0;	//直线方向代价1.0
			}

		private:
#ifdef _DEBUG_ASTAR
			std::vector<char*>		debugGraph;
			void initDebug(const AbstractMap& map, PathNode* end);
			void debug(PathNode* node, const char symbol);
#endif // _DEBUG_ASTAR
		};
	}
}



#endif