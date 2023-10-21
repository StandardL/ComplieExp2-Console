#include "XLex.h"
#include <cassert>

using namespace std;

XLex::XLex()
{
    expression = "";

    o_priority = { {'(', 1}, {'|', 2}, {'^', 3}, {'*', 4}, {'?', 4} };

}

bool XLex::Read(std::string input)
{
    expression = input;
    if (Check())
        return true;
    return false;
}

bool XLex::Check()
{
    const int len = expression.length();
    // ����Ƿ���ڳ�������(Ĭ��ֱ��������) | * () ? ������ַ�
    if (expression.find('+') < len || expression.find('[') < len || expression.find(']') < len)
        return false;
    return true;
}

void XLex::Reset()
{
    expression.clear();
    suffix.clear();
    NFA.Reset();
    DFA.Reset();
    chars.clear();
    while (!st.empty())
        st.pop();
}

bool XLex::toDFA()
{
    return false;
}

bool XLex::toNFA()
{
    for (auto& c : suffix)
    {
        if (isalpha(c) || isdigit(c))
        {
            BuildCell(c);
            chars.emplace_back(c);
        }
        else
        {
            switch (c)
            {
            case '*': Closure(); break;
            case '?': Selectable(); break;
            case '^': Joint(); break;
            case '|': Union(); break;
            default: break;
            }
        }
    }

    int e = st.top(); st.pop();
    int s = st.top();
    st.emplace(e);
    nfa_start_node = s;
    nfa_end_node = e;
    return true;
}

void XLex::preProcess()
{
    string newexp;
    assert(expression.length() > 0);
    int l = 0;
    for (auto & c : expression)
	{
        if (c == '(')
            l++;
		else if (c == ')')
			l--;
		assert(l >= 0);  // ��������Ƿ�ƥ��
	}
    assert(l == 0);

    char c, c2;
    for (int i = 0; i < expression.length(); i++)
    {
        c = expression[i];
        if (i + 1 < expression.length())
        {
            c2 = expression[i + 1];
            newexp.append(1, c);
            if (c != '(' && c2 != ')' && c != '|' && c2 != '|' && c2 != '*' && c2 != '?')
                newexp.append(1, '^');
        }
    }
    newexp.append(1, expression[expression.length() - 1]);
    expression = newexp;
}

void XLex::BuildCell(char c)  // ����ԭ�ӽڵ�
{
    int s, e;
    s = NFA.NumofVertixes();
    st.emplace(s);
    NFA.insertVertix();
    e = NFA.NumofVertixes();
    st.emplace(e);
    NFA.insertVertix();
    NFA.insertEdge(s, e, c);
}

void XLex::Joint()  // ab
{
    int b_e = st.top(); st.pop();
    int b_s = st.top(); st.pop();
    int a_e = st.top(); st.pop();
    int a_s = st.top(); st.pop();
    NFA.insertEdge(a_e, b_s, 'e');  // e for epsilon.
    
    // view them as a new CELL
    st.emplace(a_s);
    st.emplace(b_e);
}

void XLex::Union()  // a|b
{
    int s = NFA.NumofVertixes(); NFA.insertVertix();
    int e = NFA.NumofVertixes(); NFA.insertVertix();
    
    int b_e = st.top(); st.pop();
    int b_s = st.top(); st.pop();
    int a_e = st.top(); st.pop();
    int a_s = st.top(); st.pop();

    // Add new Edges
    NFA.insertEdge(s, a_s, 'e');
    NFA.insertEdge(s, b_s, 'e');
    NFA.insertEdge(a_e, e, 'e');
    NFA.insertEdge(b_e, e, 'e');

    // view them as a new CELL
    st.emplace(s);
    st.emplace(e);
}

void XLex::Closure()  // a*
{
    int s = NFA.NumofVertixes(); NFA.insertVertix();
    int e = NFA.NumofVertixes(); NFA.insertVertix();

    int a_e = st.top(); st.pop();
    int a_s = st.top(); st.pop();

    // Add new Edges
    NFA.insertEdge(s, a_s, 'e');
    NFA.insertEdge(s, e, 'e');
    NFA.insertEdge(a_e, e, 'e');
    NFA.insertEdge(a_e, a_s, 'e');

    // view them as a new CELL
    st.emplace(s);
    st.emplace(e);
}

void XLex::Selectable()  // a?
{
    int s = NFA.NumofVertixes(); NFA.insertVertix();
    int e = NFA.NumofVertixes(); NFA.insertVertix();

    int a_e = st.top(); st.pop();
    int a_s = st.top(); st.pop();

    // Add new Edges
    NFA.insertEdge(s, a_s, 'e');
    NFA.insertEdge(s, e, 'e');
    NFA.insertEdge(a_e, e, 'e');

    // view them as a new CELL
    st.emplace(s);
    st.emplace(e);
}

void XLex::getNeighbor(int v, char c, set<int>& ni)
{
    auto target = NFA.G[v];
    for (auto& e : target)
    {
        if (e.character == c)
            ni.insert(e.end);
    }
}

void XLex::e_closure(int v, set<int>& ei)
{
    auto target = NFA.G[v];
}

void XLex::toSuffix()
{
    // ת�ɺ�׺���ʽ֮ǰ�ȶԷ��Ž���Ԥ����
    preProcess();
    suffix.clear();
    stack<char> op;
    for (auto& c : expression)
    {
        if (isOperator(c))  // �����
        {
            if (op.empty())  // Ϊ��ֱ����ջ
                op.emplace(c);
            else
            {
                while (!op.empty())  // ���ȼ�����ȳ�ջ
                {
                    char t = op.top();
                    if (o_priority[c] <= o_priority[t])
                    {
                        op.pop();
                        suffix.append(1, t); // 1��ʾ�ظ�һ��
                    }
                    else
                        break;
                }
                op.emplace(c);
            }
        }
        else  // �ַ�
        {
            if (c == '(')  // ��������ջ
                op.emplace(c);
            else if (c == ')')  // ���������ų�ջ
            {
                while (op.top() != '(')
                {
                    auto t = op.top();
                    suffix.append(1, t);
                    op.pop();
                }
                // ��ʣ���������Ҳ��ջ
                op.pop();
            }
            else
                suffix.append(1, c);  // �Ƿ���ֱ�ӷ����׺���ʽ��
        }
    }
    // ��ʣ��������Ҳ����ȥ
    while (!op.empty())
    {
        auto t = op.top(); op.pop();
        suffix.append(1, t);
    }

    cout << suffix << endl;
}

void XLex::ShowNFA()
{
    string res;
    cout << "-------------------------------------------\n";
    for (int i = 0; i < NFA.NumofVertixes(); i++)
    {
        for (auto& e : NFA.G[i])
        {
            cout << "From: " << i << "\tTo: " << e.end << "\tChar: " << e.character << endl;
        }
    }
    cout << "Start node is " << nfa_start_node << endl;
    cout << "End node is " << nfa_end_node << endl;
    cout << "-------------------------------------------\n";
}

void XLex::ShowDFA()
{
}

bool XLex::isOperator(char c)
{
    switch (c)
    {
    case '*':
    case '|':
    case '^':
    case '?':
        return true;
    default:
        return false;
    }
}
