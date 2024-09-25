#include "tokenize.hpp"

const int Hcpl_OperWeight[] = {
	1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3,
	4,
	5,
	6,
	7,
	8, 8,
	9, 9, 9, 9,
	10, 10,
	11, 11,
	12, 12, 12,
	13, 13, 13, 13, 13, 13, 13, 13, 13,
	14, 14, 14, 14,
	15
};

const char *Hcpl_keywordStr[] = {
	"if", "else", "while", "for", "switch", "case", "break", "continue", "return", "class", "func", "var", "enum", "override"
};

int Hasm_tokenize(const std::string &str, std::vector<Hcpl_Token> &tokens) {
	return 1;
}