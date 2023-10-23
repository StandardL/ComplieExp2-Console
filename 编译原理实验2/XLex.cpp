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
    // 检查是否存在除了连接(默认直接连起来) | * () ? 以外的字符
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
}

bool XLex::toDFA()
{
    queue<set<int>> q;
    map<set<int>, int> state_set;  // Key-状态集合, Value-DFA节点编号
    
    set<int> si; si.clear();
    si.emplace(nfa_start_node);  // 放入初始节点
    e_closure(nfa_start_node, si);  // 求其e-闭包
    int DFA_nodenum = 0;  // DFA节点编号
    DFA.insertVertix();  // DFA图中插入初始节点
    state_set.emplace(si, DFA_nodenum);
    DFA_nodenum++;  //放进去后记得自增节点编号
    state_chart[state_set[si]][0] = si;  // 状态表的第一列存放状态集合
    q.push(si);  // 待处理的节点队列

    while (!q.empty())
    {
        auto sc = q.front(); q.pop();
        state_chart[state_set[sc]][0] = sc;
        int state_index = 1;
        for (auto& c : chars)  // 构建列索引
        {
            col_value[c] = state_index;
            set<int> next_state;  // 记录经过字符"c"时能到达的节点编号

            for (auto& itc : sc)
            {
                getNeighbor(itc, c, next_state);
            }
            if (next_state.empty())  // 当前经过字符"c"时，没有可达节点状态
            {
                state_chart[state_set[sc]][state_index] = next_state;
                state_index++;
                continue;
            }
            for (auto& nitc : next_state)  // 若有，对其求取e-闭包
            {
                e_closure(nitc, next_state);
            }
            state_chart[state_set[sc]][state_index] = next_state;
            state_index++;

            if (sc == next_state)  // 独立处理闭包情况，避免死循环
            {
                DFA.insertEdge(state_set[sc], state_set[sc], c);
                continue;
            }
            if (state_set.count(next_state) == 0)  // 没有存过新的可达节点集合时要先存一下
            {
                state_set[next_state] = DFA_nodenum;
                DFA_nodenum++;
                DFA.insertVertix();
                DFA.insertEdge(state_set[sc], state_set[next_state], c);
                q.push(next_state);
            }
            else  // 否则直接将这两个集合连起来，表示状态集合A经过字符"c"变换可以得到状态集合B.
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
    vector<int> S1, S2;  // DFA节点集合，S1终态集合，S2非终态集合
    for (int i = 0; i < DFA.NumofVertixes(); i++)
    {
        if (state_chart[i][0].count(nfa_end_node) != 0)
            S1.emplace_back(i);
        else
			S2.emplace_back(i);
	}

    queue<vector<int>> q;  // 待处理的节点队列
    q.push(S1); q.push(S2);
    
    map<int, vector<int>> mp_temp;  // 临时存放当前划分的节点集合与DFA状态转换表的映射
    int minDFA_nodenum_temp = 0;  // 当前划分的最小化DFA节点数
    mp_temp.emplace(minDFA_nodenum_temp++, S1);
    mp_temp.emplace(minDFA_nodenum_temp++, S2);

    map<int, vector<int>> mp;  // miniDFA->DFA的映射
    int minDFA_nodenum = 0;  // 最小化DFA节点数

    int processing = -1;  // 当前处理的集合编号

    while (!q.empty())  // 获取映射表
    {
        auto s = q.front(); q.pop();
        processing++;

        if (s.empty())
            continue;
        int size = s.size();
        if (size == 1)  // 只有一个节点，直接放入最小化DFA中
        {
			mp[minDFA_nodenum++] = s;
			continue;
		}
        // 需要划分的子集合
        vector<vector<int>> sub_set;

        int node = s[0];
        bool is_same = true;  // 检查当前集合是否和目前已有的集合相同
        for (int i = 1; i < size; i++)
        {
            // 如果当前节点的转换表是否和已有的节点转换表不一致，说明需要继续划分
            if (!is_equal(node, s[i], mp_temp))
            {
				is_same = false;
                int cur_node = s[i];
                vector<int> cur_set;
                for (auto it = s.begin()+1; it != s.end();)
                {
                    // 在划分子集过程中确保是相同集合的元素被放在一起
                    if (is_equal(*it, cur_node, mp_temp))
                    {
                        cur_set.emplace_back(*it);
                        s.erase(it);
                    }
                    else
                        it++;
                }
                sub_set.emplace_back(cur_set);
                size = s.size();
			}
        }
        if (is_same)  // 如果相同，直接放入最小化DFA中
        {
            mp[minDFA_nodenum++] = s;
        }
        else // 否则，把划分剩余的子集s加入到待处理队列
        {
            sub_set.emplace_back(s);
            for (auto& it : sub_set)
            {
				q.emplace(it);
                mp_temp[minDFA_nodenum_temp++] = it;
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

    // 生成minDFA
    for (int i = 0; i < mp.size(); i++)  // 插入节点
        minDFA.insertVertix();
    for (int i = 0; i < mp.size(); i++)  // 插入边
    {
        for (auto j = 0; j < mp[i].size(); j++)
        {
            auto edges = DFA.G[mp[i][j]];
            for (auto& e : edges)  // 找DFA节点中的每个边
            {
				// 找min_dfa节点对应的dfa节点集中的节点
                for (int k = 0; k < mp.size(); k++)
                {
                    auto target = find(mp[k].begin(), mp[k].end(), e.end);
                    if (target != mp[k].end())
                    {
						bool res = minDFA.insertEdge(i, k, e.character);
                        cout << "Insert Result: " << res << endl;
						break;
					}
				}
			}
        }
    }
    for (int i = minDFA.NumofVertixes() - 1; i >= 0; i--)  // 反向
    {
		minDFA_t.insertVertix();
        for (auto& j : minDFA.G[i])
        {
            minDFA_t.insertEdge(abs(i - minDFA.NumofVertixes() + 1), abs(j.end - minDFA.NumofVertixes() + 1), j.character);
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
		assert(l >= 0);  // 检查括号是否匹配
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

void XLex::BuildCell(char c)  // 建立原子节点
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

void XLex::getNeighbor(int v, char c, set<int>& ni)  // 获取经过c之后的可达状态
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

int XLex::dfa_transform(int v, char c, map<int, vector<int>> mp)
{
    int node = -114514;
    for (auto& e : DFA.G[v])  // 回到DFA图中找这个编号的节点是否有c这条边
    {
        if (e.character == c)
        {
			node = e.end;
			break;
		}
	}
    if (node == -114514) // 找不到就附一个他本身的值
        node = v;
    int target;
    for (auto& i : mp)  // 在映射表中找到这个节点的映射值编号
    {
        for (auto& j : i.second)
        {
            if (j == node)
                target = i.first;
        }
    }
    return target;
}

bool XLex::is_equal(int v1, int v2, map<int, vector<int>> mp)
{
    for (auto& c : chars)
    {
        /*if (dfa_transform(v1, c, mp) != dfa_transform(v2, c, mp))
			return false;*/
        if (state_chart[v1][c].count(nfa_end_node) != state_chart[v2][c].count(nfa_end_node))
			return false;
    }
    return true;
}

void XLex::toSuffix()
{
    // 转成后缀表达式之前先对符号进行预处理
    preProcess();
    suffix.clear();
    stack<char> op;
    for (auto& c : expression)
    {
        if (isOperator(c))  // 运算符
        {
            if (op.empty())  // 为空直接入栈
                op.emplace(c);
            else
            {
                while (!op.empty())  // 优先级大的先出栈
                {
                    char t = op.top();
                    if (o_priority[c] <= o_priority[t])
                    {
                        op.pop();
                        suffix.append(1, t); // 1表示重复一次
                    }
                    else
                        break;
                }
                op.emplace(c);
            }
        }
        else  // 字符
        {
            if (c == '(')  // 左括号入栈
                op.emplace(c);
            else if (c == ')')  // 遇到右括号出栈
            {
                while (op.top() != '(')
                {
                    auto t = op.top();
                    suffix.append(1, t);
                    op.pop();
                }
                // 把剩余的左括号也出栈
                op.pop();
            }
            else
                suffix.append(1, c);  // 非符号直接放入后缀表达式中
        }
    }
    // 把剩余的运算符也加上去
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
    // 输出首行标题
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
