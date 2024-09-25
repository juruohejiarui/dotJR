#include "tokenize.hpp"
#include <cstring>
#include <stack>

const int Hcpl_OperWeight[] = {
	1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3,
	4,
	5,
	6,
	7,
	8, 8, 8, 8,
	9, 9, 9, 9,
	10, 10,
	11, 11,
	12, 12, 12,
	13, 13, 13, 13, 13, 13, 13, 13, 13,
	14, 14, 14, 14,
	15
};

const char *Hcpl_keywordStr[] = {
	"if", "else", "while", "for", "switch", "case", "break", "continue", "return", "using", "namespace", "class", "func", "var", "enum", "public", "protected", "private", "override", "fixed"
};

static int getKwId(const std::string &str) {
	for (int i = 0; i < Hcpl_keywordNum; i++) if (str == Hcpl_keywordStr[i]) return i;
	return -1;
}

BsData calcConst(const BsData &x, const BsData &y) {
	u8 tgrType;
	return BsData();
}

int Hcpl_tokenize(const std::string &str, std::vector<Hcpl_Token> &tokens)
{
	tokens.clear();
	std::stack<size_t> brkStk;
	int lineId = 1;
	for (int l = 0, r = 0; l < str.size(); l = ++r) {
		if (str[l] == '\n') lineId++;
		if (isSeparator(str[l])) continue;
		Hcpl_Token tk;
		memset(&tk.data, 0, sizeof(BsData));
		tk.lineId = lineId;
		if (isLetter(str[l])) {
			while (r + 1 < str.size() && (isLetter(str[r + 1]) || isNumber(str[r + 1]))) r++;
			auto subStr = str.substr(l, r - l + 1);
			tk.strData = subStr;
			if (subStr == "new") {
				tk.type	= Hcpl_TokenType::Oper;
				tk.opInfo.id = Hcpl_Oper::New;
			} else {
				int kwId = getKwId(subStr);
				if (kwId != -1) {
					tk.kwId = (Hcpl_Keyword)kwId;
					tk.type = Hcpl_TokenType::Keyword;
				} else tk.type = Hcpl_TokenType::Iden;
			}
		} else if (isNumber(str[l])) {
			while (r + 1 < str.size() && (isLetter(str[r + 1] || isNumber(str[r + 1]) || str[r + 1] == '.'))) r++;
			u64 data = 0; u8 type = 0;
			tk.strData = str.substr(l, r - l + 1);
			Lib_readData(tk.strData.c_str(), &tk.data.u64Data, &tk.data.type);
			if (type == BsData_Type_void) {
				printf("line %d: invalid syntax on \"%s\"", lineId, tk.strData.c_str());
				return 1;
			}
		} else {
			switch (str[l]) {
				case '=' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') {
						if (str[l + 2] == '=') tk.opInfo.id = Hcpl_Oper::Oeq, r += 2;
						else tk.opInfo.id = Hcpl_Oper::Eq, r++;
					} else tk.opInfo.id = Hcpl_Oper::Ass;
					break;
				case '+' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '+') tk.opInfo.id = Hcpl_Oper::SInc, r++;
					else if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::AddAss, r++;
					else tk.opInfo.id = Hcpl_Oper::Add;
					break;
				case '-' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '-') tk.opInfo.id = Hcpl_Oper::SDec, r++;
					else if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::SubAss, r++;
					else tk.opInfo.id = Hcpl_Oper::Sub;
					break;
				case '*' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::MulAss, r++;
					else tk.opInfo.id = Hcpl_Oper::Mul;
					break;
				case '/' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::DivAss, r++;
					else tk.opInfo.id = Hcpl_Oper::Div;
					break;
				case '%' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::ModAss, r++;
					else tk.opInfo.id = Hcpl_Oper::Mod;
					break;
				case '!' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') {
						if (str[l + 2] == '=') tk.opInfo.id = Hcpl_Oper::One, r += 2;
						else tk.opInfo.id = Hcpl_Oper::Ne, r++;
					} else tk.opInfo.id = Hcpl_Oper::Lnot;
					break;
				case '<' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '<') {
						if (str[l + 2] == '=') tk.opInfo.id = Hcpl_Oper::ShlAss, r += 2;
						else tk.opInfo.id = Hcpl_Oper::Shl, r++;
					} else if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::Le, r++;
					else if (str[l + 1] == '$') tk.type = Hcpl_TokenType::BrkSt, tk.brkInfo.type = Hcpl_BrkType::GenericL, r++;
					else tk.opInfo.id = Hcpl_Oper::Ls;
					break;
				case '$' :
					if (str[l + 1] == '>') {
						tk.type = Hcpl_TokenType::BrkEd, tk.brkInfo.type = Hcpl_BrkType::GenericR, r++;
					} else {
						printf("line %d: invalid symbol $\n", tk.lineId);
						return 1;
					}
					break;
				case '>' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '>') {
						if (str[l + 2] == '=') tk.opInfo.id = Hcpl_Oper::ShrAss, r += 2;
						else tk.opInfo.id = Hcpl_Oper::Shr, r++;
					} else if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::Ge, r++;
					else tk.opInfo.id = Hcpl_Oper::Gt;
					break;
				case '|' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::OrAss, r++;
					else if (str[l + 1] == '|') tk.opInfo.id = Hcpl_Oper::Lor, r++;
					else tk.opInfo.id = Hcpl_Oper::Or;
					break;
				case '&' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::AndAss, r++;
					else if (str[l + 1] == '&') tk.opInfo.id = Hcpl_Oper::Land, r++;
					else tk.opInfo.id = Hcpl_Oper::And;
					break;
				case '^' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.id = Hcpl_Oper::XorAss, r++;
					else tk.opInfo.id = Hcpl_Oper::Xor;
					break;
				case '~' :
					tk.type = Hcpl_TokenType::Oper;
					tk.opInfo.id = Hcpl_Oper::Not;
					break;
				case ',' :
					tk.type = Hcpl_TokenType::Oper;
					tk.opInfo.id = Hcpl_Oper::Comma;
					break;
				case '{' :
					tk.type = Hcpl_TokenType::BrkSt;
					tk.brkInfo.type = Hcpl_BrkType::LargeL;
					break;
				case '}' :
					tk.type = Hcpl_TokenType::BrkEd;
					tk.brkInfo.type = Hcpl_BrkType::LargeR;
					break;
				case '[' :
					tk.type = Hcpl_TokenType::BrkSt;
					tk.brkInfo.type = Hcpl_BrkType::MediumL;
					break;
				case ']' :
					tk.type = Hcpl_TokenType::BrkEd;
					tk.brkInfo.type = Hcpl_BrkType::MediumR;
					break;
				case '(' :
					tk.type = Hcpl_TokenType::BrkSt;
					tk.brkInfo.type = Hcpl_BrkType::SmallL;
					break;
				case ')' :
					tk.type = Hcpl_TokenType::BrkEd;
					tk.brkInfo.type = Hcpl_BrkType::SmallR;
					break;
				case ';' :
					tk.type = Hcpl_TokenType::ExprEnd;
					break;
				case ':' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == ':') tk.opInfo.id = Hcpl_Oper::Scope, r++;
					else tk.opInfo.id = Hcpl_Oper::Cvt;
					break;
				default :
					printf("line %d: invalid symbol %c\n", lineId, str[l]);
					return 1;
			}
			tk.strData = str.substr(l, r - l + 1);
			switch (tk.type) {
				case Hcpl_TokenType::Oper :
					tk.opInfo.weight = Hcpl_OperWeight[(int)tk.opInfo.id];
					break;
				case Hcpl_TokenType::BrkSt :
					brkStk.push(tokens.size());
					break;
				case Hcpl_TokenType::BrkEd :
					if (brkStk.empty() || (int)tokens[brkStk.top()].brkInfo.type != ((int)tk.brkInfo.type & ~1)) {
						printf("line %d: can not match bracket %c", lineId, str[l]);
						break;
					} 
					tokens[brkStk.top()].brkInfo.pir = tokens.size();
					tk.brkInfo.pir = brkStk.top();
					brkStk.pop();
					break;
			}
		}
		tokens.push_back(tk);
	}
	return 0;
}