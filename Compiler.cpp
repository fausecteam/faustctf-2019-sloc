#include <bits/stdc++.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

//#define ABORT(X) {stringstream s; s << X; throw *(new runtime_error(s.str().c_str()));}
#define ABORT(X) {cout << X; exit(1); }

string ip = "bin/sh"; // just a random default name

string getIPAddress() {
	string ipAddress = "FAUST_"; // just a random default name
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	// retrieve the current interfaces - returns 0 on success
	int success = getifaddrs(&interfaces);
	if (success == 0) {
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while(temp_addr != NULL) {
			if(temp_addr->ifa_addr != nullptr && temp_addr->ifa_addr->sa_family == AF_INET) {
				ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
				if (ipAddress[0] == '1' && ipAddress[1] == '0' && ipAddress[2] == '.') {
					freeifaddrs(interfaces);
					return ipAddress;
				}
			}
			temp_addr = temp_addr->ifa_next;
		}
	}
	// Free memory
	freeifaddrs(interfaces);
	return ipAddress;
}

uint8_t getId(const string & s) {
	string s2 = s + ip;
	uint16_t curx = 42, cury = 43;
	for (char c : s2) {
		uint16_t x = c % 16;
		uint16_t y = c / 16;
		curx = curx * 14323 + x;
		cury = cury * 53731 + y;
		
	}
	return (curx + cury) % 256;
}

vector<string> split(const string & s) {
	vector<string> res;
	string cur = "";
	for (char c : s) {
		if (c == ' ' || c == '\t') {
			if (cur != "") res.push_back(cur);
			cur = "";
		} else {
			cur += c;
		}
	}
	if (cur != "") res.push_back(cur);
	return res;
}

bool isValid(const string & s) {
	for (char c : s) {
		if (c >= 'a' && c <= 'z') continue;
		if (c >= 'A' && c <= 'Z') continue;
		if (c >= '0' && c <= '9') continue;
		return false;
	}
	return true;
}

// Library functions
struct LibFunc {
	string mappedName;
	int argcount;
	bool retValue;
	bool valid;
	LibFunc() : mappedName(), argcount(0), retValue(false), valid(false) {}
	LibFunc(string n, int a, bool r) : mappedName(n), argcount(a), retValue(r), valid(true) {}
};
vector<LibFunc> _functions;

/**
 * AST classes
 */
class Program;
struct Command {
	Program & prog;
	Command(Program & p) : prog(p) {}
	virtual ~Command() {}
	virtual void writeCode(ostream & out, Program & p) {}
	void dataToReg(Program & prog, const string & v, const string & reg, ostream & out);
};

struct Variable {
	int offset;
	bool array;
	int size; // size is in 8-Byte qwords
};

struct Program {
	int stacksize = 0;
	map<string, Variable> vars;
	vector<Command*> _code;
	vector<pair<string,string>> strings;
	set<string> labels;

	void writeCode(ostream & out) {
		out << ".global main" << endl;
		out << ".text" << endl;
		out << "main:" << endl;
		out << "push %rbp" << endl;
		out << "mov %rsp, %rbp" << endl;
		out << "sub $" << 8 * vars.size() << ", %rsp" << endl; // FIXME: this should account for arrays
		for (int i = 0; i < _code.size(); i++) {
			_code[i]->writeCode(out, *this);
		}
		out << "xor %rax, %rax" << endl; // exit code
		out << "mov %rbp, %rsp" << endl;
		out << "pop %rbp" << endl;
		out << "ret" << endl;
		out << endl;
		for (pair<string,string> s : strings) {
			out << s.first << ":" << endl;
			out << ".asciz \"" << s.second << "\"" << endl;
		}
	}
};

int varToOffset(const Program & prog, const string & v) {
	if (v[0] != '$') ABORT(v << " is not a valid variable");
	if (v[v.size()-1] == '}') { // array?
		int start = 0;
		while (start < v.size() && v[start] != '{') start++;
		if (start == v.size()) ABORT("Invalid array syntax.");
		char * endp;
		uint64_t a = strtoll(v.c_str() + start + 1, &endp, 0);
		if (endp != v.c_str() + v.size() - 1) ABORT("Invalid array index. Expected an integer");
		string var = v.substr(0, start);
		auto it = prog.vars.find(var);
		if (it == prog.vars.end()) ABORT("Array " << var << " not declared");
		if (!it->second.array) ABORT(var << " is not an array");
		if (a < 0 || a >= it->second.size) ABORT("index out of range");
		return it->second.offset-8*a;
	} else {
		auto it = prog.vars.find(v);
		if (it == prog.vars.end()) ABORT("variable " << v << " not declared");
		return it->second.offset;
	}
}

void Command::dataToReg(Program & prog, const string & v, const string & reg, ostream & out) {
	if (v[0] == '$') { // variable
		out << "mov -" << varToOffset(prog, v) << "(%rbp), " << reg << endl;
	} else if (v[0] == '"') { // string
		if (v.size() < 2) ABORT("string invalid");
		if (v[v.size()-1] != '\"') ABORT("string invalid");
		for (int i = 1; i < v.size() - 1; i++) {
			if (v[i] == '"') ABORT("string invalid");
		}
		string rs = v.substr(1, v.size() - 2);
		string stringId = "string" + to_string(prog.strings.size());
		prog.strings.push_back(make_pair(stringId, rs));
		out << "leaq " << stringId << "(%rip), " << reg << endl;
	} else if (v[0] == '-' || (v[0] >= '0' && v[0] <= '9')) { // integer constant
		for (int i = 1; i < v.size(); i++) {
			if (v[i] < '0' || v[i] > '9') ABORT("Integer number invalid");
		}
		out << "mov $" << v << ", " << reg << endl;
	} else {
		ABORT("invalid argument: '" << v << "'");
		return;
	}
}

vector<string> args = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

// ex.: CALL myfunc "argument1" 42 $b -> $ret
struct Call : public Command {
	string funcName;
	string retValue = "";
	vector<string> arguments;
	LibFunc * f;

	Call(const vector<string> & rem, Program & p) : Command(p) {
		if (rem.size() <= 1) ABORT("Call needs at least the function name");
		if (!isValid(rem[1])) ABORT("Invalid function name");
		funcName = rem[1];
		
		// FIXME: experimental features disabled. 
		if (funcName == "FOPEN" || funcName == "READCHAR") ABORT("Experimental features disabled");
		uint8_t h = getId(funcName);
		f = &_functions[h];
		if (!f->valid) ABORT("function " << funcName << " does not exist");
		
		for (int i = 2; i < rem.size(); i++) {
			if (rem[i] == "->") {
				if (i + 2 != rem.size()) ABORT("exactly one variable name expected after ->");
				retValue = rem[i + 1];
				break;
			} else {
				arguments.push_back(rem[i]);
			}
		}
		if (arguments.size() > f->argcount) ABORT("too many arguments");
	}

	virtual void writeCode(ostream & out, Program & p) {
		// arguments
		for (int i = 0; i < arguments.size(); i++) {
			dataToReg(p, arguments[i], args[i], out);
		}
		out << "call " << f->mappedName << endl;
		// store result
		if (retValue != "") {
			out << "mov %rax, -" << varToOffset(p, retValue) << "(%rbp)" << endl;
		}
	}
};
struct Declare : public Command {
	Declare(const vector<string> & rem, Program & p) : Command(p) {
		if (rem.size() > 3) ABORT("DECLARE needs at most 2 values");
		if (rem[1][0] != '$' || rem[1].size() <= 1) ABORT("variable identifier invalid");
		if (p.vars.find(rem[1]) != p.vars.end()) ABORT("Variable already declared");
		if (p.vars.size() >= 1000) ABORT("Too many variables declared");
		bool array = false;
		int sz = 1;
		if (rem.size() == 3) {
			array = true;
			sz = strtoll(rem[2].c_str(), nullptr, 0);
			if (sz < 1 || sz > 1024) {
				ABORT("Array size out of range");
			}
		}
		int c = p.vars.size();
		p.vars[rem[1]] = {c * 8 + sz * 8, array, sz};
	}
	void writeCode(ostream & out, Program & p) {
	}
};
struct Label : public Command {
	string lbl;
	Label(const vector<string> & rem, Program & p) : Command(p) {
		if (rem.size() != 2) ABORT("LABEL needs exactly one value");
		if (!isValid(rem[1])) ABORT("Label invalid");
		lbl = rem[1];
		if (p.labels.find(lbl) != p.labels.end()) ABORT("Label declared twice");
		p.labels.insert(lbl);
	}
	void writeCode(ostream & out, Program & p) {
		out << "lbl" << lbl << ":" << endl;
	}
};
struct Goto : public Command {
	string lbl;
	Goto(const vector<string> & rem, Program & p) : Command(p) {
		if (rem.size() != 2) ABORT("Goto needs exactly one value");
		if (!isValid(rem[1])) ABORT("Goto label invalid");
		lbl = rem[1];
	}
	void writeCode(ostream & out, Program & p) {
		if (p.labels.find(lbl) == p.labels.end()) ABORT("Label " + lbl + " not declared");
		out << "jmp lbl" << lbl << endl;
	}
};
struct Bin : public Command {
	string b1, b2, target, cmd;
	Bin(const vector<string> & rem, Program & p, const string & c) : Command(p) {
		if (rem.size() != 4) ABORT("Binary ops needs exactly three values");
		b1 = rem[1];
		b2 = rem[2];
		target = rem[3];
		cmd = c;
	}
	void writeCode(ostream & out, Program & p) {
		dataToReg(p, b1, "%rax", out);
		dataToReg(p, b2, "%rbx", out);
		out << cmd << " %rbx, %rax" << endl;
		out << "mov %rax, -" << varToOffset(p, target) << "(%rbp)" << endl;
	}
};
struct Div : public Command {
	string b1, b2, target;
	Div(const vector<string> & rem, Program & p) : Command(p) {
		if (rem.size() != 4) ABORT("Binary ops needs exactly three values");
		b1 = rem[1];
		b2 = rem[2];
		target = rem[3];
	}
	void writeCode(ostream & out, Program & p) {
		dataToReg(p, b1, "%rax", out);
		dataToReg(p, b2, "%rbx", out);
		out << "cqo" << endl; // sign extend rax to rdx
		out << "idiv %rbx" << endl;
		out << "mov %rax, -" << varToOffset(p, target) << "(%rbp)" << endl;
	}
};

// ex.: IF $a NE 7 GOTO label
struct If : public Command {
	string left, right, cmp;
	string lbl;
	If(const vector<string> & rem, Program & p) : Command(p) {
		if (rem.size() != 6) ABORT("IF invalid usage: IF <left> <op> <right> GOTO <label>");
		if (rem[rem.size() - 2] != "GOTO") ABORT("IF needs to end with GOTO <label>");
		lbl = rem[rem.size() - 1];
		left = rem[1];
		cmp = rem[2];
		right = rem[3];
		transform(cmp.begin(), cmp.end(), cmp.begin(), ::tolower);
		if (cmp != "e" && cmp != "ne"
		 && cmp != "l" && cmp != "le"
		 && cmp != "g" && cmp != "ge") ABORT("IF with invalid comparison");
	}
	void writeCode(ostream & out, Program & p) {
		dataToReg(p, left, "%rax", out);
		dataToReg(p, right, "%rbx", out);
		out << "cmp %rbx, %rax" << endl;
		if (p.labels.find(lbl) == p.labels.end()) ABORT("Label " + lbl + " not declared");
		out << "j" << cmp << " lbl" << lbl << endl;
	}
};

int main(int argc, char ** argv) {
	if (argc != 3) {
		cerr << "Need exactly two arguments" << endl;
		return 1;
	}
	
	ip = getIPAddress();
	
	// fill functions
	_functions.resize(256);
	_functions[getId("WRITECHAR")] = {"putchar@PLT", 1, false};
	_functions[getId("STRNCMP")] = {"strncmp@PLT", 3, true};
	_functions[getId("FCLOSE")] = {"fclose@PLT", 1, false};
	
	// experimental, untested FIXME: needs review
	_functions[getId("FOPEN")] = {"fopen@PLT", 1, true};
	_functions[getId("READCHAR")] = {"fgetc@PLT", 1, true};

	// Parse program
	ifstream inFile;
	inFile.open(argv[1]);
	string line;
	Program p;
	while (getline(inFile, line)) {
		if (line.size() >= 1 && line[0] == '#') continue; // comment
		vector<string> ls = split(line);
		if (ls.size() != 0) {
			string cmd = ls[0];
			if (cmd == "CALL") {
				p._code.push_back(new Call(ls, p));
			} else if (cmd == "DECLARE") {
				p._code.push_back(new Declare(ls, p));
			} else if (cmd == "IF") {
				p._code.push_back(new If(ls, p));
			} else if (cmd == "LABEL") {
				p._code.push_back(new Label(ls, p));
			} else if (cmd == "GOTO") {
				p._code.push_back(new Goto(ls, p));
			} else if (cmd == "ADD") {
				p._code.push_back(new Bin(ls, p, "add"));
			} else if (cmd == "SUB") {
				p._code.push_back(new Bin(ls, p, "sub"));
			} else if (cmd == "MUL") {
				p._code.push_back(new Bin(ls, p, "imul"));
			} else if (cmd == "DIV") { // because div is special..
				p._code.push_back(new Div(ls, p));
			} else {
				cerr << "Command " << cmd << " unknown" << endl;
				return 1;
			}
		}
	}
	
	// write assembler
	ofstream file(argv[2], ofstream::out);
	p.writeCode(file);
	file.close();
	
	for (int i = 0; i < p._code.size(); i++) {
		delete p._code[i];
	}
}

