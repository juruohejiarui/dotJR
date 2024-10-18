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

std::vector<std::string> split(const std::string &str, const std::string &sep) {
    std::vector<int> fil; fil.resize(sep.size() + 1);
	std::vector<std::string> res;
	fil[1] = fil[0] = 0;
	for (size_t i = 2, j = 0; i <= sep.size(); i++) {
		while (j && sep[i - 1] != sep[j]) j = fil[j];
		if (sep[i - 1] == sep[j]) j++;
		fil[i] = j;
	}
	size_t lst = 1;
	for (size_t i = 1, j = 0; i <= str.size(); i++) {
		while (j && (j == sep.size() || str[i - 1] != str[j])) j = fil[j];
		if (str[i - 1] == sep[j]) j++;
		if (j == sep.size() && i - lst + 1 >= sep.size())
			res.push_back(str.substr(lst - 1, i - lst + 1 - sep.size())),
			lst = i + 1;
	}
	if (lst <= str.size()) res.push_back(str.substr(lst - 1));
	return res;
}

std::string getIndent(int dep) { std::string str = ""; for (int i = 0; i < dep; i++) str.append("  "); return str; }
