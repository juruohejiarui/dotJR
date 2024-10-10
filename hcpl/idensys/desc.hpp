#pragma once

#include <map>
#include <vector>
#include <string>
#include <deque>
#include "../cpltree.hpp"

enum class IdenType {
	Nsp, Cls, Enum, Func, Var, Error
};
// priority of fitting of identifier, 
// for nsp, class, enum and variable, only prefect and NotFit are used,
// for function, the smaller the rate is, the the fitter this function 
enum class IdenFitRate {
	Prefect, GenericPrefect, BaseDataConvert, NotFit,
};
struct Class;
struct Enum;
struct Function;
struct Variable;
struct ExprType;
struct Namespace;

enum class ExprTypeCategory {
	Ptr, FuncPtr, Normal,	
};

struct ExprType {
	ExprTypeCategory category;
	virtual ExprType *deepCopy() const = 0;
	virtual IdenFitRate fit(ExprType *type, std::map<Class *, ExprType *> &substitude) const = 0;
	virtual ~ExprType() = 0;
};

struct ExprType_Normal {
	Class *cls;
	std::vector<ExprType *> substitude;
	virtual ExprType *deepCopy() const;
	virtual IdenFitRate fit(ExprType *type, std::map<Class *, ExprType *> &substitude) const;
	virtual ~ExprType_Normal();
};

struct ExprType_Ptr {
	ExprType *srcType;
	virtual ExprType *deepCopy() const;
	virtual IdenFitRate fit(ExprType *type, std::map<Class *, ExprType *> &substitude) const;
	virtual ~ExprType_Ptr();
};

struct ExprType_FuncPtr {
	ExprType *srcType;
	virtual ExprType *deepCopy() const;
	virtual IdenFitRate fit(ExprType *type, std::map<Class *, ExprType *> &substitude) const;
	virtual ~ExprType_FuncPtr();
};


struct Iden {
	IdenType type;
	IdenAccessType *access;
	std::map<std::string, Iden *> child;

	std::string name, fullName;
	
	Iden *parent;

	virtual std::tuple<IdenFitRate, ExprType *> fit(const std::string &idenName, const std::vector<ExprType *> paramList);
	virtual std::vector<Iden *> getChildren(const std::string &name, IdenAccessType *maxAcc) const = 0;

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
};

struct Namespace {
	IdenFrame child;
	std::vector<NspNode *> nodes;
	virtual std::vector<Iden *> getChildren(const std::string &name, IdenAccessType *maxAcc) const;
};

struct Class {
	IdenFrame child;
	std::vector< std::pair<std::string, Class *> > generic;
	ExprType *bsCls;
	u64 size;
	std::vector<ClsNode *> nodes;
	virtual std::vector<Iden *> getChildren(const std::string &name, IdenAccessType *maxAcc) const;
	virtual std::tuple<IdenFitRate, ExprType *> fit(const std::string &idenName, const std::vector<ExprType *> paramList);	
};

struct Function {
	std::vector< std::pair<std::string, Class *> > generic;
	ExprType *bsType;
	FuncNode *node;
	std::vector<Variable *> params;
	virtual std::tuple<IdenFitRate, ExprType *> fit(const std::string &idenName, const std::vector<ExprType *> paramList);
};

struct Variable {
	ExprType *type;
	BsData *constVal;
	VarNode *node;
	virtual std::tuple<IdenFitRate, ExprType *> fit(const std::string &idenName, const std::vector<ExprType *> paramList);
};

struct Enum {
	std::map<std::string, Variable *> items;
	EnumNode *node;
	virtual std::vector<Iden *> getChildren(const std::string &name, IdenAccessType *maxAcc) const;
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
