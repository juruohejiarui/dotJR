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

static inline bool operator < (IdenFitRate a, IdenFitRate b) { return (int)a < (int)b; }
struct Class;
struct Enum;
struct Function;
struct Variable;
struct ExprType;
struct ExprType_Normal;
struct ExprType_Ptr;
struct ExprType_FuncPtr;
struct Namespace;

enum class ExprTypeCategory {
	Ptr, FuncPtr, Normal,	
};

typedef std::map<Class *, std::shared_ptr<ExprType> > SubstiMap;
typedef std::shared_ptr<ExprType> ExprTypePtr;
typedef std::shared_ptr<ExprType_Normal> ExprTypePtr_Normal;
typedef std::shared_ptr<ExprType_Ptr> ExprTypePtr_Ptr;
typedef std::shared_ptr<ExprType_FuncPtr> ExprTypePtr_FuncPtr;

struct ExprType {
	ExprTypeCategory category;
	virtual ExprTypePtr deepCopy() const = 0;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const = 0;
	virtual ~ExprType();
	virtual bool isGeneric() const ;
	virtual ExprTypePtr susbtitude(const SubstiMap &substMap) const = 0;
	ExprType() = default;
};

struct ExprType_Normal : ExprType {
	Class *cls;
	std::vector<ExprTypePtr> substList;
	virtual ExprTypePtr deepCopy() const;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
	virtual bool isGeneric() const;
	virtual ExprTypePtr susbtitude(const SubstiMap &substMap) const;
	virtual ~ExprType_Normal();
	ExprTypePtr_Normal toBsType() const;
	ExprType_Normal();
};

struct ExprType_Ptr : ExprType {
	ExprTypePtr srcType;
	virtual ExprTypePtr deepCopy() const;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
	virtual ExprTypePtr susbtitude(const SubstiMap &substMap) const;
	virtual ~ExprType_Ptr();
	ExprType_Ptr();
};

struct ExprType_FuncPtr : ExprType {
	ExprTypePtr retType;
	std::vector<ExprTypePtr> paramType;
	virtual ExprTypePtr deepCopy() const;
	virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
	virtual ExprTypePtr susbtitude(const SubstiMap &substMap) const;
	virtual ~ExprType_FuncPtr();
	ExprType_FuncPtr();
};


struct Iden {
	IdenType type;
	IdenAccessType *access;
	std::map<std::string, Iden *> child;

	std::string name, fullName;
	
	Iden *parent;

	virtual std::tuple<IdenFitRate, ExprType *> fit(const std::string &idenName, const std::vector<ExprTypePtr> &paramList, const SubstiMap &substMap);
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
	ExprTypePtr_Normal bsCls;
	bool isGeneric;
	u64 size, dep;
	std::vector<ClsNode *> nodes;
	virtual std::vector<Iden *> getChildren(const std::string &name, IdenAccessType *maxAcc) const;
	virtual std::tuple<IdenFitRate, ExprTypePtr> fit(const std::string &idenName, const std::vector<ExprTypePtr> &paramList, const SubstiMap &substMap);	
};

struct Function {
	std::vector< std::pair<std::string, Class *> > generic;
	ExprType *bsType;
	FuncNode *node;
	std::vector<Variable *> params;
	virtual std::tuple<IdenFitRate, ExprTypePtr> fit(const std::string &idenName, const std::vector<ExprTypePtr> &paramList, const SubstiMap &substMap);
};

struct Variable {
	ExprType *type;
	BsData *constVal;
	VarNode *node;
	virtual std::tuple<IdenFitRate, ExprTypePtr> fit(const std::string &idenName, const std::vector<ExprTypePtr> &paramList, const SubstiMap &substMap);
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
