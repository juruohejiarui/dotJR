#include "tokenize.hpp"
#include <cstring>
#include <stack>
#include <format>

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

BsData calcConst(OperType oper, const BsData &x, const BsData &y, bool hasL, bool hasR) {
	u8 tgrType;
	BsData a, b;
	auto maxType = [](u8 type1, u8 type2) {
		// if both are integer, then get the larger size
		if (Lib_BsData_isInt(type1) && Lib_BsData_isInt(type2)) {
			return (type1 & 3) > (type2 & 3) ? type1 : type2;
		// if one of them is float, then get the larger float size
		} else return std::max(type1, type2);
	};
	auto cvtTo = [](const BsData &data, u8 tgrType) {
		if (data.type == tgrType) return data;
		else {
			BsData res;
			res.type = tgrType;
			if (Lib_BsData_isFloat(tgrType)) {
				f64 f64Data = data.toF64();
				if (tgrType == BsData_Type_f32) res.f32Data = (f32)f64Data;
				else res.f64Data = f64Data;
			} else if (Lib_BsData_isSign(tgrType)) {
				res.i64Data = data.toI64();
			} else res.u64Data = data.toU64();
			return res;
		}
	};
	if ((hasL && !isConst(x)) || (hasR && !isConst(y))) return BsData();
	// if there is two operands, then convert them to the larger type
	if (hasL && hasR) {
		tgrType = maxType(x.type, y.type);
		a = cvtTo(x, tgrType), b = cvtTo(y, tgrType);
	} else (hasL ? (tgrType = x.type, a = x) : (tgrType = y.type, b = y));
	switch (oper) {
		
	}
	return BsData();
}

std::string BsData::toString() const {
	switch (type) {
		case BsData_Type_u8: return std::format("u8:{0:#02x}", u8Data);
		case BsData_Type_i8: return std::format("i8:{0:d},{0}", i8Data);
		case BsData_Type_u16: return std::format("u16:{0:#04x}", u16Data);
		case BsData_Type_i16: return std::format("i16:{0:d}", i16Data);
		case BsData_Type_u32: return std::format("u32:{0:#08x}", u32Data);
		case BsData_Type_i32: return std::format("i32:{0:d}", i32Data);
		case BsData_Type_u64: return std::format("u64:{0:#016x}", u64Data);
		case BsData_Type_i64: return std::format("i64:{0:d}", i64Data);
		case BsData_Type_f32:  return std::format("f32:{0}", f32Data);
		case BsData_Type_f64:  return std::format("f64:{0}", f64Data);
		default: return std::string("<Not Base Data>");
	}
}

u64 BsData::toU64() const {
	if (type == BsData_Type_f32 || type == BsData_Type_f64) return (u64)toF64(); 
	return u64Data;
}

i64 BsData::toI64() const {
	if ((type & 4) && type < BsData_Type_f32) return (i64)u64Data;
	switch (type) {
		case BsData_Type_i8 : return (i64)i8Data;
		case BsData_Type_i16 : return (i64)i16Data;
		case BsData_Type_i32 : return (i64)i32Data;
		case BsData_Type_i64 : return i64Data;
		case BsData_Type_f32 : return (i64)f32Data;
		case BsData_Type_f64 : return (i64)f64Data;
		default : return BsData_Type_void;
	}
}

f64 BsData::toF64() const {
	switch (type) {
		case BsData_Type_u8:
		case BsData_Type_u16 :
		case BsData_Type_u32 :
		case BsData_Type_u64 :
			return (f64)u64Data;
		case BsData_Type_i8 :
		case BsData_Type_i16 :
		case BsData_Type_i32 :
		case BsData_Type_i64 :
			return (f64)toI64();
		case BsData_Type_f32 : return (f64)f32Data;
		case BsData_Type_f64 : return f64Data;
		default : return BsData_Type_void;
	}
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
			while (r + 1 < str.size() && (isLetter(str[r + 1]) || isNumber(str[r + 1]) || str[r + 1] == '.')) r++;
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