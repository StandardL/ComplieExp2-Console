#include "Graph.h"
#include <cassert>

Graph::Graph()
{
	numEdges = 0;
	numVertixes = 0;
	maxVertixes = 100;
	G.resize(maxVertixes + 10);
}

bool Graph::insertVertix()
{
	if (numVertixes == maxVertixes)
		return false;
	//G[numVertixes].insert(Edge(-1, '#'));
	numVertixes++;
	return true;
}

bool Graph::insertEdge(int v1, int v2, char c)
{
	if (v1 > -1 && v1 < maxVertixes && v2 > -2 && v2 < maxVertixes)
	{
		Edge e2(v2, c);
		// 确保两点之间的边值不重复（重复条件：终点相同，字符相同）
		for (auto& e : G[v1])
		{
			if (e.end == e2.end && e.character == e2.character)
				return false;
		}
		// 默认是v1 -> v2
		G[v1].insert(e2);
		numEdges++;
		return true;
	}
	
	return false;
}

int Graph::getFirstNeighbor(int v, char c)
{
	return 0;
}

int Graph::getNextNeighbor(int v, char c, int w)
{
	return 0;
}

int Graph::getValue(int i)
{
	return 0;
}

int Graph::getVertixPos(int Vertix)
{
	return 0;
}

int Graph::NumofVertixes()
{
	return numVertixes;
}

int Graph::NumofEdges()
{
	return numEdges;
}

void Graph::DFS(int v, char c)
{
}

void Graph::Reset()
{
	numVertixes = 0;
	numEdges = 0;
	G.clear();
	G.resize(maxVertixes + 10);
}

void Graph::DFS(int v, char c, bool visited[])
{
}
