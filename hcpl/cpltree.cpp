#include "cpltree.hpp"
#include <cstring>
#include <tuple>
#include <stack>
#include <queue>
#include <format>

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

std::stack<CplNode *> brkStk, lpStk;

#define setAccess(token, node) \
	do { \
		Hcpl_Token tk = (token); \
		if (tk.type != Hcpl_TokenType::Keyword) break; \
		switch (tk.kwType) { \
			case KeywordType::Private: 	(node)->access = IdenAccessType::Private; break; \
			case KeywordType::Public: 	(node)->access = IdenAccessType::Public; break; \
			case KeywordType::Protected:(node)->access = IdenAccessType::Protected; break; \
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
	// check if it is a reference
	} else if (isSpecOper(tokens[to], OperType::And)) {
		node->token = tokens[to];
		node->attr |= TypeNode_Attr_isRef;
		res |= parseType(tokens, fr, to - 1, node->subType);
	// check if it is a pointer
	} else if (isSpecOper(tokens[to], OperType::Mul)) {
		// use the token of * to represents that this type is a pointer
		node->token = tokens[to];
		node->attr |= TypeNode_Attr_isPtr;
		res |= parseType(tokens, fr, to - 1, node->subType);
	// check if it is a function pointer or a generic type
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

static void calcConst(OperNode *node) {
	BsData l, r;
	bool hasL = false, hasR;
	if (node->lOperand != nullptr) {
		if (node->lOperand->type == CplNodeType::Oper) calcConst((OperNode *)node->lOperand);
		l = node->lOperand->constData;
		hasL = true;
	}
	if (node->rOperand != nullptr) {
		if (node->rOperand->type == CplNodeType::Oper) calcConst((OperNode *)node->rOperand);
		r = node->rOperand->constData;
		hasR = true;
	}
	node->constData = calcConst(node->token.opInfo.type, l, r, hasL, hasR);
	return ;
	NotConst:
	node->constData.type = BsData_Type_void;
}

static int parseExpr(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t to, ExprNode *&root, CplNode *father) {
	int errorPos = 0, res = 0, shift = 0;
	bool needType = false, needClass = false;
	std::vector<ExprNode *> ele;
	std::queue< std::tuple<int, int, ExprNode **> > parseQue;
	auto calcWeight = [](OperType op, int isPrefix) -> std::tuple<OperType, int> {
		OperType relOp;
		if (isPrefix) {
			switch (op) {
				case OperType::SDec: relOp = OperType::PDec; break;
				case OperType::SInc: relOp = OperType::PInc; break;
				case OperType::Mul: relOp = OperType::GetVal; break;
				case OperType::And: relOp = OperType::GetAddr; break;
				case OperType::Sub: relOp = OperType::Minus; break;
				default: relOp = op; break;
			}
		} else relOp = op;
		return std::make_tuple(relOp, Hcpl_operWeight[(int)relOp]);
	};
	auto argmin = [&](int l, int r) -> int {
		int mnWeight = 25, pos = -1;
		if (l == r) {
			if (ele[l]->type == CplNodeType::Const || ele[l]->type == CplNodeType::Iden
			 || ele[l]->type == CplNodeType::Type || ele[l]->type == CplNodeType::Expr)
				return l;
			else {
				printf("line %d: Error on parsing expression\n", ele[l]->token.lineId);
				return -1;
			}
		}
		for (int i = l, isPrefix = true; i <= r; i++) {
			if (ele[i]->type != CplNodeType::Oper) {
				isPrefix = false;
				continue;
			}
			auto tpl = calcWeight(ele[i]->token.opInfo.type, isPrefix);
			// modify the type and weight of this operator
			ele[i]->token.opInfo.type = std::get<0>(tpl);
			ele[i]->token.opInfo.weight = std::get<1>(tpl);
			if ((isPrefix && std::get<1>(tpl) < mnWeight) || (!isPrefix && std::get<1>(tpl) <= mnWeight))
				pos = i, mnWeight = std::get<1>(tpl);
			isPrefix = true;
		}
		if (pos == -1) 
			printf("line %d: Error on parsing expression\n", ele[l]->token.lineId);
		return pos;
	};
	
	// make the element list
	for (size_t l = fr, r = l; l <= to; l = ++r) {
		switch (tokens[l].type) {
			case Hcpl_TokenType::Oper: {
				OperNode *opNode = new OperNode(); CplNode_initOperNode(opNode, tokens[l]);
				ele.push_back(opNode);
				break;
			}
			case Hcpl_TokenType::Const: {
				ExprNode *expr = new ExprNode();
				expr->type = CplNodeType::Const;
				expr->constData = tokens[l].data;
				ele.push_back(expr);
				break;
			}
			case Hcpl_TokenType::BrkSt: {
				switch (tokens[l].brkInfo.type) {
					case BrkType::SmallL: {
						ExprNode *expr;
						res |= parseExpr(tokens, l + 1, tokens[l].brkInfo.pir - 1, expr, nullptr);
						if (res & Res_SeriousError) return Res_SeriousError;
						ele.push_back(expr);
						break;
					}
					case BrkType::MediumL: {
						ExprNode *idx;
						OperNode *op = new OperNode();
						CplNode_initOperNode(op,tokens[l]);
						op->token.opInfo.type = OperType::Idx;
						op->token.opInfo.weight = Hcpl_operWeight[(int)OperType::Idx];
						res |= parseExpr(tokens, l + 1, tokens[l].brkInfo.pir - 1, idx, nullptr);
						if (res & Res_SeriousError) return Res_SeriousError;
						ele.push_back(op), ele.push_back(idx);
						break;
					}
					case BrkType::LargeL: {
						TypeNode *type;
						res |= parseType(tokens, l + 1, tokens[l].brkInfo.pir - 1, type);
						if (res & Res_SeriousError) return Res_SeriousError;
						ele.push_back(type);
						break;
					}
					default:
						errorPos = l; goto SyntaxError;
				}
				r = tokens[l].brkInfo.pir;
				break;
			}
			case Hcpl_TokenType::Iden: {
				IdenNode *idenNode = new IdenNode();
				idenNode->type = CplNodeType::Iden;
				idenNode->token = tokens[l];
				// followed by generic type list
				if (isSpecBrk(tokens[l + 1], BrkType::GenericL)) {
					for (size_t x = l + 2, y = x; x < tokens[l + 1].brkInfo.pir; x = ++y) {
						while (!isSpecOper(tokens[y], OperType::Comma) && y < tokens[l + 1].brkInfo.pir)
							skipBrk(tokens[y], y), y++;
						TypeNode *type;
						res |= parseType(tokens, x, y - 1, type);
						if (res & Res_SeriousError) return Res_SeriousError;
						idenNode->gener.push_back(type);
					}
					l = tokens[l + 1].brkInfo.pir;
					r = l;
				}
				// is a function call
				if (isSpecBrk(tokens[l + 1], BrkType::SmallL)) {
					idenNode->isFunc = 1;
					for (size_t x = l + 2, y = x; x < tokens[l + 1].brkInfo.pir; x = ++y) {
						while (!isSpecOper(tokens[y], OperType::Comma) && y < tokens[l + 1].brkInfo.pir)
							skipBrk(tokens[y], y), y++;
						ExprNode *param;
						res |= parseExpr(tokens, x, y - 1, param, nullptr);
						if (res & Res_SeriousError) return Res_SeriousError;
						idenNode->param.push_back(param);
					}
					r = tokens[l + 1].brkInfo.pir;
				}
				ele.push_back(idenNode);
				break;
			}
		}
	}
	root = new ExprRootNode();
	root->type = CplNodeType::Expr;
	parseQue.push(std::make_tuple(0, ele.size() - 1, &((ExprRootNode *)root)->expr));
	while (!parseQue.empty()) {
		auto u = parseQue.front(); parseQue.pop();
		int pos = argmin(std::get<0>(u), std::get<1>(u));
		if (pos == -1) {
			errorPos = std::get<0>(u);
			goto SyntaxError;
		}
		*std::get<2>(u) = ele[pos];
		if (pos > std::get<0>(u)) parseQue.push(std::make_tuple(std::get<0>(u), pos - 1, &((OperNode *)ele[pos])->lOperand));
		if (pos < std::get<1>(u)) parseQue.push(std::make_tuple(pos + 1, std::get<1>(u), &((OperNode *)ele[pos])->rOperand));
	}
	return res;
	SyntaxError:
	printf("line %d: expression syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static int parseCond(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	CondNode *node = new CondNode(); root = node, node->type = CplNodeType::Cond;
	node->token = tokens[fr];
	int errorPos = 0, res = 0;
	if (isSpecBrk(tokens[fr + 1], BrkType::SmallL)) {
		res |= parseExpr(tokens, fr + 1, tokens[fr + 1].brkInfo.pir - 1, node->cond, node);
		if (res & Res_SeriousError) return Res_SeriousError;
	} else { errorPos = fr + 1; goto SyntaxError; }
	to = tokens[fr + 1].brkInfo.pir;
	res |= parse(tokens, to + 1, to, node->succ, node);
	if (res & Res_SeriousError) return Res_SeriousError;
	if (node->succ != nullptr) node->locVarNum = node->succ->locVarNum;
	if (isSpecKw(tokens[to + 1], KeywordType::Else)) {
		res |= parse(tokens, to + 2, to, node->fail, node);
		if (res & Res_SeriousError) return Res_SeriousError;
		if (node->fail != nullptr) node->locVarNum = std::max(node->locVarNum, node->fail->locVarNum);
	}
	return res;
	SyntaxError:
	printf("line %d: Syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static int parseWhile(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	LoopNode *node = new LoopNode(); root = node, node->type = CplNodeType::Loop;

	lpStk.push(node);
	brkStk.push(node);

	node->token = tokens[fr];
	int errorPos = 0, res = 0;
	if (isSpecBrk(tokens[fr + 1], BrkType::SmallL)) {
		res |= parseExpr(tokens, fr + 1, tokens[fr + 1].brkInfo.pir - 1, node->cond, node);
		if (res & Res_SeriousError) goto Return;
	} else { errorPos = fr + 1; goto SyntaxError; }
	to = tokens[fr + 1].brkInfo.pir;
	res |= parse(tokens, to + 1, to, node->content, node);
	if (res & Res_SeriousError) goto Return;
	if (node->content != nullptr) node->locVarNum = node->content->locVarNum;
	Return:
	brkStk.pop(), lpStk.pop();
	return res;
	SyntaxError:
	printf("line %d: Loop syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static int parseFor(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	LoopNode *node = new LoopNode(); root = node, node->type = CplNodeType::Loop;

	lpStk.push(node);
	brkStk.push(node);

	node->token = tokens[fr];
	int errorPos = 0, res = 0;
	size_t subL, subR;
	if (!isSpecBrk(tokens[fr + 1], BrkType::SmallL)) { errorPos = fr + 1; goto SyntaxError; }
	subL = fr + 2, subR = subL; 
	res |= parse(tokens, subL, subR, node->init, node);
	if (res & Res_SeriousError) goto Return;
	if (node->init != nullptr && node->init->type == CplNodeType::VarDef) node->locVarNum = node->init->locVarNum;

	subL = ++subR;
	while (tokens[subR].type != Hcpl_TokenType::ExprEnd) skipBrk(tokens[subR], subR), subR++;
	res |= parseExpr(tokens, subL, subR - 1, node->cond, node);
	if (res & Res_SeriousError) goto Return;

	if (subR + 1 < tokens[fr + 1].brkInfo.pir) {
		res |= parseExpr(tokens, subR + 1, tokens[fr + 1].brkInfo.pir - 1, node->modify, node);
		if (res & Res_SeriousError) goto Return;
	}

	to = tokens[fr + 1].brkInfo.pir;
	res |= parse(tokens, to + 1, to, node->content, node);
	if (res & Res_SeriousError) goto Return;
	if (node->content != nullptr) node->locVarNum += node->content->locVarNum;

	Return:
	brkStk.pop();
	lpStk.pop();
	return res;

	SyntaxError:
	printf("line %d: For-loop syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static int parseSwitch(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	SwitchNode *node = new SwitchNode(); root = node, node->type = CplNodeType::Switch;
	node->token = tokens[fr];
	int errorPos = 0, res = 0;
	size_t l, r;
	brkStk.push(node);
	if (isSpecBrk(tokens[fr + 1], BrkType::SmallL)) {
		res |= parseExpr(tokens, fr + 1, tokens[fr + 1].brkInfo.pir - 1, node->expr, node);
		if (res & Res_SeriousError) goto Return;
	} else { errorPos = fr + 1; goto SyntaxError; }
	to = tokens[fr + 1].brkInfo.pir;
	if (!isSpecBrk(tokens[to + 1], BrkType::LargeL)) {
		errorPos = to + 1; goto SyntaxError;
	}
	for (l = to + 2, r = l; l < tokens[to + 1].brkInfo.pir; l = ++r) {
		if (isSpecKw(tokens[l], KeywordType::Case)) {
			while (!isSpecOper(tokens[r], OperType::Scope)) r++;
			ExprNode *cond;
			res |= parseExpr(tokens, l + 1, r - 1, cond, node);
			if (res & Res_SeriousError) goto Return;
			node->content.push_back(cond);
			node->cases.push_back(node->content.size() - 1);
		} else if (isSpecKw(tokens[l], KeywordType::VarDef)) {
			printf("line %d: Can not define variables in switch block directly, you can use {} and define the variable inside it.\n",
				tokens[l].lineId);
			res |= Res_SeriousError;
			break;
		} else {
			CplNode *child;
			res |= parse(tokens, l, r, child, node);
			if (res & Res_SeriousError) goto Return;
			node->locVarNum = std::max(node->locVarNum, child->locVarNum);
			node->content.push_back(child);
		}
	}
	Return:
	brkStk.pop();
	return res;
	SyntaxError:
	printf("line %d: For-loop syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static int parseCtrl(const std::vector<Hcpl_Token> &tokens, size_t fr , size_t &to, CplNode *&root) {
	while (tokens[to].type != Hcpl_TokenType::ExprEnd) to++;
	if (isSpecKw(tokens[fr], KeywordType::Return)) {
		ReturnNode *node = new ReturnNode();
		root = node;
		node->type = CplNodeType::Return;
		node->token = tokens[fr];
		if (fr + 1 < to) 
			return parseExpr(tokens, fr + 1, to - 1, node->expr, node);
		else return 0;
	}
	CtrlNode *node = new CtrlNode();
	root = node;
	node->token = tokens[fr];
	switch (tokens[fr].kwType) {
		case KeywordType::Break:
			node->type = CplNodeType::Break;
			if (brkStk.empty()) {
				printf("line %d: invalid usage of \"break\"", node->token.lineId);
				return Res_SeriousError;
			}
			node->target = brkStk.top();
			break;
		case KeywordType::Continue:
			node->type = CplNodeType::Continue;
			if (lpStk.empty()) {
				printf("line %d: invalid usage of \"continue\"", node->token.lineId);
				return Res_SeriousError;
			}
			node->target = lpStk.top();
			break;
	}
	return 0;
}

static __inline__ int parseVarDef(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	VarDefNode *node = new VarDefNode(); root = node, node->type = CplNodeType::VarDefBlock;
	if (fr) setAccess(tokens[fr - 1], node);
	to = fr;
	while (tokens[to].type != Hcpl_TokenType::ExprEnd) to++;
	size_t l, r;
	int errorPos = 0, res = 0;
	for (l = fr + 1, r = l; l < to; l = ++r) {
		const Hcpl_Token &fir = tokens[l];
		if (fir.type != Hcpl_TokenType::Iden) { errorPos = l; goto SyntaxError; }
		VarNode *varNode = new VarNode(); varNode->type = CplNodeType::VarDef;
		varNode->token = fir;
		r = l;
		if (isSpecOper(tokens[l + 1], OperType::Cvt)) {
			while (!isSpecOper(tokens[r], OperType::Ass) && !isSpecOper(tokens[r], OperType::Comma) && r < to) 
				skipBrk(tokens[r], r), r++;
			res |= parseType(tokens, l + 2, r - 1, varNode->varType);
			if (res & Res_SeriousError) return res;
			l = r;
		} else varNode->varType = nullptr, r = ++l;
		if (isSpecOper(tokens[l], OperType::Ass)) {
			while (!isSpecOper(tokens[r], OperType::Comma) && r < to) 
				skipBrk(tokens[r], r), r++;
			res |= parseExpr(tokens, l + 1, r - 1, varNode->initExpr, varNode);
			if (res & Res_SeriousError) return res;
			l = r;
		} else r = l;
		if (!isSpecOper(tokens[r], OperType::Comma) && r != to) {
			errorPos = r;
			goto SyntaxError;
		}
		node->locVarNum++;
		node->vars.push_back(varNode);
	}
	return res;
	SyntaxError:
	printf("line %d: variable definition syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;

}

static __inline__ int parseFunc(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	FuncNode *node = new FuncNode(); root = node, node->type = CplNodeType::FuncDef;
	int pre = fr, errorPos = fr, res = 0;
	// set attribute "fixed"
	if (pre && isSpecKw(tokens[pre - 1], KeywordType::Fixed))
		node->attr |= Hcpl_DefNode_Attr_Fixed, pre--;
	// set attribute "override"
	if (isSpecKw(tokens[pre - 1], KeywordType::Override))
		node->attr |= Hcpl_DefNode_Attr_Override, pre--;
	// set access
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

static __inline__ int parseEnum(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	EnumNode *node = new EnumNode(); root = node, node->type = CplNodeType::EnumDef;
	if (fr) setAccess(tokens[fr - 1], node);
	int res = 0, errorPos = 0;
	size_t l, r;
	node->token = tokens[fr + 1];
	to = fr + 1;
	if (!isSpecBrk(tokens[to], BrkType::LargeL)) {
		errorPos = to; goto SyntaxError;
	}
	for (l = to + 1, r = l; l < tokens[to].brkInfo.pir; l = ++r) {
		const Hcpl_Token &fir = tokens[l];
		if (fir.type != Hcpl_TokenType::Iden) { errorPos = l; goto SyntaxError; }
		node->iden.push_back(fir.strData);
		if (isSpecOper(tokens[l + 1], OperType::Ass)) {
			r = l + 1;
			while (!isSpecOper(tokens[r], OperType::Comma) && r < tokens[to].brkInfo.pir) 
				skipBrk(tokens[r], r), r++;
			ExprNode *valExpr;
			res |= parseExpr(tokens, l + 2, r - 1, valExpr, node);
			if (res & Res_SeriousError) break;
			node->valExpr.push_back(valExpr);
		} else if (!isSpecOper(tokens[l + 1], OperType::Comma) && l + 1 != tokens[to].brkInfo.pir) {
			errorPos = l + 1;
			goto SyntaxError;
		} else node->valExpr.push_back(nullptr), r = l + 1;
	}
	return res;
	SyntaxError:
	printf("line %d: enum syntax error on \"%s\"\n", tokens[errorPos].lineId, tokens[errorPos].strData.c_str());
	return Res_SeriousError;
}

static __inline__ int parseCls(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
	ClsNode *node = new ClsNode(); root = node, node->type = CplNodeType::ClsDef;
	if (fr) setAccess(tokens[fr - 1], node);
	int res = 0;
	node->token = tokens[fr + 1];
	to = fr + 1;
	// there is generic template
	if (isSpecBrk(tokens[fr + 2], BrkType::GenericL)) {
		to = fr + 2;
		for (size_t l = to + 1, r = l; l < tokens[to].brkInfo.pir; l = ++r) {
			while (r < tokens[to].brkInfo.pir && !isSpecOper(tokens[r], OperType::Comma))
				skipBrk(tokens[r], r), r++;
			if (l + 1 != r && tokens[l].type != Hcpl_TokenType::Iden) {
				printf("line %d: invalid syntax \"%s\"", tokens[l].lineId, tokens[l].strData.c_str());
				return Res_SeriousError;
			}
			CplNode *gener = new CplNode(); gener->type = CplNodeType::Iden;
			gener->token = tokens[l];
			node->tmplList.push_back(gener);
		}
		fr = tokens[to].brkInfo.pir + 1;
		to = fr;
	} else to = (fr += 2);
	// there is base class
	if (isSpecOper(tokens[fr], OperType::Cvt)) {
		to = fr;
		while (!isSpecBrk(tokens[to], BrkType::LargeL)) to++;
		res |= parseType(tokens, fr + 1, to - 1, node->bsCls);
		if (node->bsCls->attr) {
			printf("line %d: invalid base class", tokens[fr].lineId);
			res |= Res_Error;
		}
	} else to = fr;
	if (!isSpecBrk(tokens[to], BrkType::LargeL)) {
		printf("line %d: invalid syntax \"%s\"", tokens[to].lineId, tokens[to].strData.c_str());
		return Res_SeriousError;
	}
	for (size_t l = to + 1, r = l; l < tokens[to].brkInfo.pir; l = ++r) {
		CplNode *child = nullptr;
		switch (tokens[l].type) {
			case Hcpl_TokenType::Keyword :
				res |= parseKeyword(tokens, l, r, child, root);
				if (res & Res_SeriousError || child == nullptr) break;
				switch (child->type) {
					case CplNodeType::FuncDef		 : node->func.push_back((FuncNode *)child); break;
					case CplNodeType::VarDefBlock	 : node->var.push_back((VarDefNode *)child); break;
					case CplNodeType::EnumDef		 : node->enm.push_back((EnumNode *)child); break;
					case CplNodeType::ClsDef		 : node->cls.push_back((ClsNode *)child); break;
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
	return res;
}

static __inline__ int parseNsp(const std::vector<Hcpl_Token> &tokens, size_t fr, size_t &to, CplNode *&root) {
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
				if (res & Res_SeriousError || child == nullptr) break;
				switch (child->type) {
					case CplNodeType::FuncDef		 : node->func.push_back((FuncNode *)child); break;
					case CplNodeType::VarDefBlock	 : node->var.push_back((VarDefNode *)child); break;
					case CplNodeType::EnumDef		 : node->enm.push_back((EnumNode *)child); break;
					case CplNodeType::ClsDef		 : node->cls.push_back((ClsNode *)child); break;
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
	return res;
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
		case KeywordType::Class :
			res |= parseCls(tokens, fr, to, root);
			break;
		case KeywordType::EnumDef :
			res |= parseEnum(tokens, fr, to, root);
			break;
		case KeywordType::FuncDef :
			res |= parseFunc(tokens, fr, to, root);
			((FuncNode *)root)->belong = father;
			break;
		case KeywordType::VarDef :
			res |= parseVarDef(tokens, fr, to, root);
			((VarDefNode *)root)->belong = father;
			break;
		case KeywordType::Private:
		case KeywordType::Protected:
		case KeywordType::Public:
		case KeywordType::Override:
		case KeywordType::Fixed:
			break;
		case KeywordType::Break:
		case KeywordType::Continue:
		case KeywordType::Return:
			res |= parseCtrl(tokens, fr, to, root);
			break;
		case KeywordType::Switch:
			res |= parseSwitch(tokens, fr, to, root);
			break;
		case KeywordType::If:
			res |= parseCond(tokens, fr, to, root);
			break;
		case KeywordType::While:
			res |= parseWhile(tokens, fr, to, root);
			break;
		case KeywordType::For:
			res |= parseFor(tokens, fr, to, root);
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
	int varDef = 0;
	for (size_t l = fr + 1, r = l; l < to; l = ++r) {
		res |= parse(tokens, l, r, child, node);
		if (res & Res_SeriousError) break;
		node->child.push_back(child);
		if (child != nullptr) {
			if (child->type == CplNodeType::VarDefBlock) node->locVarNum = std::max(node->locVarNum, varDef += child->locVarNum);
			else node->locVarNum = std::max(node->locVarNum, varDef + child->locVarNum);
		}
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
			for (to = fr; tokens[to].type != Hcpl_TokenType::ExprEnd; to++)
				skipBrk(tokens[to], to);
			int res = parseExpr(tokens, fr, to - 1, node, father);
			root = node;
			return res;
		}
		case Hcpl_TokenType::ExprEnd: {
			root = nullptr;
			break;
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

#pragma region toString()
static std::string getIndent(int dep) { std::string str = ""; for (int i = 0; i < dep; i++) str.append("    "); return str; }

std::string IdenAccessType_toString(IdenAccessType val) {
    switch (val) {
		case IdenAccessType::Private:	return std::string("private");
		case IdenAccessType::Public:	return std::string("public");
		case IdenAccessType::Protected: return std::string("protected");
	}
	return std::string("invalid access type");
}

std::string CplNode::toString(int dep) {
    static const char *typeStr[] = {
		"Expr", "Gener", "Type", "Const", "Iden", "Oper",
		"NspDef", "ClsDef", "EnumDef", "FuncDef", "VarDef", "VarDefBlock",
		"Using",
		"Block", "Cond", "Loop", "Switch", "Continue", "Break", "Return",
		"SrcRoot", "SymRoot"
	};
	static char buf[128];
	sprintf(buf, "%10s strData:%s locVal:%2d\n", typeStr[(int)this->type], token.strData.c_str(), this->locVarNum);
	return getIndent(dep) + std::string(buf);
}

ExprNode::ExprNode() {
	constData.type = BsData_Type_void;
	constData.u64Data = 0;
}

bool ExprNode::isConst() { return ::isConst(constData); }

std::string ExprNode::toString(int dep) { return getIndent(dep) + "expr " + constData.toString() + "\n"; }

std::string ExprRootNode::toString(int dep) { return expr->toString(dep); }

std::string OperNode::toString(int dep) {
	static const char *opStr[] = {
		"Comma",
		"Ass", "AddAss", "SubAss", "MulAss", "DivAss", "ModAss", "AndAss", "OrAss", "XorAss", "ShlAss", "ShrAss",
		"Lor",
		"Land",
		"Or",
		"Xor",
		"And",
		"Eq", "Ne", "Oeq", "One",
		"Gt", "Ls", "Ge", "Le",
		"Shl", "Shr",
		"Add", "Sub",
		"Mul", "Div", "Mod",
		"New", "GetAddr", "GetVal", "Lnot", "Not", "Minus", "Cvt", "PInc", "PDec",
		"Idx", "SInc", "SDec", "Mem",
		"Scope",
	};
	std::string res = getIndent(dep) + "Oper " + opStr[(int)token.opInfo.type] + " constData:" + constData.toString();
	res.push_back('\n');
	if (lOperand != nullptr) res.append(lOperand->toString(dep + 1));
	if (rOperand != nullptr) res.append(rOperand->toString(dep + 1));
	return res;
}

std::string TypeNode::toString(int dep) {
    std::string res = getIndent(dep) + "type ";
	if (attr & TypeNode_Attr_isRef) {
		res.append("reference of\n");
		res.append(subType->toString(dep + 1));
	} else if (attr & TypeNode_Attr_isPtr) {
		res.append("pointer of\n");
		res.append(subType->toString(dep + 1));
	} else if (attr & TypeNode_Attr_isArr) {
		res.append(std::format("{0}-D array of\n", std::to_string(dimc)));
		res.append(subType->toString(dep + 1));
	} else if (attr & TypeNode_Attr_isFunc) {
		res.append("function pointer of\n");
		res.append(subType->toString(dep + 1));
		for (int i = 0; i < params.size(); i++)
			res.append(params[i]->toString(dep + 1));
	} else if (attr & TypeNode_Attr_hasGener) {
		res.append(std::format("generic {0}\n", token.strData));
		for (int i = 0; i < params.size(); i++)
			res.append(params[i]->toString(dep + 1));
	} else res.append(token.strData + "\n");
	return res;
}

std::string IdenNode::toString(int dep) {
	std::string res = std::format("{0}Iden {1}\n", getIndent(dep), token.strData);
	if (gener.size() > 0) {
		res.append(getIndent(dep) + "generic:\n");
		for (int i = 0; i < gener.size(); i++) res.append(gener[i]->toString(dep + 1));
	}
	if (isFunc) {
		res.append(getIndent(dep) + "parameters:\n");
		for (int i = 0; i < param.size(); i++) res.append(param[i]->toString(dep + 1));
	}
	return res;
}

std::string EnumNode::toString(int dep) {
    std::string res = std::format("{0}Enum {1}\n", getIndent(dep), token.strData);
	for (int i = 0; i < iden.size(); i++) {
		std::string idenStr = getIndent(dep + 1) + iden[i];
		if (valExpr[i] != nullptr) {
			idenStr.append("=\n");
			idenStr.append(valExpr[i]->toString());
		} else idenStr.push_back('\n');
		res.append(idenStr);
	}
	return res;
}

std::string VarNode::toString(int dep) {
	std::string res = std::format("{0}var {1}\n", getIndent(dep), token.strData);
	if (varType != nullptr)
		res.append(varType->toString(dep + 1));
	if (initExpr != nullptr)
		res.append(initExpr->toString(dep + 1));
	return res;
}

std::string VarDefNode::toString(int dep) {
    std::string res = std::format("{0}varDef access:{1}\n", getIndent(dep), IdenAccessType_toString(access));
	for (int i = 0; i < vars.size(); i++) {
		std::string varStr = getIndent(dep + 1) + vars[i]->token.strData + "\n";
		if (vars[i]->varType != nullptr) varStr.append(vars[i]->varType->toString(dep + 1));
		if (vars[i]->initExpr != nullptr) varStr.append(vars[i]->initExpr->toString(dep + 1));
		res.append(varStr);
	}
	return res;
}

std::string UsingNode::toString(int dep) {
    std::string res = CplNode::toString(dep) + " ";
	for (int i = 0; i < path.size(); i++) res.append("::" + path[i]);
	return res;
}

std::string FuncNode::toString(int dep) {
	std::string res = std::format("{0}Func {1} access:{2} attr:{3}\n", getIndent(dep), token.strData, IdenAccessType_toString(access), attr);
	if (retType != nullptr) res.append(retType->toString(dep + 1));
	for (int i = 0; i < tmplList.size(); i++) res.append(tmplList[i]->toString(dep + 1));
	for (int i = 0; i < paramList.size(); i++) res.append(paramList[i]->toString(dep + 1));
	res.append(content->toString(dep + 1));
	return res;
}

std::string ClsNode::toString(int dep) {
    std::string res = std::format("{0}Cls {1} access:{2}\n", getIndent(dep), token.strData, IdenAccessType_toString(access));
	if (bsCls != nullptr) res.append(bsCls->toString(dep + 1));
	for (int i = 0; i < tmplList.size(); i++) res.append(tmplList[i]->toString(dep + 2));
	for (UsingNode *node : usng) res.append(node->toString(dep + 1));
	for (ClsNode *node : cls) res.append(node->toString(dep + 1));
	for (FuncNode *node : func) res.append(node->toString(dep + 1));
	for (EnumNode *node : enm) res.append(node->toString(dep + 1));
	for (VarDefNode *node : var) res.append(node->toString(dep + 1));
	return res;
}

std::string NspNode::toString(int dep) {
	std::string res = std::format("{0}Nsp {1} access:{2}\n", getIndent(dep), token.strData, IdenAccessType_toString(access));
    for (UsingNode *node : usng) res.append(node->toString(dep + 1));
	for (ClsNode *node : cls) res.append(node->toString(dep + 1));
	for (FuncNode *node : func) res.append(node->toString(dep + 1));
	for (EnumNode *node : enm) res.append(node->toString(dep + 1));
	for (VarDefNode *node : var) res.append(node->toString(dep + 1));
	return res;
}

std::string BlkNode::toString(int dep) {
    std::string res = std::format("{0}Blk locVarNum:{1:2}\n", getIndent(dep), std::to_string(locVarNum));
	for (CplNode *child : child) res.append(child->toString(dep + 1));
	return res;
}

std::string CondNode::toString(int dep) {
    std::string res = getIndent(dep) + "Cond\n";
	res.append(cond->toString(dep + 1));
	res.append(getIndent(dep) + "succ:\n" + succ->toString(dep + 1));
	if (fail != nullptr)
		res.append(getIndent(dep) + "fail:\n" + fail->toString(dep + 1));
	return res;
}

std::string LoopNode::toString(int dep) {
    std::string res = getIndent(dep) + "Loop\n";
	if (init != nullptr) res.append(getIndent(dep) + "init:\n" + init->toString(dep + 1));
	if (cond != nullptr) res.append(getIndent(dep) + "cond:\n" + cond->toString(dep + 1));
	if (modify != nullptr) res.append(getIndent(dep) + "modi:\n" + modify->toString(dep + 1));
	if (content != nullptr) res.append(getIndent(dep) + "content:\n" + content->toString(dep + 1));
	return res;
}

std::string SwitchNode::toString(int dep) {
    std::string res = getIndent(dep) + "Switch\n";
	res.append(expr->toString(dep + 1));
	for (size_t pos = 0, i = 0; i < content.size(); i++) {
		if (cases[pos] == i) res.append(getIndent(dep + 1) + "[target]\n"), pos++;
		res.append(content[i]->toString(dep + 1));
	}
	return res;
}

std::string ReturnNode::toString(int dep) {
    std::string res = getIndent(dep) + "Return\n";
	if (expr != nullptr) res.append(expr->toString(dep + 1));
	else res.append(getIndent(dep + 1) + "void\n");
	return res;
}

#pragma endregion
