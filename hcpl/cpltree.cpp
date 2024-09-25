#include "cpltree.hpp"
#include <cstring>
#include <stack>

static int parseKeyword(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root, CplNode *father);

static int parseUsing(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	UsingNode *node = new UsingNode(); root = node, node->type = Hcpl_CplNodeType::Using;
	node->token = tokens[fr];
	for (to = fr + 1; tokens[to].type != Hcpl_TokenType::ExprEnd; to++) {
		if (tokens[to].type == Hcpl_TokenType::Iden) {
			if (to - 1 != fr && !isSpecOper(tokens[to - 1], Hcpl_Oper::Scope))
				goto SyntaxError;
			node->path.push_back(tokens[to].strData);
		} else if (!isSpecOper(tokens[to], Hcpl_Oper::Scope) || tokens[to - 1].type != Hcpl_TokenType::Iden)
			goto SyntaxError;
	}
	return 0;
	SyntaxError:
	printf("line %d: invalid content of \"using\"", tokens[to].lineId);
	return 0x2;
}

static __always_inline int parseFunc(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	
	return 0;
}

static __always_inline int parseNsp(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	NspNode *node = new NspNode(); root = node, node->type = Hcpl_CplNodeType::NspDef;
	int res = 0;
	node->token = tokens[fr + 1];
	for (to = fr + 1; tokens[to].type == Hcpl_TokenType::Iden || isSpecOper(tokens[to], Hcpl_Oper::Scope); to++) {
		if (tokens[to].type == Hcpl_TokenType::Iden) {
			if (to - 1 != fr && !isSpecOper(tokens[to - 1], Hcpl_Oper::Scope))
				goto SyntaxError;
			node->path.push_back(tokens[to].strData);
		} else if (!isSpecOper(tokens[to], Hcpl_Oper::Scope)) goto SyntaxError;
	}
	if (!isSpecBrk(tokens[to], Hcpl_BrkType::LargeL)) goto SyntaxError;
	for (size_t l = to + 1, r = l; r < tokens[to].brkInfo.pir; l = ++r) {
		CplNode *child = nullptr;
		switch (tokens[l].type) {
			case Hcpl_TokenType::Keyword :
				res |= parseKeyword(tokens, l, r, child, root);
				if (res & 0x2 || child == nullptr) break;
				switch (child->type) {
					case Hcpl_CplNodeType::FuncDef :	node->func.push_back(child); break;
					case Hcpl_CplNodeType::VarDef : 	node->var.push_back(child); break;
					case Hcpl_CplNodeType::EnumDef : 	node->enm.push_back(child); break;
					case Hcpl_CplNodeType::ClsDef : 	node->cls.push_back(child); break;
				}
				break;
			case Hcpl_TokenType::ExprEnd :
				continue;
			default :
				printf("line %d: invalid token \"%s\"\n", tokens[l].lineId, tokens[l].strData.c_str());
				res |= 0x2;
				break;
		}
		if (res & 0x2) return 0x2;
	}
	to = tokens[to].brkInfo.pir;
	return 0;
	SyntaxError:
	printf("line %d: invalid content of \"namespace\"\n", tokens[to].lineId);
	return 0x2;
}

static int parseKeyword(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root, CplNode *father) {
	int errorPos = to;
	switch (tokens[fr].kwId) {
		case Hcpl_Keyword::Using:
			return parseUsing(tokens, fr, to, root);
		case Hcpl_Keyword::Namespace :
			if (father->type != Hcpl_CplNodeType::SrcRoot && father->type != Hcpl_CplNodeType::SymRoot)
				goto SyntaxError;
			return parseNsp(tokens, fr, to, root);
		case Hcpl_Keyword::FuncDef :
			if (father->type != Hcpl_CplNodeType::SrcRoot && father->type != Hcpl_CplNodeType::SymRoot
				 && father->type != Hcpl_CplNodeType::ClsDef && father->type != Hcpl_CplNodeType::NspDef)
				goto SyntaxError;
			return parseFunc(tokens, fr, to, root);
	}
	return 0;
	SyntaxError:
	printf("line %d: invalid position of keyword \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return 0x2;
}

int Hcpl_makeCplTree(const std::vector<Hcpl_Token> &tokens, Hcpl_CplNodeType rootType, CplNode *&root) {
	root = new BlkNode(); 
	BlkNode *node = new BlkNode(); root = node; node->type = Hcpl_CplNodeType::SrcRoot;
	CplNode *child;
	int res = 0;
	for (size_t fr = 0, to = 0; fr < tokens.size(); fr = ++to) {
		switch (tokens[fr].type) {
			case Hcpl_TokenType::Keyword :
				res |= parseKeyword(tokens, fr, to, child, root);
				node->child.push_back(child);
				break;
			case Hcpl_TokenType::ExprEnd :
				continue;
			default :
				printf("line %d: invalid token \"%s\"\n", tokens[fr].lineId, tokens[fr].strData.c_str());
				res |= 0x2;
				break;
		}
		if (res & 0x2) break;
	}
	return res;
}
