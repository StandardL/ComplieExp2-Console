#include "XLex.h"
#include <cassert>

using namespace std;

XLex::XLex()
{
    expression = "";

    o_priority = { {'(', 1}, {'|', 2}, {'^', 3}, {'*', 4}, {'?', 4} };
    state_chart.resize(110);
    for (int i = 0; i < 110; i++)
        state_chart[i].resize(110);
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
    queue<set<int>> q;
    map<set<int>, int> state_set;  // Key-״̬����, Value-DFA�ڵ���
    
    set<int> si; si.clear();
    si.emplace(nfa_start_node);  // �����ʼ�ڵ�
    e_closure(nfa_start_node, si);  // ����e-�հ�
    int DFA_nodenum = 0;  // DFA�ڵ���
    DFA.insertVertix();  // DFAͼ�в����ʼ�ڵ�
    state_set.emplace(si, DFA_nodenum);
    DFA_nodenum++;  //�Ž�ȥ��ǵ������ڵ���
    state_chart[state_set[si]][0] = si;  // ״̬��ĵ�һ�д��״̬����
    q.push(si);  // ������Ľڵ����

    while (!q.empty())
    {
        auto sc = q.front(); q.pop();
        state_chart[state_set[sc]][0] = sc;
        int state_index = 1;
        for (auto& c : chars)  // ����������
        {
            col_value[c] = state_index;
            set<int> next_state;  // ��¼�����ַ�"c"ʱ�ܵ���Ľڵ���

            for (auto& itc : sc)
            {
                getNeighbor(itc, c, next_state);
            }
            if (next_state.empty())  // ��ǰ�����ַ�"c"ʱ��û�пɴ�ڵ�״̬
            {
                state_chart[state_set[sc]][state_index] = next_state;
                state_index++;
                continue;
            }
            for (auto& nitc : next_state)  // ���У�������ȡe-�հ�
            {
                e_closure(nitc, next_state);
            }
            state_chart[state_set[sc]][state_index] = next_state;
            state_index++;

            if (sc == next_state)  // ��������հ������������ѭ��
            {
                DFA.insertEdge(state_set[sc], state_set[sc], c);
                continue;
            }
            if (state_set.count(next_state) == 0)  // û�д���µĿɴ�ڵ㼯��ʱҪ�ȴ�һ��
            {
                state_set[next_state] = DFA_nodenum;
                DFA_nodenum++;
                DFA.insertVertix();
                DFA.insertEdge(state_set[sc], state_set[next_state], c);
                q.push(next_state);
            }
            else  // ����ֱ�ӽ���������������������ʾ״̬����A�����ַ�"c"�任���Եõ�״̬����B.
            {
                DFA.insertEdge(state_set[sc], state_set[next_state], c);
            }
        }
    }

    return true;
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

void XLex::getNeighbor(int v, char c, set<int>& ni)  // ��ȡ����c֮��Ŀɴ�״̬
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
    for (auto& e : target)
    {
        if (e.character == 'e')
        {
            ei.insert(e.end);
            e_closure(e.end, ei);
        }
    }
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
    cout << "-----------------NFA---------------------\n";
    for (int i = 0; i < NFA.NumofVertixes(); i++)
    {
        for (auto& e : NFA.G[i])
        {
            cout << "From: " << i << "\tTo: " << e.end << "\tChar: " << e.character << endl;
        }
    }
    cout << "Start node is " << nfa_start_node << endl;
    cout << "End node is " << nfa_end_node << endl;
    cout << "-----------------NFA---------------------\n";
}

void XLex::ShowDFA()
{
    cout << "-----------------DFA---------------------\n";
    // ������б���
    for (int i = 0; i < DFA.NumofVertixes(); i++)
    {
        for (auto& e : DFA.G[i])
        {
            cout << "From: " << i << "\tTo: " << e.end << "\tChar: " << e.character << endl;
        }
    }
    cout << "-----------------DFA---------------------\n";
    cout << endl;
    cout << "--------------State Chart----------------\n";
    for (int i = 0; i < DFA.NumofVertixes(); i++)
    {
        for (auto& e : state_chart[i][0])
        {
            cout << e << ",";
        }
        cout << "\t\t";
        for (int j = 1; j < state_chart[i].size(); j++)
        {
            for (auto& e : state_chart[i][j])
            {
                cout << e << ",";
            }
            cout << "\t\t";
        }
        cout << endl;
    }
    cout << "--------------State Chart----------------\n";
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
