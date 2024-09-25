#pragma once

#include "tokenize.hpp"

enum class Hcpl_CplNodeType {
	Expr, Gener, Type, Const, Iden, Oper,
	NspDef, ClsDef, EnumDef, FuncDef, VarDef,
	Using,
	Block, Cond, Loop, Switch, Continue, Break, Return,
	SrcRoot, SymRoot
};

enum class Hcpl_AccessType {
	Public, Protected, Private
};

// Because I dont want to use inherentation
// there is no contruction and destruction function and every STL members should be initialized manually
struct CplNode {
	Hcpl_CplNodeType type;
	// if this node is a definition node, then the token is the identifier token
	Hcpl_Token token;
	int locVarNum = 0;
};

struct ExprNode : CplNode {
	CplNode *expRoot = nullptr;
	BsData constData;
};

struct GenericNode : CplNode {
	std::vector<CplNode *> types;
};

struct OperNode : CplNode {
	CplNode *lOperand = nullptr, *rOperand = nullptr;
	BsData constData;
};

struct TypeNode : CplNode {
	int dimc;
	int attr;
	#define Hcpl_TypeNode_Attr_isPtr 	(1 << 0)
	// if this bit is set, then isPtr bit should also be set.
	#define Hcpl_TypeNode_Attr_isFunc	(1 << 1) 

	// if this type is pointer, then subType is the source of the pointer
	// if this type is array, then subType is the type of the elements
	CplNode *subType;
	// if this type is pointer, then gener should be nullptr
	CplNode *gener;
};

struct EnumNode : CplNode {
	Hcpl_AccessType access;
	// if this enum class is an inner class, then this field points to the class, namespace or function that it belongs to
	CplNode *belong = nullptr;
	std::vector<CplNode *> Iden;
	std::vector<CplNode *> valExpr;
};

#define Hcpl_DefNode_Attr_Override	(1ul << 0)
#define Hcpl_DefNode_Attr_Fixed		(1ul << 1)
#define Hcpl_DefNode_Attr_Local		(1ul << 2)

struct VarNode : CplNode {
	Hcpl_AccessType access;
	TypeNode *varType = nullptr;
	CplNode *initExpr = nullptr;
};

struct UsingNode : CplNode {
	std::vector<std::string> path;
};

struct FuncNode : CplNode {
	Hcpl_AccessType access;
	u64 attr;
	TypeNode *retType = nullptr;
	CplNode *belong = nullptr, *content = nullptr;
	std::string fullName;
	std::vector<CplNode *> tmplList;
	std::vector<VarNode *> paramList;
};

struct ClsNode : CplNode {
	Hcpl_AccessType access;
	std::string fullName;
	// this field is the type of the parent class
	CplNode *parType;
	// if this class is an inner class, then this field points to the class that it belongs to
	CplNode *belong;
	std::vector<CplNode *> tmplList;
	std::vector<UsingNode *> usng;
	std::vector<EnumNode *> enm;
	std::vector<ClsNode *> cls;
	std::vector<FuncNode *> func;
	std::vector<VarNode *> var;
};

struct NspNode : CplNode {
	Hcpl_AccessType access;
	std::string fullName;
	// this field points to the namespace that it belongs to
	CplNode *belong;
	std::vector<std::string> path;
	std::vector<UsingNode *> usng;
	std::vector<EnumNode *> enm;
	std::vector<ClsNode *> cls;
	std::vector<FuncNode *> func;
	std::vector<VarNode *> var;
};

struct BlkNode : CplNode {
	std::vector<CplNode *> child;
};

struct CondNode : CplNode {
	CplNode *cond = nullptr, *succ = nullptr, *fail = nullptr;
};

struct LoopNode : CplNode {
	CplNode *init = nullptr, *cond = nullptr, *content = nullptr, *modify = nullptr;
};
struct CtrlNode : CplNode {
	CplNode *target = nullptr;
};
struct SwitchNode : CplNode {
	std::vector<CplNode *> content;
	std::vector< std::pair<CplNode *, size_t> > cases;
};



int Hcpl_makeCplTree(const std::vector<Hcpl_Token> &tokens, Hcpl_CplNodeType rootType, CplNode *&root);