#pragma once

#include "../lib/bstype.hpp"
#include <vector>

struct BsData {
	union {
		u64 u64Data;
		i64 i64Data;
		u32 u32Data;
		i32 i32Data;
		u16 u16Data;
		i16 i16Data;
		u8 u8Data;
		i8 i8Data;
		f32 f32Data;
		f64 f64Data;
	};
	u8 type;
	std::string toString();
};

BsData Hcpl_calcConst(const BsData &x, const BsData &y);

enum class Hcpl_TokenType {
	Const, String, Iden, Oper, ExprEnd,
	BrkSt, BrkEd, Keyword, 
};

enum class OperType {
	// weight 1
	Comma,
	// weight 2
	Ass, AddAss, SubAss, MulAss, DivAss, ModAss, AndAss, OrAss, XorAss, ShlAss, ShrAss,
	// weight 3
	Lor,
	// weight 4
	Land,
	// weight 5
	Or,
	// weight 6
	Xor,
	// weight 7
	And,
	// weight 8
	Eq, Ne, Oeq, One,
	// weight 9
	Gt, Ls, Ge, Le,
	// weight 10
	Shl, Shr,
	// weight 11
	Add, Sub,
	// weight 12
	Mul, Div, Mod,
	// weight 13
	New, GetAddr, GetVal, Lnot, Not, Minus, Cvt, PInc, PDec,
	// weight 14
	Idx, SInc, SDec, Mem,
	// weight 15
	Scope,
};

#define Hcpl_operNum	46
#define Hcpl_operWeightShift 100
extern const int Hcpl_operWeight[];

enum class KeywordType {
	If, Else, While, For, Switch, Case, Break, Continue, Return, Using, Namespace, Class, FuncDef, VarDef, EnumDef, Public, Protected, Private, Override, Fixed
};

#define Hcpl_keywordNum	20
extern const char *Hcpl_keywordStr[];

enum class BrkType {
	LargeL, LargeR, MediumL, MediumR, SmallL, SmallR, GenericL, GenericR 
};

struct Hcpl_Token {
	Hcpl_TokenType type;
	u32 lineId;
	union {
		BsData data;
		struct {
			// Some operators use same character(s) which is hard to distinguish during tokenization process.
			// Fortunately, most operators with same character(s) accepts different number of operands.
			// So during tokenization, the operator that accept more operands will be written to this field.
			// the special cases is ++ and --, which will be treated as suffix operators.
			OperType type;
			// the weight of the operator represented by ID
			int weight;
		} opInfo;
		KeywordType kwType;
		struct {
			size_t pir;
			BrkType type;
		} brkInfo;
	};
	std::string strData;
};

static inline bool isSpecOper(const Hcpl_Token &token, OperType opType) { return token.type == Hcpl_TokenType::Oper && token.opInfo.type == opType; }
static inline bool isSpecKw(const Hcpl_Token &token, KeywordType kwId) { return token.type == Hcpl_TokenType::Keyword && token.kwType == kwId; }
static inline bool isSpecBrk(const Hcpl_Token &token, BrkType brkType) {
	return token.type == ((int)brkType & 1 ? Hcpl_TokenType::BrkEd : Hcpl_TokenType::BrkSt) && token.brkInfo.type == brkType; 
}
static inline void skipBrk(const Hcpl_Token &token, size_t &pos) { token.type != Hcpl_TokenType::BrkSt || (pos = token.brkInfo.pir); }

int Hcpl_tokenize(const std::string &str, std::vector<Hcpl_Token> &tokens);