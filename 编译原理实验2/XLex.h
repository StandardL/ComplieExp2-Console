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

	bool Read(std::string input);  // ��ȡ���ʽ
	bool Check();  // �����ʽ�Ƿ�Ϸ�
	void Reset();  // ���
	bool toDFA();  // �����ʽת��ΪDFA
	bool toNFA();  // �����ʽת��ΪNFA
	bool toMinDFA();  // ��DFAת��Ϊ��С��DFA
	void toSuffix();  // תΪ���ñ��ʽ
	void ShowNFA();  // ��ʾNFA
	void ShowDFA();  // ��ʾDFA
	void ShowMinDFA();  // ��ʾ��С��DFA

private:
	Graph NFA;
	Graph DFA;
	Graph minDFA;
	Graph minDFA_t;

	int nfa_start_node;  // NFA��ʼ�Ľ����
	int nfa_end_node;  // NFA�����Ľ����

	std::string expression;  // ���ʽ
	std::string suffix;  // ��׺���ʽ
	std::map<char, int> o_priority;  // ����������ȼ�
	std::stack<int> st;  // ��Žڵ��ŵ�ջ
	std::set<char> chars;  // �ַ��ڵ�
	std::vector<std::vector<std::set<int>>> state_chart;  // ���״̬���vector
	std::map<char, int> col_value;  // ״̬ת�������������e.g. a->1.


	bool isOperator(char c);  // �ж��Ƿ��������
	void preProcess();  // Ԥ������ʽ���������ӷ�

	// suffix -> NFA
	void BuildCell(char c);  // ���������Ľڵ�
	void Joint();  // ab
	void Union();  // a|b
	void Closure();  // a*
	void Selectable();  // a?

	// NFA -> DFA
	void getNeighbor(int v, char c, std::set<int>& ni);  // �ҵ�ǰ�ڵ����ڵı��Ϊc�Ľڵ�
	void e_closure(int v, std::set<int>& ei);  // �ҳ�����epsilon�հ�

	// DFA -> MinDFA
	int dfa_transform(int v, char c, std::map<int, std::vector<int>> mp);
	bool is_equal(int v1, int v2, std::map<int, std::vector<int>> mp);
};

