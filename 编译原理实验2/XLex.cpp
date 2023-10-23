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
    min_state_chart.resize(110);
    for (int i = 0; i < 110; i++)
		min_state_chart[i].resize(110);
    memset(isAccepted, 0, sizeof(isAccepted));
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
    minDFA.Reset();
    minDFA_t.Reset();
    chars.clear();
    while (!st.empty())
        st.pop();
    state_chart.clear();
    state_chart.resize(110);
    for (int i = 0; i < 110; i++)
        state_chart[i].resize(110);
    col_value.clear();
    min_state_chart.clear();
    min_state_chart.resize(110);
    for (int i = 0; i < 110; i++)
        min_state_chart[i].resize(110);
    memset(isAccepted, 0, sizeof(isAccepted));
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
            chars.emplace(c);
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

bool XLex::toMinDFA()
{
    vector<int> S1, S2;  // DFA�ڵ㼯�ϣ�S1��̬���ϣ�S2����̬����
    for (int i = 0; i < DFA.NumofVertixes(); i++)
    {
        if (state_chart[i][0].count(nfa_end_node) != 0)
            S1.emplace_back(i);
        else
			S2.emplace_back(i);
	}

    queue<vector<int>> q;  // ������Ľڵ����
    q.push(S1); q.push(S2);
    
    map<int, vector<int>> mp_temp;  // ��ʱ��ŵ�ǰ���ֵĽڵ㼯����DFA״̬ת�����ӳ��
    int minDFA_nodenum_temp = 0;  // ��ǰ���ֵ���С��DFA�ڵ���
    mp_temp.emplace(minDFA_nodenum_temp++, S1);
    mp_temp.emplace(minDFA_nodenum_temp++, S2);

    map<int, vector<int>> mp;  // miniDFA->DFA��ӳ��
    int minDFA_nodenum = 0;  // ��С��DFA�ڵ���

    int processing = -1;  // ��ǰ����ļ��ϱ��

    while (!q.empty())  // ��ȡӳ���
    {
        auto s = q.front(); q.pop();
        processing++;

        if (s.empty())
            continue;
        if (s.size() == 1)  // ֻ��һ���ڵ㣬ֱ�ӷ�����С��DFA��
        {
			mp[minDFA_nodenum++] = s;
			continue;
		}
        // ��Ҫ���ֵ��Ӽ���
        vector<vector<int>> sub_set;

        int node = s[0];
        bool is_same = true;  // ��鵱ǰ�����Ƿ��Ŀǰ���еļ�����ͬ
        for (int i = 1; i < s.size(); i++)
        {
            // �����ǰ�ڵ��ת�����Ƿ�����еĽڵ�ת����һ�£�˵����Ҫ��������
            if (!is_equal(s[i], node))
            {
				is_same = false;
                int cur_node = s[i];
                vector<int> cur_set;
                for (auto it = s.begin()+1; it != s.end();)
                {
                    // �ڻ����Ӽ�������ȷ������ͬ���ϵ�Ԫ�ر�����һ��
                    if (is_equal(*it, cur_node))
                    {
                        cur_set.emplace_back(*it);
                        it = s.erase(it);
                    }
                    else
                        it++;
                }
                sub_set.emplace_back(cur_set);
			}
        }
        if (is_same)  // �����ͬ��ֱ�ӷ�����С��DFA��
        {
            mp[minDFA_nodenum++] = s;
        }
        else // ���򣬰ѻ���ʣ����Ӽ�s���뵽���������
        {
            sub_set.emplace_back(s);
            /*for (int it = 0; it < sub_set.size(); it++)
            {
				q.emplace(sub_set[it]);
                mp_temp[minDFA_nodenum_temp++] = sub_set[it];
            }*/
            for (auto i : sub_set)
            {
                q.emplace(i);
                mp_temp[minDFA_nodenum_temp++] = i;
            }
            vector<int> temp;
            mp_temp[processing] = temp;
        }
    }

    cout << "Test 2:\n";
    for (auto& i : mp)
    {
        cout << i.first << " ";
        for (auto& j: i.second)
			cout << j << ",";
        cout << endl;
    }

    // ����minDFA
    for (int i = 0; i < mp.size(); i++)  // ����ڵ�
        minDFA.insertVertix();
    for (int i = 0; i < mp.size(); i++)  // �����
    {
        for (auto j = 0; j < mp[i].size(); j++)
        {
            auto edges = DFA.G[mp[i][j]];
            for (auto& e : edges)  // ��DFA�ڵ��е�ÿ����
            {
				// ��min_dfa�ڵ��Ӧ��dfa�ڵ㼯�еĽڵ�
                for (int k = 0; k < mp.size(); k++)
                {
                    auto target = find(mp[k].begin(), mp[k].end(), e.end);
                    if (target != mp[k].end())
                    {
						bool res = minDFA.insertEdge(i, k, e.character);
						break;
					}
				}
			}
        }
    }
    for (int i = minDFA.NumofVertixes() - 1; i >= 0; i--)  // ����
    {
		minDFA_t.insertVertix();
        for (auto& j : minDFA.G[i])
        {
            minDFA_t.insertEdge(abs(i - minDFA.NumofVertixes() + 1), abs(j.end - minDFA.NumofVertixes() + 1), j.character);
        }
	}

    // ��¼��С��DFA��״̬ת����
    for (int i = 0; i < minDFA_t.NumofVertixes(); i++)
    {
        if (mp[abs(i - minDFA_t.NumofVertixes() + 1)].size() != 1)
        {
            set<int> s;
            for (auto& it : mp[abs(i - minDFA_t.NumofVertixes() + 1)])
            {
                for (auto& state : state_chart[it][0])
                    s.emplace(state);
                min_state_chart[i][0] = s;
            }
            for (int j = 1; j <= chars.size(); j++)
                min_state_chart[i][j] = state_chart[*mp[abs(i - minDFA_t.NumofVertixes() + 1)].begin()][j];
        }
        else
        {
            for (int j = 0; j <= chars.size(); j++)
				min_state_chart[i][j] = state_chart[*mp[abs(i - minDFA_t.NumofVertixes() + 1)].begin()][j];
        }
        // ��¼����״̬�����ҵ���С��DFA�Ŀ�ʼ�ڵ�
        for (int i = 0; i < minDFA_t.NumofVertixes(); i++)
        {
            if (min_state_chart[i][0].count(nfa_end_node) != 0)
            {
				isAccepted[i] = true;
                continue;
            }
            if (min_state_chart[i][0].count(nfa_start_node) != 0)
                mindfa_start_node = i;
        }
    }
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

bool XLex::is_equal(int v1, int v2)
{
    for (int i = 1; i <= chars.size(); i++)
    {
        if (state_chart[v1][i].count(nfa_end_node) != state_chart[v2][i].count(nfa_end_node))
            return false;
    }
    return true;
}

void XLex::toCode(int v, int level)
{
    string t;
    for (int l = 0; l < level; l++)  // ����������
        t.append("\t");

    if (isAccepted[v] == true)  // �����ǰ�ڵ��ǽ���״̬
        code.emplace_back(t + "Accept();");

    // * -> while��������if. �ر�ģ���ָ���Լ��ı�˵������*
    vector<char> while_statement, if_statement;
    for (auto& e : minDFA_t.G[v])
    {
		if (e.end == v)
			while_statement.emplace_back(e.character);
		else
			if_statement.emplace_back(e.character);
	}

    // ��ʼ�����
    code.emplace_back(t + "char CHAR = Input();");

    if (!while_statement.empty())  // ����while���
    {
        string line = "while(";
        int i = 0;
        string str;
        while (i < while_statement.size() - 1)
        {
			line.append("CHAR == ");
			str.append(1, while_statement[i]);
			str.append("||");
            line.append(str);
			i++;
		}
        str.clear();
        str.append(1, while_statement[i]);
        line.append("CHAR == "); line.append(str); line.append(")");
        code.emplace_back(t + line);
        code.emplace_back(t + "{");
        for (int l = 0; l < level; l++)  // ����������
            code.emplace_back(t + "\t");
        if (isAccepted[v] == true)
			code.emplace_back(t + "\tAccept();");
		code.emplace_back(t + "\tCHAR = Input();");
		code.emplace_back(t + "}");

        if (if_statement.empty())
            code.emplace_back(t + "Error();");
    }

    if (!if_statement.empty())  // ����if���
    {
        if (!minDFA_t.G[v].empty())
        {
            for (auto& e : minDFA_t.G[v])
            {
                if (e.end != v)
                {
                    
                    string line = t + "if(CHAR == ";
                    line.append(1, e.character);
                    line.append(")");
                    code.emplace_back(line);
                    code.emplace_back(t + "{");
                    
                    toCode(e.end, level + 1);
                    code.emplace_back(t + "}");
                    code.emplace_back(t + "else");
                }
            }
            string str;
            str.append(1, '0' + v);
            string line2 = "Error(" + str + ");";
            code.emplace_back(t + "\t" + line2);
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

void XLex::ShowMinDFA()
{
    cout << "-----------------MinDFA---------------------\n";
    for (int i = 0; i < minDFA_t.NumofVertixes(); i++)
    {
        for (auto& e : minDFA_t.G[i])
        {
			cout << "From: " << i << "\tTo: " << e.end << "\tChar: " << e.character << endl;
		}
	}
    //cout << "Start node is 0" << endl;
    
	cout << "-----------------MinDFA---------------------\n";
}

void XLex::ShowCode(string filename)
{
    toCode(mindfa_start_node, 0);
	ofstream out(filename, ios_base::app);
	for (auto& line : code)
		out << line << endl;
    out << endl;
	out.close();
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
