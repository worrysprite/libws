#include "ws/core/AStar.h"
#include <memory>
#include <cstring>
#include <spdlog/spdlog.h>
#ifdef _DEBUG_ASTAR
#include <iostream>
#include <thread>
#endif

namespace ws
{
	namespace core
	{
		constexpr int NUM_NODE_SIZE = 1024;

		AStar::~AStar()
		{
			for (auto nodes : mapNodes)
			{
				delete[] nodes;
			}
		}

		bool AStar::findPath(const AbstractMap& map, int startX, int startY, int endX, int endY)
		{
			//清空上次路径
			lastPath.clear();

			if (startX == endX && startY == endY)
				return true;
			
			resizeMap(map.getWidth(), map.getHeight());
			for (auto nodes : mapNodes)
			{
				memset(static_cast<void*>(nodes), 0, sizeof(PathNode) * NUM_NODE_SIZE);
			}

			if (!heuristic)	//未指定估值函数，默认使用哈曼顿估值函数
			{
				heuristic = AStar::manhattanDistance;
			}
			if (!getCost)
			{
				getCost = AStar::commonCost;
			}

			auto start = getNode(startX, startY, map.getWidth());
			auto end = getNode(endX, endY, map.getWidth());
			if (!start || !end)
			{
				spdlog::error("%s invalid arguments!", __FUNCTION__);
				return false;		//参数错误无法寻路
			}
#ifdef _DEBUG_ASTAR
			initDebug(map, end);
#endif
			count = 0;
			openList.push_back(start);

			PathNode* current = nullptr;
			bool isChanged = false;
			while (count++ < maxCount && !openList.empty())
			{
				current = openList.front();
				openList.pop_front();

#ifdef _DEBUG_ASTAR
				debug(current, 'o');
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
				if (current == end)	//找到终点
				{
					while (current->parent)
					{
						lastPath.push_front(current);
						current = current->parent;
					}
					break;
				}
				current->isClosed = true;
#ifdef _DEBUG_ASTAR
				debug(current, 'x');
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
#endif
				//遍历周围的格子
				for (int x = current->x - 1; x <= current->x + 1; ++x)
				{
					if (x < 0 || static_cast<uint32_t>(x) >= map.getWidth()) continue;	//超出地图范围
					for (int y = current->y - 1; y <= current->y + 1; ++y)
					{
						if (y < 0 || static_cast<uint32_t>(y) >= map.getHeight()) continue;	//超出地图范围

						if (x == current->x && y == current->y)
							continue;	//当前点

						if (map.isBlock(x, y))
							continue;	//阻挡点

						auto node = getNode(x, y, map.getWidth());
						if (node->isClosed)
							continue;	//已经检查过

						//计算估值
						double g = current->g + getCost(current->x, current->y, x, y);
						
						isChanged = false;
						if (node->g)
						{
							if (g < node->g)
							{
								node->g = g;
								node->f = g + node->h;
								node->parent = current;

								openList.remove(node);
								isChanged = true;
							}
						}
						else
						{
							node->h = heuristic(x, y, end->x, end->y);
							node->g = g;
							node->f = g + node->h;
							node->parent = current;
							isChanged = true;
						}
						if (isChanged)
						{
							openList.insert(findPlace(openList, node->f), node);
						}
					}
				}
			}
			openList.clear();
			return !lastPath.empty();
		}

		void AStar::resizeMap(int w, int h)
		{
			int requireSize = (int)ceil((double)w * h / NUM_NODE_SIZE);
			int oldSize = (int)mapNodes.size();
			if (oldSize < requireSize)
			{
				mapNodes.resize(requireSize);
				for (int i = oldSize; i < requireSize; ++i)
				{
					mapNodes[i] = new PathNode[NUM_NODE_SIZE];
				}
			}
		}

		PathNode* AStar::getNode(int x, int y, int width)
		{
			int index = y * width + x;
			if (index < 0)
				return nullptr;

			int row = index / NUM_NODE_SIZE;
			int col = index - row * NUM_NODE_SIZE;
			if (static_cast<size_t>(row) < mapNodes.size())
			{
				PathNode* node = &mapNodes[row][col];
				node->x = x;
				node->y = y;
				return node;
			}
			return nullptr;
		}

		std::list<PathNode*>::const_iterator AStar::findPlace(const std::list<PathNode*>& list, double f)
		{
			auto iter = list.begin();
			while (iter != list.end())
			{
				if ((*iter)->f > f)
				{
					break;
				}
				++iter;
			}
			return iter;
		}

#ifdef _DEBUG_ASTAR

		void AStar::initDebug(const AbstractMap& map, PathNode* end)
		{
			for (auto line : debugGraph)
			{
				delete[] line;
			}
			debugGraph.clear();
			debugGraph.resize(map.getHeight());
			for (int i = 0; i < map.getHeight(); ++i)
			{
				char* line = new char[map.getWidth() + 1];
				memset(line, 0, map.getWidth() + 1);
				debugGraph[i] = line;
			}
			//draw blocks
			for (int y = 0; y < map.getHeight(); ++y)
			{
				for (int x = 0; x < map.getWidth(); ++x)
				{
					if (map.isBlock(x, y))
					{
						debugGraph[y][x] = '#';
					}
					else
					{
						debugGraph[y][x] = ' ';
					}
				}
			}
			debugGraph[end->y][end->x] = '$';
		}

		void AStar::debug(PathNode* node, const char symbol)
		{
			debugGraph[node->y][node->x] = symbol;
			system("cls");
			for (int y = 0; y < debugGraph.size(); ++y)
			{
				std::cout << debugGraph[y] << std::endl;
			}
		}

#endif
	}
}
