#pragma once

#include <map>
#include <vector>
#include <string>
#include <deque>
#include <memory>
#include "../cpltree.hpp"

enum class IdenType {
	Nsp, Cls, Enum, Func, Var, Error
};
// priority of fitting of identifier, 
// for nsp, class, enum and variable, only prefect and NotFit are used,
// for function, the smaller the rate is, the the fitter this function 
enum class IdenFitRate {
	Prefect, TypeConvert, NotFit,
};

struct Class;
struct Enum;
struct Function;
struct Variable;

struct ExprType;
struct ExprType_Normal;
struct ExprType_Ptr;
struct ExprType_FuncPtr;
struct ExprType_Ref;

struct Namespace;

enum class ExprTypeCategory {
	Ptr, FuncPtr, Ref, Normal,	
};

typedef std::map<Class *, std::shared_ptr<ExprType> > SubstiMap;
#define emptySubstiMap (SubstiMap())

typedef std::shared_ptr<ExprType> 			ExprTypePtr;
typedef std::shared_ptr<ExprType_Normal> 	ExprTypePtr_Normal;
typedef std::shared_ptr<ExprType_Ptr> 		ExprTypePtr_Ptr;
typedef std::shared_ptr<ExprType_FuncPtr> 	ExprTypePtr_FuncPtr;
typedef std::shared_ptr<ExprType_Ref> 		ExprTypePtr_Ref;

// return substitute map after substitute all the generic class in Y found in X
SubstiMap substitute(const SubstiMap &x, const SubstiMap &y);

struct ExprType {
	ExprTypeCategory category;
	bool referable = false;

	virtual ExprTypePtr deepCopy() const = 0;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const = 0;
	virtual ~ExprType();
	virtual bool isGeneric() const ;
	virtual ExprTypePtr substitute(const SubstiMap &substMap) const = 0;
	virtual ExprTypePtr_Ref toRef() const;
	ExprType() = default;
};

struct ExprType_Normal : ExprType {
	Class *cls;
	std::vector<ExprTypePtr> substList;
	virtual ExprTypePtr deepCopy() const;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
	virtual bool isGeneric() const;
	virtual ExprTypePtr substitute(const SubstiMap &substMap) const;
	virtual ~ExprType_Normal();
	ExprTypePtr_Normal toBsType() const;
	ExprType_Normal();
};

struct ExprType_Ptr : ExprType {
	ExprTypePtr srcType;
	virtual ExprTypePtr deepCopy() const;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
	virtual ExprTypePtr substitute(const SubstiMap &substMap) const;
	virtual ~ExprType_Ptr();
	ExprType_Ptr();
};

struct ExprType_FuncPtr : ExprType {
	ExprTypePtr retType;
	std::vector<ExprTypePtr> paramType;
	virtual ExprTypePtr deepCopy() const;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
	virtual ExprTypePtr substitute(const SubstiMap &substMap) const;
	virtual ~ExprType_FuncPtr();
	ExprType_FuncPtr();
};

struct ExprType_Ref : ExprType {
	ExprTypePtr srcType;
	virtual ExprTypePtr deepCopy() const;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
	virtual ExprTypePtr substitute(const SubstiMap &substMap) const;
	virtual ~ExprType_Ref();
	ExprType_Ref();
};

struct Iden {
	IdenType type;
	IdenAccessType access;

	std::string name, fullName;
	
	Iden *parent;

	bool isClsMember();
	bool isGlobal();
	bool isLocal();
};

struct IdenFrame {
	std::map<std::string, Namespace *> nsp;
	std::map<std::string, Class *> cls;
	std::map< std::string, std::vector<Function *> > funcList;
	std::map<std::string, Variable *> var;
	std::map<std::string, Enum *> enm;

	std::map<std::string, Namespace *> usgList;

	// when belong == nulptr, then this is a local frame, 
	Iden *belong = nullptr;

	bool insertChild(Iden *iden);
	std::vector<Iden *> getChildren(const std::string &name, IdenAccessType minAcc = IdenAccessType::Private, IdenAccessType maxAcc = IdenAccessType::Public);
	~IdenFrame();
};

struct Namespace : Iden {
	IdenFrame child;
	std::vector<NspNode *> nodes;
	Namespace();
};

struct Class : Iden {
	IdenFrame child;
	std::vector< std::pair<std::string, Class *> > generic;
	ExprTypePtr_Normal bsCls;
	bool isGeneric;
	u64 size, dep;
	// the susbtitution map for inner class or local class
	SubstiMap outSubst;
	std::vector<ClsNode *> nodes;
};

struct Function : Iden {
	std::vector< std::pair<std::string, Class *> > generic;
	ExprType *retType;
	FuncNode *node;
	std::vector<Variable *> params;
	SubstiMap outSubst;
	// the start index of parameter that has default value
	size_t defaultSt;
	std::tuple<IdenFitRate, ExprTypePtr> fit(const std::string &idenName, const std::vector<ExprTypePtr> &generParam, const std::vector<ExprTypePtr> &paramList) const;
};

struct Variable : Iden {
	ExprType *varType;
	BsData *constVal;
	VarNode *node;
	SubstiMap outSubst;

	int varId;
};

struct Enum : Iden {
	std::map<std::string, Variable *> items;
	EnumNode *node;
};

struct IdenEnvironment {
private:
	Namespace *curNsp;
	Class *curClass;
	std::deque<IdenFrame> local;
public:
	void chgClass(Class *target);
	void chgNsp(Namespace *nsp);
	void localPush();
	void localPop();
	std::vector<Iden *> search(const std::vector<std::string &> ) const;
};
