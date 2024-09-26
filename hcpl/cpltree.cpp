#include "cpltree.hpp"
#include <cstring>
#include <stack>

static int parse(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root, CplNode *father);
static int parseKeyword(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root, CplNode *father);

static int parseUsing(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	UsingNode *node = new UsingNode(); root = node, node->type = CplNodeType::Using;
	node->token = tokens[fr];
	for (to = fr + 1; tokens[to].type != Hcpl_TokenType::ExprEnd; to++) {
		if (tokens[to].type == Hcpl_TokenType::Iden) {
			if (to - 1 != fr && !isSpecOper(tokens[to - 1], OperType::Scope))
				goto SyntaxError;
			node->path.push_back(tokens[to].strData);
		} else if (!isSpecOper(tokens[to], OperType::Scope) || tokens[to - 1].type != Hcpl_TokenType::Iden)
			goto SyntaxError;
	}
	return 0;
	SyntaxError:
	printf("line %d: invalid content of \"using\"", tokens[to].lineId);
	return Res_SeriousError;
}

#define setAccess(token, node) \
	do { \
		Hcpl_Token tk = (token); \
		if (tk.type != Hcpl_TokenType::Keyword) break; \
		switch (tk.kwType) { \
			case KeywordType::Private: 	node->access = IdenAccessType::Private; break; \
			case KeywordType::Public: 		node->access = IdenAccessType::Public; break; \
			case KeywordType::Protected: 	node->access = IdenAccessType::Protected; break; \
		} \
	} while (0)

static int parseType(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t to, TypeNode *&root) {
	TypeNode *node = new TypeNode(); root = node, node->type == CplNodeType::Type;
	int errorPos = 0, res = 0;
	for (; fr <= to && isSpecBrk(tokens[fr], BrkType::SmallL) && tokens[fr].brkInfo.pir == to; fr++, to--) ;
	if (fr <= to) { errorPos = fr; goto SyntaxError; }
	// check if it is an array
	if (isSpecBrk(tokens[to], BrkType::MediumR) && tokens[to].brkInfo.pir == to - 1) {
		while (isSpecBrk(tokens[to], BrkType::MediumR) && tokens[to].brkInfo.pir == to - 1 && fr < to) {
			node->attr |= TypeNode_Attr_isArr;
			node->dimc++;
			to = tokens[to].brkInfo.pir - 1;
		}
		node->token = tokens[to + 1];
		if (fr <= to) { errorPos = fr; goto SyntaxError; }
		res |= parseType(tokens, fr, to, node->subType);
	// check if it is a pointer
	} else if (isSpecOper(tokens[to], OperType::Mul)) {
		node->token = tokens[to];
		node->attr |= TypeNode_Attr_isPtr;
		if (fr <= to) { errorPos = fr; goto SyntaxError; }
		res |= parseType(tokens, fr, to - 1, node->subType);
	// check if it is a function pointer
	} else if (isSpecBrk(tokens[to], BrkType::SmallR)) {
		for (int l = tokens[to].brkInfo	.pir + 1, r = l; l < to; l = ++r) {
			
		}
	}
	// 
	return res;
	SyntaxError:
	printf("line %d: Syntax Error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static __always_inline int parseFunc(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	FuncNode *node = new FuncNode(); root = node, node->type = CplNodeType::FuncDef;
	int pre = fr, errorPos = fr, res;
	if (pre && isSpecKw(tokens[pre - 1], KeywordType::Fixed))
		node->attr |= Hcpl_DefNode_Attr_Fixed, pre--;
	if (isSpecKw(tokens[pre - 1], KeywordType::Override))
		node->attr |= Hcpl_DefNode_Attr_Override, pre--;
	if (pre) setAccess(tokens[pre - 1], node);
	node->token = tokens[fr + 1];
	if (tokens[fr + 1].type != Hcpl_TokenType::Iden) { errorPos = fr + 1; goto SyntaxError; }
	if (isSpecBrk(tokens[fr + 2], BrkType::GenericL)) {
		for (int l = fr + 3; l < tokens[fr + 2].brkInfo.pir; l += 2) {
			if (tokens[l].type != Hcpl_TokenType::Iden) { errorPos = l; goto SyntaxError; }
			CplNode *gener = new CplNode();
			gener->type = CplNodeType::Iden;
			gener->token = tokens[l];
			node->tmplList.push_back(gener);
			if (l + 1 != tokens[fr + 2].brkInfo.pir && !isSpecOper(tokens[l + 1], OperType::Comma)) {
				errorPos = l + 1; goto SyntaxError;
			}
		}
		to = tokens[fr + 2].brkInfo.pir + 1;
	} else to = fr + 2;
	if (isSpecBrk(tokens[to], BrkType::SmallL)) {
		for (int l = to + 1, r = l; l < tokens[to].brkInfo.pir; l = ++r) {
			if (tokens[l].type != Hcpl_TokenType::Iden || !isSpecOper(tokens[l + 1], OperType::Cvt)) { errorPos = l + 1; goto SyntaxError; }
			VarNode *param = new VarNode(); param->type = CplNodeType::VarDef;
			param->token = tokens[l];
			r = l + 2;
			for (r = l + 2; !isSpecOper(tokens[r], OperType::Comma) && !isSpecBrk(tokens[r], BrkType::SmallR); r++)
				if (tokens[r].type == Hcpl_TokenType::BrkSt) r = tokens[r].brkInfo.pir;
			res |= parseType(tokens, l + 2, r - 1, param->varType);
			node->paramList.push_back(param);
		}
		to = tokens[to].brkInfo.pir + 1;

	} else { errorPos = to; goto SyntaxError; }
	if (isSpecOper(tokens[to], OperType::Cvt)) {
		for (fr = ++to; !isSpecOper(tokens[to], OperType::Comma) && !isSpecBrk(tokens[to], BrkType::LargeL); to++)
			if (tokens[to].type == Hcpl_TokenType::BrkSt) to = tokens[to].brkInfo.pir;
		res |= parseType(tokens, fr, to - 1, node->retType);
	} else { errorPos = to; goto SyntaxError; }
	if (!isSpecBrk(tokens[to], BrkType::LargeL)) { errorPos = to; goto SyntaxError; }
	res |= parse(tokens, to, to, node->content, node);
	return res;
	SyntaxError:
	printf("line %d: syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static __always_inline int parseNsp(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	NspNode *node = new NspNode(); root = node, node->type = CplNodeType::NspDef;
	if (fr) setAccess(tokens[fr - 1], node);
	int res = 0;
	node->token = tokens[fr + 1];
	for (to = fr + 1; tokens[to].type == Hcpl_TokenType::Iden || isSpecOper(tokens[to], OperType::Scope); to++) {
		if (tokens[to].type == Hcpl_TokenType::Iden) {
			if (to - 1 != fr && !isSpecOper(tokens[to - 1], OperType::Scope))
				goto SyntaxError;
			node->path.push_back(tokens[to].strData);
		} else if (!isSpecOper(tokens[to], OperType::Scope)) goto SyntaxError;
	}
	if (!isSpecBrk(tokens[to], BrkType::LargeL)) goto SyntaxError;
	for (size_t l = to + 1, r = l; r < tokens[to].brkInfo.pir; l = ++r) {
		CplNode *child = nullptr;
		switch (tokens[l].type) {
			case Hcpl_TokenType::Keyword :
				res |= parseKeyword(tokens, l, r, child, root);
				if (res &Res_SeriousError || child == nullptr) break;
				switch (child->type) {
					case CplNodeType::FuncDef :	node->func.push_back((FuncNode *)child); break;
					case CplNodeType::VarDef : 	node->var.push_back((VarNode *)child); break;
					case CplNodeType::EnumDef : node->enm.push_back((EnumNode *)child); break;
					case CplNodeType::ClsDef : 	node->cls.push_back((ClsNode *)child); break;
				}
				break;
			case Hcpl_TokenType::ExprEnd :
				continue;
			default :
				printf("line %d: invalid token \"%s\"\n", tokens[l].lineId, tokens[l].strData.c_str());
				res |= Res_SeriousError;
				break;
		}
		if (res & Res_SeriousError) return Res_SeriousError;
	}
	to = tokens[to].brkInfo.pir;
	return 0;
	SyntaxError:
	printf("line %d: invalid content of \"namespace\"\n", tokens[to].lineId);
	return Res_SeriousError;
}

static int parseKeyword(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root, CplNode *father) {
	int errorPos = to;
	switch (tokens[fr].kwType) {
		case KeywordType::Using:
			return parseUsing(tokens, fr, to, root);
		case KeywordType::Namespace :
			if (father->type != CplNodeType::SrcRoot && father->type != CplNodeType::SymRoot)
				goto SyntaxError;
			return parseNsp(tokens, fr, to, root);
		case KeywordType::FuncDef :
			return parseFunc(tokens, fr, to, root);
		case KeywordType::Private:
		case KeywordType::Protected:
		case KeywordType::Public:
		case KeywordType::Override:
		case KeywordType::Fixed:
			break;
		default : goto SyntaxError;
	}
	return 0;
	SyntaxError:
	printf("line %d: invalid position of keyword \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static int parseExpr(const std::vector<Hcpl_Token> &token, size_t fr, size_t to, CplNode *&root, CplNode *father) {
	std::stack< std::pair<OperNode *, u32> > opers;
	std::stack<CplNode *> idens;

	return 0;
}

static int parseBlk(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t to, CplNode *&root, CplNode *father) {
	BlkNode *node = new BlkNode(); root = node; CplNode *child;
	int res = 0;
	for (size_t l = fr + 1, r = l; l < to; l = ++r) {
		res |= parse(tokens, l, r, child, node);
		if (res & Res_SeriousError) break;
		node->child.push_back(child);
	}
	return res;
}

static int parse(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root, CplNode *father) {
	switch (tokens[fr].type) {
		case Hcpl_TokenType::Keyword : return parseKeyword(tokens, fr, to, root, father);
		case Hcpl_TokenType::BrkSt :
			if (tokens[fr].brkInfo.type == BrkType::LargeL) 
				return to = tokens[fr].brkInfo.pir, parseBlk(tokens, fr, to, root, father);
		case Hcpl_TokenType::Iden:
		case Hcpl_TokenType::Const:
		case Hcpl_TokenType::Oper:
		case Hcpl_TokenType::String:
			for (to = fr; !isSpecOper(tokens[to], OperType::Comma) && !isSpecBrk(tokens[to], BrkType::LargeL); to++)
				if (tokens[to].type == Hcpl_TokenType::BrkSt) to = tokens[to].brkInfo.pir;
			return parseExpr(tokens, fr, to - 1, root, father);
	}
	return 0;
}

int Hcpl_makeCplTree(const std::vector<Hcpl_Token> &tokens, CplNodeType rootType, CplNode *&root) {
	root = new BlkNode(); 
	BlkNode *node = new BlkNode(); root = node; node->type = CplNodeType::SrcRoot;
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
				res |= Res_SeriousError;
				break;
		}
		if (res & Res_SeriousError) break;
	}
	return res;
}
