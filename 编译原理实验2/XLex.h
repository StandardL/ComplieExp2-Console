#pragma once
#include <string>
#include <map>
#include <stack>
#include <queue>
#include <iostream>
#include "Graph.h"
class XLex
{
public:
	XLex();

	bool Read(std::string input);  // 读取表达式
	bool Check();  // 检查表达式是否合法
	void Reset();  // 清空
	bool toDFA();  // 将表达式转换为DFA
	bool toNFA();  // 将表达式转换为NFA
	bool toMinDFA();  // 将DFA转换为最小化DFA
	void toSuffix();  // 转为后置表达式
	void ShowNFA();  // 显示NFA
	void ShowDFA();  // 显示DFA
	void ShowMinDFA();  // 显示最小化DFA

private:
	Graph NFA;
	Graph DFA;
	Graph minDFA;
	Graph minDFA_t;

	int nfa_start_node;  // NFA开始的结点编号
	int nfa_end_node;  // NFA结束的结点编号

	std::string expression;  // 表达式
	std::string suffix;  // 后缀表达式
	std::map<char, int> o_priority;  // 运算符的优先级
	std::stack<int> st;  // 存放节点编号的栈
	std::set<char> chars;  // 字符节点
	std::vector<std::vector<std::set<int>>> state_chart;  // 存放状态表的vector
	std::map<char, int> col_value;  // 状态转换表的列索引，e.g. a->1.


	bool isOperator(char c);  // 判断是否是运算符
	void preProcess();  // 预处理表达式，插入连接符

	// suffix -> NFA
	void BuildCell(char c);  // 建立独立的节点
	void Joint();  // ab
	void Union();  // a|b
	void Closure();  // a*
	void Selectable();  // a?

	// NFA -> DFA
	void getNeighbor(int v, char c, std::set<int>& ni);  // 找当前节点相邻的标记为c的节点
	void e_closure(int v, std::set<int>& ei);  // 找出所有epsilon闭包

	// DFA -> MinDFA
	int dfa_transform(int v, char c, std::map<int, std::vector<int>> mp);
	bool is_equal(int v1, int v2, std::map<int, std::vector<int>> mp);
};

