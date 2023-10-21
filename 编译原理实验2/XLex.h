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

	bool Read(std::string input);  // ��ȡ���ʽ
	bool Check();  // �����ʽ�Ƿ�Ϸ�
	void Reset();  // ���
	bool toDFA();  // �����ʽת��ΪDFA
	bool toNFA();  // �����ʽת��ΪNFA
	void toSuffix();  // תΪ���ñ��ʽ
	void ShowNFA();  // ��ʾNFA
	void ShowDFA();

private:
	Graph NFA;
	Graph DFA;

	int nfa_start_node;  // NFA��ʼ�Ľ����
	int nfa_end_node;
	std::string expression;  // ���ʽ
	std::string suffix;  // ��׺���ʽ
	std::map<char, int> o_priority;  // ����������ȼ�
	std::stack<int> st;  // ��Žڵ��ŵ�ջ
	std::vector<char> chars;  // �ַ��ڵ�

	bool isOperator(char c);  // �ж��Ƿ��������
	void preProcess();  // Ԥ������ʽ���������ӷ�

	// suffix -> NFA
	void BuildCell(char c);  // ���������Ľڵ�
	void Joint();  // ab
	void Union();  // a|b
	void Closure();  // a*
	void Selectable();  // a?

	// NAF -> DFA
	void getNeighbor(int v, char c, set<int>& ni);  // �ҵ�ǰ�ڵ����ڵı��Ϊc�Ľڵ�
	void e_closure(int v, set<int>& ei);  // �ҳ�����epsilon�հ�
};

