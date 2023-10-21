#pragma once
#include <string>
#include <map>
#include <stack>
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
	void toSuffix();  // 转为后置表达式
	void ShowNFA();  // 显示NFA
	void ShowDFA();

private:
	Graph NFA;
	Graph DFA;

	int nfa_start_node;  // NFA开始的结点编号
	int nfa_end_node;
	std::string expression;  // 表达式
	std::string suffix;  // 后缀表达式
	std::map<char, int> o_priority;  // 运算符的优先级
	std::stack<int> st;  // 存放节点编号的栈
	std::vector<char> chars;  // 字符节点

	bool isOperator(char c);  // 判断是否是运算符
	void preProcess();  // 预处理表达式，插入连接符

	// suffix -> NFA
	void BuildCell(char c);  // 建立独立的节点
	void Joint();  // ab
	void Union();  // a|b
	void Closure();  // a*
	void Selectable();  // a?

	// NAF -> DFA
	void getNeighbor(int v, char c, set<int>& ni);  // 找当前节点相邻的标记为c的节点
	void e_closure(int v, set<int>& ei);  // 找出所有epsilon闭包
};

