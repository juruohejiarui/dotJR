#include "tokenize.hpp"
#include <cstring>
#include <stack>

const int Hcpl_operWeight[] = {
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
	15,
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

std::string BsData::toString() {
	static char buf[64];
	switch (type) {
		case BsData_Type_u8: sprintf(buf, "u8:%#04x", u8Data); break; 
		case BsData_Type_i8: sprintf(buf, "i8:%d,\'%c\'", i8Data, i8Data); break;
		case BsData_Type_u16: sprintf(buf, "u16:%#06x", u16Data); break;
		case BsData_Type_i16: sprintf(buf, "i16:%d", i16Data); break;
		case BsData_Type_u32: sprintf(buf, "u32:%#010x", u32Data); break;
		case BsData_Type_i32: sprintf(buf, "i32:%d", i32Data); break;
		case BsData_Type_u64: sprintf(buf, "u64:%#018llx", u64Data); break;
		case BsData_Type_i64: sprintf(buf, "i64:%lld", i64Data); break;
		case BsData_Type_f32: sprintf(buf, "f32:%f", f32Data); break;
		case BsData_Type_f64: sprintf(buf, "f64:%lf", f64Data); break;
		default: sprintf(buf, "<Not base data>"); break;
	}
	return std::string(buf);
}

int Hcpl_tokenize(const std::string &str, std::vector<Hcpl_Token> &tokens) {
	tokens.clear();
	std::stack<size_t> brkStk;
	int lineId = 1;
	for (int l = 0, r = 0; l < str.size(); l = ++r) {
		if (str[l] == '\n') lineId++;
		if (isSeparator(str[l])) continue;
		Hcpl_Token tk;
		memset(&tk.data, 0, sizeof(BsData));
		tk.lineId = lineId;
		if (isLetter(str[l]) || str[l] == '#') {
			while (r + 1 < str.size() && (isLetter(str[r + 1]) || isNumber(str[r + 1]) || str[r + 1] == '#')) r++;
			auto subStr = str.substr(l, r - l + 1);
			tk.strData = subStr;
			int kwId = getKwId(subStr);
			if (kwId != -1) {
				tk.kwType = (KeywordType)kwId;
				tk.type = Hcpl_TokenType::Keyword;
			} else tk.type = Hcpl_TokenType::Iden;
		} else if (isNumber(str[l])) {
			while (r + 1 < str.size() && (isLetter(str[r + 1] || isNumber(str[r + 1]) || str[r + 1] == '.'))) r++;
			u64 data = 0; u8 type = 0;
			tk.strData = str.substr(l, r - l + 1);
			tk.type = Hcpl_TokenType::Const;
			Lib_readData(tk.strData.c_str(), &tk.data.u64Data, &tk.data.type);
			if (type == BsData_Type_void) {
				printf("line %d: invalid syntax on \"%s\"", lineId, tk.strData.c_str());
				return Res_Error;
			}
		} else {
			switch (str[l]) {
				case '=' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') {
						if (str[l + 2] == '=') tk.opInfo.type = OperType::Oeq, r += 2;
						else tk.opInfo.type = OperType::Eq, r++;
					} else tk.opInfo.type = OperType::Ass;
					break;
				case '+' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '+') tk.opInfo.type = OperType::SInc, r++;
					else if (str[l + 1] == '=') tk.opInfo.type = OperType::AddAss, r++;
					else tk.opInfo.type = OperType::Add;
					break;
				case '-' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '-') tk.opInfo.type = OperType::SDec, r++;
					else if (str[l + 1] == '=') tk.opInfo.type = OperType::SubAss, r++;
					else tk.opInfo.type = OperType::Sub;
					break;
				case '*' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.type = OperType::MulAss, r++;
					else tk.opInfo.type = OperType::Mul;
					break;
				case '/' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.type = OperType::DivAss, r++;
					// a line comment
					else if (str[l + 1] == '/') {
						for (r = l + 1; r + 1 < str.size() && str[r + 1] != '\n'; r++) ;
						continue;
					}
					else tk.opInfo.type = OperType::Div;
					break;
				case '%' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.type = OperType::ModAss, r++;
					else tk.opInfo.type = OperType::Mod;
					break;
				case '!' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') {
						if (str[l + 2] == '=') tk.opInfo.type = OperType::One, r += 2;
						else tk.opInfo.type = OperType::Ne, r++;
					} else tk.opInfo.type = OperType::Lnot;
					break;
				case '<' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '<') {
						if (str[l + 2] == '=') tk.opInfo.type = OperType::ShlAss, r += 2;
						else tk.opInfo.type = OperType::Shl, r++;
					} else if (str[l + 1] == '=') tk.opInfo.type = OperType::Le, r++;
					else if (str[l + 1] == '$') tk.type = Hcpl_TokenType::BrkSt, tk.brkInfo.type = BrkType::GenericL, r++;
					else tk.opInfo.type = OperType::Ls;
					break;
				case '$' :
					if (str[l + 1] == '>') {
						tk.type = Hcpl_TokenType::BrkEd, tk.brkInfo.type = BrkType::GenericR, r++;
					} else {
						tk.type = Hcpl_TokenType::Oper;
						tk.opInfo.type = OperType::New;
					}
					break;
				case '>' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '>') {
						if (str[l + 2] == '=') tk.opInfo.type = OperType::ShrAss, r += 2;
						else tk.opInfo.type = OperType::Shr, r++;
					} else if (str[l + 1] == '=') tk.opInfo.type = OperType::Ge, r++;
					else tk.opInfo.type = OperType::Gt;
					break;
				case '|' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.type = OperType::OrAss, r++;
					else if (str[l + 1] == '|') tk.opInfo.type = OperType::Lor, r++;
					else tk.opInfo.type = OperType::Or;
					break;
				case '&' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.type = OperType::AndAss, r++;
					else if (str[l + 1] == '&') tk.opInfo.type = OperType::Land, r++;
					else tk.opInfo.type = OperType::And;
					break;
				case '^' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == '=') tk.opInfo.type = OperType::XorAss, r++;
					else tk.opInfo.type = OperType::Xor;
					break;
				case '~' :
					tk.type = Hcpl_TokenType::Oper;
					tk.opInfo.type = OperType::Not;
					break;
				case ',' :
					tk.type = Hcpl_TokenType::Oper;
					tk.opInfo.type = OperType::Comma;
					break;
				case '{' :
					tk.type = Hcpl_TokenType::BrkSt;
					tk.brkInfo.type = BrkType::LargeL;
					break;
				case '}' :
					tk.type = Hcpl_TokenType::BrkEd;
					tk.brkInfo.type = BrkType::LargeR;
					break;
				case '[' :
					tk.type = Hcpl_TokenType::BrkSt;
					tk.brkInfo.type = BrkType::MediumL;
					break;
				case ']' :
					tk.type = Hcpl_TokenType::BrkEd;
					tk.brkInfo.type = BrkType::MediumR;
					break;
				case '(' :
					tk.type = Hcpl_TokenType::BrkSt;
					tk.brkInfo.type = BrkType::SmallL;
					break;
				case ')' :
					tk.type = Hcpl_TokenType::BrkEd;
					tk.brkInfo.type = BrkType::SmallR;
					break;
				case ';' :
					tk.type = Hcpl_TokenType::ExprEnd;
					break;
				case ':' :
					tk.type = Hcpl_TokenType::Oper;
					if (str[l + 1] == ':') tk.opInfo.type = OperType::Scope, r++;
					else tk.opInfo.type = OperType::Cvt;
					break;
				default :
					printf("line %d: invalid symbol %c\n", lineId, str[l]);
					return Res_Error;
			}
			tk.strData = str.substr(l, r - l + 1);
			switch (tk.type) {
				case Hcpl_TokenType::Oper :
					tk.opInfo.weight = Hcpl_operWeight[(int)tk.opInfo.type];
					break;
				case Hcpl_TokenType::BrkSt :
					brkStk.push(tokens.size());
					break;
				case Hcpl_TokenType::BrkEd :
					if (brkStk.empty() || (int)tokens[brkStk.top()].brkInfo.type != ((int)tk.brkInfo.type & ~1)) {
						printf("line %d: can not match bracket %c", lineId, str[l]);
						return Res_SeriousError;
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