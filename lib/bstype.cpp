#include "bstype.hpp"
std::string Lib_getRealStr(const std::string &str) {
	std::string res;
	for (int i = 0, strSize = str.size(); i < strSize; i++) {
		if (str[i] == '\\') {
			switch (str[i + 1]) {
				case 'n' : res.push_back('\n'); break;
				case 'r' : res.push_back('\r'); break;
				case '0' : res.push_back('\0'); break;
				case 't' : res.push_back('\t'); break;
				case 'b' : res.push_back('\b'); break;
				default : res.push_back(str[i + 1]); break;
			}
			i++;
		} else res.push_back(str[i]);
	}
	return res;
}

std::string Lib_getCodeStr(const std::string &str) {
	std::string res;
	for (int i = 0, strSize = str.size(); i < strSize; i++) {
		switch (str[i]) {
			case '\n' : res.append("\\n"); break;
			case '\r' : res.append("\\r"); break;
			case '\0' : res.append("\\0"); break;
			case '\t' : res.append("\\t"); break;
			case '\b' : res.append("\\b"); break;
			default : res.push_back(str[i]); break;
		}
	}
	res.push_back('\0');
	return res;
}
