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
	int type;
};

enum class Hcpl_TokenType {
	Const, String, Iden, Oper, ExprEnd,
	BrkSt, BrkEd, Keyword, 
};

enum class Hcpl_Oper {
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
	Eq, Ne,
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
	Scope
};

#define Hcpl_operNum	44
extern const int Hcpl_OperWeight[];

enum class Hcpl_Keyword {
	If, Else, While, For, Switch, Case, Break, Continue, Return, Class, FuncDef, VarDef, EnumDef, Override, 
};

#define Hcpl_keywordNum	14
extern const char *Hcpl_keywordStr[];

enum class Hcpl_BrkType {
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
			Hcpl_Oper id;
			// the weight of the operator represented by ID
			int weight;
		} opInfo;
		Hcpl_Keyword kwId;
		struct {
			u32 pir;
			Hcpl_BrkType type;
		} brkInfo;
	};
	std::string strData;
};

int Hasm_tokenize(const std::string &str, std::vector<Hcpl_Token> &tokens);