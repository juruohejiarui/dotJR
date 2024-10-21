#pragma once

#include <map>
#include <vector>
#include <string>
#include <deque>
#include <memory>
#include "../cpltree.hpp"

namespace IdenSystem {
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
	struct ExprType_Array;
	struct ExprType_Ptr;
	struct ExprType_FuncPtr;
	struct ExprType_Ref;

	struct Namespace;

	enum class ExprTypeCategory {
		Ptr, FuncPtr, Ref, Array, Normal,
	};

	typedef std::map<Class *, std::shared_ptr<ExprType> > SubstiMap;
	#define emptySubstiMap (SubstiMap())

	typedef std::shared_ptr<ExprType> 			ExprTypePtr;
	typedef std::shared_ptr<ExprType_Normal> 	ExprTypePtr_Normal;
	typedef std::shared_ptr<ExprType_Array>		ExprTypePtr_Array;
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
		virtual u64 size() const = 0;
		ExprType() = default;

		// check whether two type expression is completely same
		static bool equal(const ExprTypePtr &a, const ExprTypePtr &b);

		virtual std::string toString(int dep = 0) = 0;
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
		virtual u64 size() const;
		ExprType_Normal();

		virtual std::string toString(int dep = 0);
	};

	struct ExprType_Array : ExprType {
		ExprTypePtr eleType = nullptr;
		int dimc = 0;
		virtual ExprTypePtr deepCopy() const;
		virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
		virtual ExprTypePtr substitute(const SubstiMap &substMap) const;
		virtual ~ExprType_Array();
		ExprType_Array();

		virtual std::string toString(int dep = 0);
	};

	struct ExprType_Ptr : ExprType {
		ExprTypePtr srcType;
		virtual ExprTypePtr deepCopy() const;
		virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
		virtual ExprTypePtr substitute(const SubstiMap &substMap) const;
		virtual ~ExprType_Ptr();
		ExprType_Ptr();

		virtual std::string toString(int dep = 0);
	};

	struct ExprType_FuncPtr : ExprType {
		ExprTypePtr retType;
		std::vector<ExprTypePtr> paramType;
		virtual ExprTypePtr deepCopy() const;
		virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
		virtual ExprTypePtr substitute(const SubstiMap &substMap) const;
		virtual ~ExprType_FuncPtr();
		ExprType_FuncPtr();

		virtual std::string toString(int dep = 0);
	};

	struct ExprType_Ref : ExprType {
		ExprTypePtr srcType;
		virtual ExprTypePtr deepCopy() const;
		virtual IdenFitRate fit(ExprTypePtr type, SubstiMap &substMap) const;
		virtual ExprTypePtr substitute(const SubstiMap &substMap) const;
		virtual ~ExprType_Ref();
		ExprType_Ref();

		virtual std::string toString(int dep = 0);
	};


	struct Iden {
		IdenType type;
		IdenAccessType access = IdenAccessType::Private;

		std::string name, fullName;
		
		Iden *parent = nullptr;

		bool isClsMember();
		bool isGlobal();
		bool isLocal();

		virtual std::string toString(int dep = 0) = 0;
	};

	struct IdenFrame {
		std::map<std::string, Namespace *> nsp;
		std::map<std::string, Class *> cls;
		std::map< std::string, std::vector<Function *> > funcList;
		std::map<std::string, Variable *> var;
		std::map<std::string, Enum *> enm;

		std::vector<Namespace *> usgList;

		// when belong == nulptr, then this is a local frame, 
		Iden *belong = nullptr;

		bool insertChild(Iden *iden);
		std::vector<Iden *> getChildren(const std::string &name, IdenAccessType minAcc = IdenAccessType::Private, IdenAccessType maxAcc = IdenAccessType::Public) const;
		~IdenFrame();

		std::string toString(int dep);
	};

	struct Namespace : Iden {
		IdenFrame child;
		std::vector<NspNode *> nodes;
		Namespace();

		virtual std::string toString(int dep);
	};

	struct Class : Iden {
		IdenFrame child;
		std::vector< std::pair<std::string, Class *> > generic;
		ExprTypePtr_Normal bsCls = nullptr;
		bool isGeneric = false;
		bool isBaseType = false;
		u64 size = 0;
		int dep = -1;
		// the susbtitution map for inner class or local class
		SubstiMap outSubst;
		std::vector<ClsNode *> nodes;
		Class();

		virtual std::string toString(int dep);
	};

	struct Function : Iden {
		// generic class list
		std::vector< std::pair<std::string, Class *> > generic;
		ExprTypePtr retType;
		FuncNode *node;
		std::vector<Variable *> params;
		SubstiMap outSubst;
		// the start index of parameter that has default value
		size_t defaultSt;
		std::tuple<IdenFitRate, ExprTypePtr> fit(const std::string &idenName, const std::vector<ExprTypePtr> &generParam, const std::vector<ExprTypePtr> &paramList) const;

		std::vector<OperType> generOper;

		// if this function is inherented from base type, then this attribute points to the definition of the original one
		// otherwise, this points to itself.
		Function *oriFunc = nullptr;
		SubstiMap substToOri;
		bool isVirtual = false;

		Function();

		virtual std::string toString(int dep);
	};

	struct Variable : Iden {
		ExprTypePtr varType;
		BsData *constVal;
		VarNode *node;
		SubstiMap outSubst;

		size_t offset;
		
		Variable *oriVar = nullptr;
		SubstiMap substToOri;

		Variable();

		virtual std::string toString(int dep);
	};

	struct Enum : Iden {
		std::map<std::string, Variable *> items;
		EnumNode *node;

		Enum();

		virtual std::string toString(int dep);
	};

	struct IdenEnvironment {
	private:
		Namespace *curNsp = nullptr, *gloNsp = nullptr;
		Class *curClass = nullptr;
		Function *curFunc = nullptr;
		std::deque<IdenFrame> local;
		size_t preLocalVarNum = 0;
		
	public:
		void setGloNsp(Namespace *glo);
		void setCurFunc(Function *target);
		void setCurCls(Class *target);
		void setCurNsp(Namespace *target);
		Namespace *getCurNsp();
		Class *getCurCls();
		Namespace *getGloNsp();
		void localPush();
		void localPop();
		size_t getLocalVarNum();
		IdenFrame &localTop();

		std::vector<Iden *> search(const std::vector<std::string> &path) const ;

		~IdenEnvironment();
	};

	// get the convertion of base class "object"
	ExprTypePtr objExprType(IdenEnvironment *idenEnv);

	ExprTypePtr cvtToExprType(IdenEnvironment *idenEnv, TypeNode *typeNode);

	bool allSpecType(const std::vector<Iden *> &idens, IdenType type);

	Namespace *buildGloNsp();

	IdenEnvironment *build(Namespace *glo, const std::vector<CplNode *> &roots);

	bool buildIdenFile(Namespace *glo);
}