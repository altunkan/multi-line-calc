#include <string>
#include <iostream>
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <ctype.h>
#include <unordered_map>
#include <stack>
#include <stdlib.h>
#include <vector>

using namespace std;

const string keyword_in = "IN";
const string keyword_out = "OUT";
const string keyword_if = "IF";
const string keyword_then = "THEN";
const string keyword_else = "ELSE";
const string keyword_loop = "LOOP";
const string keyword_times = "TIMES";

enum statement_type { CALCULATION_STATEMENT, IF_STATEMENT, LOOP_STATEMENT, UNKNOWN_STATEMENT };

struct statement {
	vector<string> statement_steps;
	vector<string> postfix_statement_steps;
	enum statement_type statement_type;
};

vector<string> parse_calculation_statement_steps(string statement_line);
vector<string> parse_if_statement_steps(string statement_line);
bool is_input_file(string filename);
void populate_assignment(string line, unordered_map<string, string>& assignments);
vector<string> infix_to_postfix(vector<string> statement_steps);
double calculate_operation(vector<string> postfix, unordered_map<string, string>& assignments);

bool is_arithmatic_operator(char ch);
bool is_arithmatic_operator(string str);
bool is_logical_operator(char current, char next);
bool is_logical_operator(string str);
int get_precedence(char ch);
int get_precedence(string str);
void pop_and_append(stack<string>& st, vector<string>& appendable);

int main(int argc, char** argv)
{
	DIR* dir;
	struct dirent* dirent;
	
	if ((dir = opendir(".")) == NULL)
	{
		fprintf(stderr, "Cannot open directory!\n");
		return -1;
	}

	while ((dirent = readdir(dir)) != NULL)
	{
		unsigned char file_type = dirent->d_type;
		string filename = dirent->d_name;
		if (!is_input_file(filename))
		{
			continue;
		}

		ifstream input_file(filename);
		if (input_file.is_open())
		{
			unordered_map<string, string> assignments;
			string line;
			while (getline(input_file, line))
			{
				populate_assignment(line, assignments);
			}

			string out_statement_line = assignments[assignments[keyword_out]];
			double result = 0.0;
			struct statement statement;
			if (out_statement_line.find(keyword_if) != string::npos)
			{
				vector<string> if_statement_steps = parse_if_statement_steps(out_statement_line);
				statement.statement_type = IF_STATEMENT;
				statement.statement_steps = if_statement_steps;
				vector<string> logical_steps;
				vector<string> then_steps;
				vector<string> else_steps;

				bool after_then = false;
				bool after_else = false;
				for (vector<string>::iterator it = if_statement_steps.begin()+1; it != if_statement_steps.end(); ++it)
				{
					string current = *it;
					if (!after_then) 
					{
						if (current == keyword_then)
						{
							after_then = true;
							continue;
						}

						logical_steps.push_back(current);
					} 
					else if (after_then && !after_else)
					{
						if (current == keyword_else)
						{
							after_else = true;
							continue;
						}
						then_steps.push_back(current);
					}
					else
					{
						else_steps.push_back(current);
					}
				}

				vector<string> logical_steps_postfix = infix_to_postfix(logical_steps);
				int logical_expression_result = calculate_operation(logical_steps_postfix, assignments);
				if (logical_expression_result)
				{
					result = calculate_operation(infix_to_postfix(then_steps), assignments);
				}
				else
				{
					result = calculate_operation(infix_to_postfix(else_steps), assignments);
				}
			}
			else if (out_statement_line.find(keyword_loop) != string::npos)
			{
				statement.statement_type = LOOP_STATEMENT;
			}
			else
			{
				statement.statement_type = CALCULATION_STATEMENT;
				statement.statement_steps = parse_calculation_statement_steps(out_statement_line);
				statement.postfix_statement_steps = infix_to_postfix(statement.statement_steps);
				result = calculate_operation(statement.postfix_statement_steps, assignments);
			}

			cout << result << endl;
			string output_filename = filename.substr(0, filename.find_last_of(".")) + ".out";
			ofstream output_file(output_filename);
			output_file << result;
			output_file.close();
		}
		else
		{
			fprintf(stderr, "Cannot open file %s!\n", filename.c_str());
			continue;
		}
		input_file.close();
	}
	closedir(dir);
	return 0;
}

vector<string> parse_calculation_statement_steps(string statement_line)
{
	vector<string> statement_steps;
	string keyword = "";

	int i = 0;
	while (i < statement_line.size())
	{
		char current = statement_line[i];
		char next = 0;

		if (i + 1 < statement_line.size()) next = statement_line[i + 1];

		if (is_arithmatic_operator(current) || current == ')' || is_logical_operator(current, next))
		{
			if (!keyword.empty()) statement_steps.push_back(keyword);

			if (is_logical_operator(current, next))
			{
				string logical_operator = "";
				logical_operator += current;
				if (next == '=')
				{
					logical_operator += next;
					i++;
				}
				statement_steps.push_back(logical_operator);
			}
			else
			{
				statement_steps.push_back(string(1, current));
			}

			keyword.clear();
		}
		else if (current == '(')
		{
			statement_steps.push_back(string(1, current));
		}
		else
		{
			keyword += current;
		}
		i++;
	}

	statement_steps.push_back(keyword);

	return statement_steps;
}

vector<string> parse_if_statement_steps(string statement_line)
{
	vector<string> statement_steps;
	
	int if_position = statement_line.find(keyword_if);
	int then_position = statement_line.find(keyword_then);
	int else_position = statement_line.find(keyword_else);
	
	string logical_statement = statement_line.substr(if_position + keyword_if.length(), then_position - if_position - keyword_if.length());
	string then_statement = statement_line.substr(then_position + keyword_then.length(), else_position - then_position - keyword_then.length());
	string else_statement = statement_line.substr(else_position + keyword_else.length(), statement_line.length() - 1);

	statement_steps.push_back(keyword_if);
	vector<string> logical_statement_steps = parse_calculation_statement_steps(logical_statement);
	statement_steps.insert(statement_steps.end(), logical_statement_steps.begin(), logical_statement_steps.end());

	statement_steps.push_back(keyword_then);
	vector<string> then_statement_steps = parse_calculation_statement_steps(then_statement);
	statement_steps.insert(statement_steps.end(), then_statement_steps.begin(), then_statement_steps.end());

	statement_steps.push_back(keyword_else);
	vector<string> else_statement_steps = parse_calculation_statement_steps(else_statement);
	statement_steps.insert(statement_steps.end(), else_statement_steps.begin(), else_statement_steps.end());
	return statement_steps;
}

bool is_input_file(string filename)
{
	if (filename.substr(filename.find_last_of(".") + 1) == "inp") return true;
	return false;
}

void populate_assignment(string line, unordered_map<string, string>& assignments)
{
	int k = 0, console_variable_pos = 0;
	while (k < line.size() &&
		line[k] != '\0' &&
		(line[k] == ' ' || isdigit(line[k]))) k++; // skip leading digits and whitespaces

	bool after_equal = false;
	bool from_console = false;
	string variable;
	string assignment;
	for (int i = k; i < line.size(); i++)
	{
		char ch = line[i];
		if (ch == ' ') continue; //if whitespace, continue

		if (!after_equal)
		{
			if (ch == '=')
			{
				after_equal = true;
				continue;
			}
			else if (variable == keyword_out)
			{
				after_equal = true;
			}
			else if (variable == keyword_in)
			{
				from_console = true;
				console_variable_pos = i;
				break;
			}
		}

		if (!after_equal) variable += ch;
		else assignment += ch;
	}

	if (from_console)
	{
		variable = line.substr(console_variable_pos, line.length() - 1);
		int i = variable.length();
		while (variable[--i] == ' ') variable.erase(i, 1);
		cout << "Enter a value for " << variable << ": ";
		cin >> assignment;
	}

	assignments[variable] = assignment;
}

vector<string> infix_to_postfix(vector<string> statement_steps)
{
	stack<string> st;
	vector<string> postfix;

	for (vector<string>::iterator it = statement_steps.begin(); it != statement_steps.end(); ++it)
	{
		string current = *it;
		if (is_arithmatic_operator(current) || is_logical_operator(current))
		{
			if (st.empty())
			{
				st.push(current);
			}
			else
			{
				while (!st.empty() && is_arithmatic_operator(st.top()) && get_precedence(current) <= get_precedence(st.top()))
				{
					pop_and_append(st, postfix);
				}
				st.push(current);
			}
		}
		else if (current == "(")
		{
			st.push(current);
		}
		else if (current == ")")
		{
			while (!st.empty() && st.top() != "(")
			{
				pop_and_append(st, postfix);
			}

			if (st.top() == "(")  st.pop();
		}
		else
		{
			postfix.push_back(current);
		}
	}

	while (!st.empty())
	{
		string ch = st.top();
		st.pop();
		postfix.push_back(ch);
	}

	return postfix;
}

double calculate_operation(vector<string> postfix, unordered_map<string, string>& assignments)
{
	double result = 0.0;
	if (postfix.empty() || assignments.empty()) return result;

	stack<string> st;
	for (vector<string>::iterator it = postfix.begin(); it != postfix.end(); ++it)
	{
		string current = *it;
		if (is_arithmatic_operator(current) || is_logical_operator(current))
		{
			string first_str = st.top();
			st.pop();
			string second_str = "0.0";
			
			if (!st.empty())
			{
				second_str = st.top();
				st.pop();
			}

			double first = atof(first_str.c_str());
			double second = atof(second_str.c_str());

			if (current == "+")
			{
				double result = first + second;
				st.push(to_string(result));
			}
			else if (current == "-")
			{
				double result = first - second;
				st.push(to_string(result));
			}
			else if (current == "*")
			{
				double result = first * second;
				st.push(to_string(result));
			}
			else if (current == "/" && second != 0.0)
			{
				if (first_str.find(".") == string::npos && second_str.find(".") == string::npos)
				{
					int result = (int)first / (int)second;
					st.push(to_string(result));
				}
				else
				{
					double result = first / second;
					st.push(to_string(result));
				}
			}
			else if (current == "<")
			{
				if (second < first) st.push("1");
				else st.push("0");
			}
			else if (current == ">")
			{
				if (second > first) st.push("1");
				else st.push("0");
			}
			else if (current == "==")
			{
				if (second == first) st.push("1");
				else st.push("0");
			}
			else if (current == "<=")
			{
				if (second <= first) st.push("1");
				else st.push("0");
			}
			else if (current == ">=")
			{
				if (second >= first) st.push("1");
				else st.push("0");
			}
			else if (current == "!")
			{
				if (!first) st.push("1");
				else st.push("0");
			}
			else if (current == "!=")
			{
				if (second != first) st.push("1");
				else st.push("0");
			}
		}
		else
		{
			string value = assignments[current];
			if (!value.empty())
			{
				// check the first character with isdigit to make sure that it is not an equation
				// if it is an equation, find the result of all nested equations
				if (isdigit(value[0])) st.push(value);
				else st.push(to_string(calculate_operation(infix_to_postfix(parse_calculation_statement_steps(value)), assignments))); //recursively do this until a value is calculated
			}
			else
			{
				// value is a constant number
				if (isdigit(current[0])) st.push(current);
			}
		}
	}

	if (!st.empty())
	{
		result = atof(st.top().c_str());
		st.pop();
	}

	return result;
}

bool is_arithmatic_operator(char ch)
{
	return ch == '+' || ch == '-' || ch == '/' || ch == '*';
}

bool is_arithmatic_operator(string str)
{
	return is_arithmatic_operator(str[0]);
}

bool is_logical_operator(char current, char next)
{
	return current == '<' ||
		current == '>' ||
		(current == '=' && next == '=') ||
		(current == '<' && next == '=') ||
		(current == '>' && next == '=') ||
		current == '!' ||
		(current == '!' && next == '=');
}

bool is_logical_operator(string str)
{
	return str == "<" ||
		str == ">" ||
		str == "==" ||
		str == ">=" ||
		str == "<=" ||
		str == "!" ||
		str == "!=";
}

int get_precedence(char ch)
{
	if (ch == '+' || ch == '-') return 1;
	else if (ch == '*' || ch == '/') return 2;
	else return -1;
}

int get_precedence(string str)
{
	if (is_logical_operator(str)) return 0;
	else return get_precedence(str[0]);
}

void pop_and_append(stack<string>& st, vector<string>& appendable)
{
	string top = st.top();
	st.pop();
	appendable.push_back(top);
}