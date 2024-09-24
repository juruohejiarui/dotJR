#include "tokenize.hpp"
#include "../lib/bstype.hpp"
#include <iostream>

const char *Hasm_defineTypeStr[] = { "byte", "word", "dword", "quad", "string", "fill", "func", "global", "enddefine" };

static int getCmdId(const std::string &cmdStr) {
	for (int i = 0; i < HInst_cmdNum; i++) if (cmdStr == HInst_cmdStr[i]) return i;
	return -1;
}
static int getBsdataType(const std::string &typeStr) {
	for (int i = 0; i < Lib_bsdataTypeNum; i++) if (typeStr == Lib_bsdataTypeStr[i]) return i;
	return -1;
}
static int getDefineType(const std::string &defTypeStr) {
	for (int i = 0; i < Hasm_defineTypeNum; i++) if (defTypeStr == Hasm_defineTypeStr[i]) return i;
	return -1;
}
bool Hasm_tokenize(const std::string &str, std::vector<Hasm_Token> &tokens) {
	tokens.clear();
	int lineId = 1;
	int error = 0;
	const char *cstr = str.c_str();
	for (int l = 0, r = 0, strSize = str.size(); l < strSize; l = ++r) {
		if (str[l] == '\n') lineId++;
		if (Lib_isSeparator(str[l])) continue;
		Hasm_Token tk;
		tk.lineId = lineId;
		tk.data = 0, tk.dataType = 0;
		if (Lib_isNumber(str[l])) {
			while (r + 1 < strSize && (Lib_isNumber(str[r + 1]) || Lib_isLetter(str[r + 1]))) r++;
			tk.type = Hasm_TokenType::BsData;
			Lib_readData(cstr + l, &tk.data, &tk.dataType);
			if (tk.dataType == BsData_Type_generic) {
				printf("line %d: invalid syntax on \"%s\"", lineId, str.substr(l, r - l + 1));
				error = 1;
			}
		} else if (Lib_isLetter(str[l])) {
			while (r + 1 < strSize && (Lib_isNumber(str[r + 1]) || Lib_isLetter(str[r + 1]))) r++;
			const auto subStr = str.substr(l, r - l + 1);
			if (r + 1 < strSize && str[r + 1] == ':') {
				tk.strData = subStr;
				tk.type = Hasm_TokenType::Label;
			} else {
				tk.strData = subStr;
				tk.type = Hasm_TokenType::StrData;
				tk.dataType = getBsdataType(subStr) + 1;
				tk.data = getCmdId(subStr) + 1;
			}
		} else if (str[l] == '.') {
			r++;
			while (r + 1 < strSize && (Lib_isNumber(str[r + 1]) || Lib_isLetter(str[r + 1]))) r++;
			const auto subStr = str.substr(l + 1, r - l);
			int defTypeId = getDefineType(subStr);
			if (defTypeId == -1) {
				printf("line %d: invalid define type \"%s\"", lineId, subStr.c_str());
				error = 1;
			}
			tk.type = Hasm_TokenType::Define;
			tk.data = defTypeId;
		} else if (str[l] == '\"') {
			r++;
			while (r + 1 < strSize && str[r] != '\"') {
				if (str[r] == '\\') r++;
				r++;
			}
			tk.type = Hasm_TokenType::String;
			tk.strData = Lib_getRealStr(str.substr(l + 1, r - l - 1));
		} else if (str[l] == ',') {
			tk.type = Hasm_TokenType::Comma;
		}
		tokens.push_back(tk);
	}
	return true;
}