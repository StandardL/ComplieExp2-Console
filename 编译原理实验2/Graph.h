#pragma once
#include <set>
#include <vector>
// 使用邻接表表示无权有向图

struct Edge
{
	int end;
	char character;
	Edge(int e, char c):end(e), character(c) {}

	bool operator<(const Edge& e2) const
	{
		if (this->end < e2.end)
			return true;
		else
			return false;
	}
	bool operator==(const Edge& e2) const
	{
		if (this->end == e2.end && this->character == e2.character)
			return true;
		else
			return false;
	}
};

class Graph
{
public:
	Graph();
	bool insertVertix();  // 插入点
	bool insertEdge(int v1, int v2, char c);  // 插入边
	int getFirstNeighbor(int v, char c);
	int getNextNeighbor(int v, char c, int w);
	int getValue(int i);
	int getVertixPos(int Vertix);
	int NumofVertixes();
	int NumofEdges();
	void DFS(int v, char c);
	void Reset();

	std::vector<std::multiset<Edge>> G;
private:
	int maxVertixes;
	int numEdges;
	int numVertixes;

	void DFS(int v, char c, bool visited[]);
};

