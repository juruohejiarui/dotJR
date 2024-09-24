#pragma once
#include <string>
#include <vector>
#include "../hinst/api.hpp"

enum class Hasm_TokenType {
	BsData, StrData, Label, String, Define, Comma
};

enum class Hasm_DefineType {
	Byte, Word, Dword, Quad, String, Fill, Func, Global, EndDefine
};

#define Hasm_defineTypeNum 9
extern const char *Hasm_defineTypeStr[];

struct Hasm_Token {
	u32 lineId;
	Hasm_TokenType type;
	struct {
		u8 dataType;
		u64 data;
	};
	std::string strData;
};

bool Hasm_tokenize(const std::string &str, std::vector<Hasm_Token> &tokens);
