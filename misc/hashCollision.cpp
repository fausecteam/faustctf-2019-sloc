#include <bits/stdc++.h>
using namespace std;

uint8_t getId(const string & s, string & ip) {
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

/*void tryAll(int n, uint8_t target) {
	string s(n, 'A');
	while(true) {
		if (getId(s) == target) {
			cout << s << endl;
			return; // TODO: breaking here
		}
		s[n-1]++;
		int k = n-1;
		while (k > 0 && s[k] > 'Z') {
			s[k] = 'A';
			k--;
			s[k]++;
		}
		if (k == 0 && s[k] > 'Z') return;
	}
}*/

// checks all ips starting with 10. if they have an internal collision in the func names
string toIP(vector<int> & v) {
	return to_string(v[0]) + "." + to_string(v[1]) + "." + to_string(v[2]) + "." + to_string(v[3]);
}

void checkIPCollision(vector<string> & funcs) {
	vector<int> ip = {0, 0, 0, 0};
	do {
		vector<bool> v(256);
		string ips = toIP(ip);
		for (string & s : funcs) {
			uint8_t x = getId(s, ips);
			if (v[x]) {
				cerr << "Found a collision for " << ips << " and string " << s << endl;
			}
			v[x] = true;
		}
		ip[3]++;
		int k = 3;
		while (k > 0 && ip[k] > 255) {
			ip[k] = 0;
			k--;
			ip[k]++;
		}
		if (k == 0) cout << ips << endl;
	} while (ip[0] <= 255);
}

int main() {
	vector<string> allFuncs = {"FOPEN", "FCLOSE", "STRNCMP", "READCHAR", "WRITECHAR"};
	checkIPCollision(allFuncs);
	//cout << getIPAddress() << endl;
/*	cout << (int) getId("FOPEN") << endl;
	cout << (int) getId("READCHAR") << endl;
	uint8_t target = getId("FOPEN");
	for (int i = 1; i < 7; i++) {
		tryAll(i, target);
	}
	target = getId("READCHAR");
	for (int i = 1; i < 7; i++) {
		tryAll(i, target);
	}*/
}
