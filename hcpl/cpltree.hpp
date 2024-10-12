#pragma once

#include "tokenize.hpp"

enum class CplNodeType {
	Expr, Gener, Type, Const, Iden, Oper,
	NspDef, ClsDef, EnumDef, FuncDef, VarDef, VarDefBlock,
	Using,
	Block, Cond, Loop, Switch, Continue, Break, Return,
	SrcRoot, SymRoot
};

enum class IdenAccessType {
	Public, Protected, Private
};

std::string IdenAccessType_toString(IdenAccessType val);

// Because I dont want to use inherentation
// there is no contruction and destruction function and every STL members should be initialized manually
struct CplNode {
	CplNodeType type;
	// if this node is a definition node, then the token is the identifier token
	Hcpl_Token token;
	int locVarNum = 0;
	virtual std::string toString(int dep = 0);
};

struct ExprNode : CplNode {
	BsData constData;
	ExprNode();
	bool isConst();
	virtual std::string toString(int dep = 0);
};
struct ExprRootNode : ExprNode {
	ExprNode *expr;
	virtual std::string toString(int dep = 0);
};
struct OperNode : ExprNode {
	ExprNode *lOperand = nullptr, *rOperand = nullptr;
	virtual std::string toString(int dep = 0);
};
static __inline__ void CplNode_initOperNode(OperNode *opNode, const Hcpl_Token &token) {
	opNode->type = CplNodeType::Oper;
	opNode->constData.type = BsData_Type_void;
	opNode->token = token;
}

struct TypeNode : ExprNode {
	int dimc = 0;
	int attr = 0;
	#define TypeNode_Attr_isPtr 	(1 << 0)
	#define TypeNode_Attr_isFunc	(1 << 1)
	#define TypeNode_Attr_isArr		(1 << 2)
	#define TypeNode_Attr_hasGener	(1 << 3)
	#define TypeNode_Attr_isRef		(1 << 4)

	// if this type is pointer, then subType is the source of the pointer
	// if this type is array, then subType is the type of the elements
	TypeNode *subType = nullptr;
	// this array should be empty if isFunc == 0
	// if hasGener == 1, then this list represents the generic parameters
	// if isFunc == 1, then this list represents the parameters type
	std::vector<TypeNode *> params;
	virtual std::string toString(int dep = 0);
};


struct IdenNode : ExprNode {
	int isFunc = 0;
	std::vector<TypeNode *> gener;
	std::vector<ExprNode *> param;
	virtual std::string toString(int dep = 0);
};

struct EnumNode : CplNode {
	IdenAccessType access;
	// if this enum class is an inner class, then this field points to the class, namespace or function that it belongs to
	CplNode *belong = nullptr;
	std::vector<std::string> iden;
	std::vector<ExprNode *> valExpr;
	virtual std::string toString(int dep = 0);
};

#define Hcpl_DefNode_Attr_Override	(1ul << 0)
#define Hcpl_DefNode_Attr_Fixed		(1ul << 1)
#define Hcpl_DefNode_Attr_Local		(1ul << 2)

struct VarNode : CplNode {
	TypeNode *varType = nullptr;
	ExprNode *initExpr = nullptr;
	virtual std::string toString(int dep = 0);
};

struct VarDefNode : CplNode {
	CplNode *belong;
	IdenAccessType access;
	std::vector<VarNode *> vars;
	virtual std::string toString(int dep = 0);
};

struct UsingNode : CplNode {
	std::vector<std::string> path;
	virtual std::string toString(int dep = 0);
};

struct FuncNode : CplNode {
	IdenAccessType access;
	u64 attr;
	TypeNode *retType = nullptr;
	CplNode *belong = nullptr, *content = nullptr;
	std::string fullName;
	std::vector<CplNode *> tmplList;
	std::vector<VarNode *> paramList;
	virtual std::string toString(int dep = 0);
};

struct ClsNode : CplNode {
	IdenAccessType access;
	std::string fullName;
	// this field is the type of the parent class
	TypeNode *bsCls;
	// if this class is an inner class, then this field points to the class that it belongs to
	CplNode *belong;
	std::vector<CplNode *> tmplList;
	std::vector<UsingNode *> usng;
	std::vector<EnumNode *> enm;
	std::vector<ClsNode *> cls;
	std::vector<FuncNode *> func;
	std::vector<VarDefNode *> var;
	virtual std::string toString(int dep = 0);
};

struct NspNode : CplNode {
	IdenAccessType access;
	std::string fullName;
	// this field points to the namespace that it belongs to
	CplNode *belong;
	std::vector<std::string> path;
	std::vector<UsingNode *> usng;
	std::vector<EnumNode *> enm;
	std::vector<ClsNode *> cls;
	std::vector<FuncNode *> func;
	std::vector<VarDefNode *> var;
	virtual std::string toString(int dep = 0);
};

struct BlkNode : CplNode {
	std::vector<CplNode *> child;
	virtual std::string toString(int dep = 0);
};

struct CondNode : CplNode {
	ExprNode *cond = nullptr;
	CplNode *succ = nullptr, *fail = nullptr;
	virtual std::string toString(int dep = 0);
};

struct LoopNode : CplNode {
	CplNode *init = nullptr, *content = nullptr;
	ExprNode *cond = nullptr, *modify = nullptr;
	virtual std::string toString(int dep = 0);
};
struct CtrlNode : CplNode {
	CplNode *target = nullptr;
};
struct SwitchNode : CplNode {
	std::vector<CplNode *> content;
	ExprNode *expr;
	std::vector<size_t> cases;
	virtual std::string toString(int dep = 0);
};

struct ReturnNode : CplNode {
	ExprNode *expr;
	virtual std::string toString(int dep = 0);
};



int Hcpl_makeCplTree(const std::vector<Hcpl_Token> &tokens, CplNodeType rootType, CplNode *&root);