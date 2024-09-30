#include "cpltree.hpp"
#include <cstring>
#include <tuple>
#include <stack>

static int parse(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root, CplNode *father);
static int parseKeyword(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root, CplNode *father);
static int parseExpr(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t to, ExprNode *&root, CplNode *father);

static int parseUsing(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	UsingNode *node = new UsingNode(); root = node, node->type = CplNodeType::Using;
	node->token = tokens[fr + 1];
	if (tokens[fr + 1].type != Hcpl_TokenType::Iden) { to = fr + 1; goto SyntaxError; }
	if (tokens[fr + 2].type != Hcpl_TokenType::ExprEnd) { to = fr + 2; goto SyntaxError; }
	to = fr + 2;
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

	if (fr > to) { errorPos = fr; goto SyntaxError; }

	// ignore the () that wrap the whole type expression
	for (; fr <= to && isSpecBrk(tokens[fr], BrkType::SmallL) && tokens[fr].brkInfo.pir == to; fr++, to--) ;
	if (fr > to) { errorPos = fr; goto SyntaxError; }
	// check if it is an array
	if (isSpecBrk(tokens[to], BrkType::MediumR) && tokens[to].brkInfo.pir == to - 1) {
		while (isSpecBrk(tokens[to], BrkType::MediumR) && tokens[to].brkInfo.pir == to - 1 && fr < to) {
			node->attr |= TypeNode_Attr_isArr;
			node->dimc++;
			to = tokens[to].brkInfo.pir - 1;
		}
		node->token = tokens[to + 1];
		res |= parseType(tokens, fr, to, node->subType);
	// check if it is a pointer
	} else if (isSpecOper(tokens[to], OperType::Mul)) {
		// use the token of * to represents that this type is a pointer
		node->token = tokens[to];
		node->attr |= TypeNode_Attr_isPtr;
		res |= parseType(tokens, fr, to - 1, node->subType);
	// check if it is a function pointer
	} else if (isSpecBrk(tokens[to], BrkType::SmallR) || isSpecBrk(tokens[to], BrkType::GenericR)) {
		node->attr |= isSpecBrk(tokens[to], BrkType::SmallR) ? TypeNode_Attr_isFunc : TypeNode_Attr_hasGener;
		for (size_t l = tokens[to].brkInfo.pir + 1, r = l; l < to; l = ++r) {
			// get the token interval of the param type expression
			for (; !isSpecOper(tokens[r], OperType::Comma) && r < to; r++) skipBrk(tokens[r], r);
			TypeNode *param;
			res |= parseType(tokens, l, r - 1, param);
			node->params.push_back(param);
		}
		to = tokens[to].brkInfo.pir;
		if (res & Res_SeriousError) return res;
		// get the return type of the function pointer
		if (node->attr & TypeNode_Attr_isFunc)
			// use the token of ( to represents that this type is a function pointer
			node->token = tokens[to],
			res |= parseType(tokens, fr, to - 1, node->subType);
		else {
			if (fr < to - 1) { errorPos = to - 1; goto SyntaxError; }
			node->token = tokens[fr];
		}
	} else { // it is just a normal type expression
		if (fr != to) { errorPos = to - 1; goto SyntaxError; }
		node->token = tokens[to];
	}	
	return res;
	SyntaxError:
	printf("line %d: Syntax Error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static int parseExpr(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t to, ExprNode *&root, CplNode *father) {
	int errorPos = 0, res = 0, shift = 0;
	bool needType = false, needClass = false;
	std::vector<ExprNode *> ele;
	// make the element list
	for (size_t l = fr, r = l; l <= to; l = ++r) {
		switch (tokens[l].type) {
			case Hcpl_TokenType::Oper: {
				OperNode *opNode = new OperNode(); CplNode_initOperNode(opNode, tokens[l]);
				ele.push_back(opNode);
				if (tokens[l].opInfo.type == OperType::Cvt) needType = true;
				else if (tokens[l].opInfo.type == OperType::New) { needType = needClass = true; }
				break;
			}
			case Hcpl_TokenType::Iden: {
				if (needType) {
					
				}
			}
		}
	}
	return 0;
	SyntaxError:
	printf("line %d: syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
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
		for (size_t l = to + 1, r = l; l < tokens[to].brkInfo.pir; l = ++r) {
			if (tokens[l].type != Hcpl_TokenType::Iden || !isSpecOper(tokens[l + 1], OperType::Cvt)) { errorPos = l + 1; goto SyntaxError; }
			VarNode *param = new VarNode(); param->type = CplNodeType::VarDef;
			param->token = tokens[l];
			r = l + 2;
			bool needDefaultValue = false;
			// get the range of the type expresstion
			for (r = l + 2; !isSpecOper(tokens[r], OperType::Comma) && !isSpecOper(tokens[r], OperType::Ass) && !isSpecBrk(tokens[r], BrkType::SmallR); r++)
				skipBrk(tokens[r], r);
			res |= parseType(tokens, l + 2, r - 1, param->varType);
			// there is a default value of this parameter
			if (isSpecOper(tokens[r], OperType::Ass)) {
				for (l = r++; !isSpecOper(tokens[r], OperType::Comma) && !isSpecBrk(tokens[r], BrkType::SmallR); r++)
					skipBrk(tokens[r], r);
				res |= parseExpr(tokens, l, r - 1, param->initExpr, param);
				if (param->initExpr != nullptr && param->initExpr->constData.type != BsData_Type_void) {
					res |= 1;
					printf("line %d: expression of default value of function parameter should be constant\n", tokens[l].lineId);
				}
				needDefaultValue = true;
			} else if (needDefaultValue) { errorPos = l; goto DefaultValueError; }
			node->paramList.push_back(param);
		}
		to = tokens[to].brkInfo.pir + 1;

	} else { errorPos = to; goto SyntaxError; }
	// parse the return type node
	if (isSpecOper(tokens[to], OperType::Cvt)) {
		for (fr = ++to; !isSpecOper(tokens[to], OperType::Comma) && !isSpecBrk(tokens[to], BrkType::LargeL); to++)
			skipBrk(tokens[to], to);
		res |= parseType(tokens, fr, to - 1, node->retType);
	} else { errorPos = to; goto SyntaxError; }
	if (!isSpecBrk(tokens[to], BrkType::LargeL)) { errorPos = to; goto SyntaxError; }
	// parse the content, which must be a block
	res |= parse(tokens, to, to, node->content, node);
	return res;
	SyntaxError:
	printf("line %d: syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
	DefaultValueError:
	printf("line %d: parameter \"%s\" requires a default value\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static __always_inline int parseNsp(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	NspNode *node = new NspNode(); root = node, node->type = CplNodeType::NspDef;
	if (fr) setAccess(tokens[fr - 1], node);
	int res = 0;
	node->token = tokens[fr + 1];
	to = fr + 1;
	if (tokens[to].type != Hcpl_TokenType::Iden) goto SyntaxError;
	if (!isSpecBrk(tokens[++to], BrkType::LargeL)) goto SyntaxError;
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
	int errorPos = to, res = 0;
	switch (tokens[fr].kwType) {
		case KeywordType::Using:
			return parseUsing(tokens, fr, to, root);
		case KeywordType::Namespace :
			if (father->type != CplNodeType::SrcRoot && father->type != CplNodeType::SymRoot)
				goto SyntaxError;
			res |= parseNsp(tokens, fr, to, root);
			break;
		case KeywordType::FuncDef :
			res |= parseFunc(tokens, fr, to, root);
			((FuncNode *)root)->belong = father;
			break;
		case KeywordType::Private:
		case KeywordType::Protected:
		case KeywordType::Public:
		case KeywordType::Override:
		case KeywordType::Fixed:
			break;
		default : goto SyntaxError;
	}
	return res;
	SyntaxError:
	printf("line %d: invalid position of keyword \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
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
		case Hcpl_TokenType::String: {
			ExprNode *node;
			for (to = fr; !isSpecOper(tokens[to], OperType::Comma) && !isSpecBrk(tokens[to], BrkType::LargeL); to++)
				if (tokens[to].type == Hcpl_TokenType::BrkSt) to = tokens[to].brkInfo.pir;
			int res = parseExpr(tokens, fr, to - 1, node, father);
			root = node;
			return res;
		}
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
