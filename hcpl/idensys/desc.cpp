#include "desc.hpp"
#include <type_traits>
#include <format>

namespace IdenSystem {
	SubstiMap substitute(const SubstiMap &x, const SubstiMap &y) {
		SubstiMap res;
		for (auto &pir : x) res.insert(std::make_pair(pir.first, pir.second->substitute(x)));
		return res;
	}

	bool Iden::isClsMember() { return parent->type == IdenType::Nsp; }

	bool Iden::isGlobal() { return parent->type == IdenType::Cls; }

	bool Iden::isLocal() { return parent->type == IdenType::Func; }

	ExprType::~ExprType() { }

	bool ExprType::isGeneric() const { return false; }

	ExprTypePtr_Ref ExprType::toRef() const {
		if (!referable) return nullptr;
		ExprTypePtr_Ref res = std::make_shared<ExprType_Ref>();
		res->srcType = deepCopy();
		res->srcType->referable = false;
		return res;
	}

    bool ExprType::equal(const ExprTypePtr &a, const ExprTypePtr &b) {
        if (a->category != b->category) return false;
		switch (a->category) {
			case ExprTypeCategory::FuncPtr : {
				auto funcA = dynCastPtr<ExprType, ExprType_FuncPtr>(a),
					funcB = dynCastPtr<ExprType, ExprType_FuncPtr>(b);
				if (!equal(funcA->retType, funcB->retType) || funcA->paramType.size() != funcB->paramType.size()) return false;
				for (size_t i = 0; i < funcA->paramType.size(); i++)
					if (!equal(funcA->paramType[i], funcB->paramType[i])) return false;
				break;
			}
			case ExprTypeCategory::Normal : {
				auto typeA = dynCastPtr<ExprType, ExprType_Normal>(a),
					typeB = dynCastPtr<ExprType, ExprType_Normal>(b);
				if (typeA->cls != typeB->cls || typeA->substList.size() != typeB->substList.size()) return false;
				for (size_t i = 0; i < typeA->substList.size(); i++)
					if (!equal(typeA->substList[i], typeB->substList[i])) return false;
				break;
			}
			case ExprTypeCategory::Array : {
				auto typeA = dynCastPtr<ExprType, ExprType_Array>(a),
					typeB = dynCastPtr<ExprType, ExprType_Array>(b);
				if (typeA->dimc != typeB->dimc || !equal(typeA->eleType, typeB->eleType)) return false;
				break;
			}
			case ExprTypeCategory::Ptr : {
				auto ptrA = dynCastPtr<ExprType, ExprType_Ptr>(a),
					ptrB = dynCastPtr<ExprType, ExprType_Ptr>(b);
				if (!equal(ptrA->srcType, ptrB->srcType)) return false;
				break;
			}
			case ExprTypeCategory::Ref : {
				auto ptrA = dynCastPtr<ExprType, ExprType_Ptr>(a),
					ptrB = dynCastPtr<ExprType, ExprType_Ptr>(b);
				if (!equal(ptrA->srcType, ptrB->srcType)) return false;
				break;
			}
		}
		return true;
    }

    ExprTypePtr ExprType_Normal::deepCopy() const {
		ExprTypePtr_Normal cpy = std::make_shared<ExprType_Normal>();
		cpy->cls = cls;
		cpy->referable = referable;
		cpy->substList.resize(substList.size());
		for (int i = 0; i < substList.size(); i++)
			cpy->substList[i] = substList[i]->deepCopy();
		return cpy;
	}

	IdenFitRate ExprType_Normal::fit(ExprTypePtr type, SubstiMap &substMap) const {
		if (type->category != ExprTypeCategory::Normal) {
			if (type->category == ExprTypeCategory::Ref && referable)
				return std::max(IdenFitRate::TypeConvert, toRef()->fit(type, substMap));
			else return IdenFitRate::NotFit;
		}
		ExprTypePtr_Normal normalType = dynCastPtr<ExprType, ExprType_Normal>(type);
		auto fitNormal = [&](ExprTypePtr_Normal type) -> IdenFitRate {
			if (type->cls->dep > cls->dep) return IdenFitRate::NotFit;
			if (type->cls->dep == cls->dep) {
				if (type->cls != cls || substList.size() != type->substList.size()) return IdenFitRate::NotFit;
				IdenFitRate rate = IdenFitRate::Prefect;
				for (int i = 0; i < substList.size(); i++)
					rate = std::max(rate, substList[i]->fit(type->substList[i], substMap));
				return rate;
			} else {
				ExprTypePtr_Normal bsType = toBsType();
				return bsType != nullptr ? std::max(bsType->fit(type, substMap), IdenFitRate::TypeConvert) : IdenFitRate::NotFit;
			}
		};
		if (normalType->cls->isGeneric) {
			auto iter = substMap.find(normalType->cls);

			if (iter != substMap.end()) {
				if (iter->second == nullptr) {
					iter->second = this->deepCopy();
					return IdenFitRate::Prefect;
				}
				ExprTypePtr_Normal substVal;
				{
					auto substi_temp = substMap[normalType->cls];
					if (substi_temp->category != ExprTypeCategory::Normal) return IdenFitRate::NotFit;
					substVal = dynCastPtr<ExprType, ExprType_Normal>(substi_temp);
				}
				
				bool flag1 = substVal->isGeneric(), flag2 = isGeneric();
				if (flag1 != flag2) return IdenFitRate::NotFit;
				// the substitution of this generic class is a generic class, then check if the substitution is the same as this expression type
				if (flag1 && flag2) 
					return substVal->cls == cls ? IdenFitRate::Prefect : IdenFitRate::NotFit;
				else return fitNormal(substVal);
			} else return IdenFitRate::NotFit;
		} else return fitNormal(normalType);
	}

	ExprType_Normal::~ExprType_Normal() {
		for (int i = 0; i < substList.size(); i++) substList[i] = nullptr;
	}

	ExprTypePtr_Normal ExprType_Normal::toBsType() const {
		if (cls->dep == 0) return nullptr;
	    ExprTypePtr_Normal res = std::make_shared<ExprType_Normal>();
		res->referable = referable;
		SubstiMap substMap;
		for (int i = 0; i < cls->generic.size(); i++) substMap.insert(std::make_pair(cls->generic[i].second, substList[i]));
		return dynCastPtr<ExprType, ExprType_Normal>(dynCastPtr<ExprType, ExprType_Normal>(cls->bsCls->deepCopy())->substitute(substMap));
	}

	u64 ExprType_Normal::size() const { return cls->size; }

	bool ExprType_Normal::isGeneric() const {
		return cls->isGeneric && substList.size() == 0;
	}

	ExprTypePtr ExprType_Normal::substitute(const SubstiMap &substMap) const {
		if (cls->isGeneric && substMap.count(cls)) return substMap.find(cls)->second;
	    ExprTypePtr_Normal res = std::make_shared<ExprType_Normal>();
		res->cls = cls;
		res->referable = referable;
		res->substList.resize(substList.size());
		for (int i = 0; i < res->substList.size(); i++)
			res->substList[i] = substList[i]->substitute(substMap);
		return res;
	}

	ExprType_Normal::ExprType_Normal() {
		category = ExprTypeCategory::Normal;
		cls = nullptr;
	}

	std::string ExprType_Normal::toString(int dep) {
		std::string res = std::format("{0}cls: fullName:{1},{2} referable\n", getIndent(dep), cls->fullName, referable ? "is" : "not");
		if (substList.size() > 0) {
			res.append(getIndent(dep) + "substitution:\n");
			for (int i = 0; i < substList.size(); i++) res.append(substList[i]->toString(dep + 1));
		}
		return res;
	}

	ExprTypePtr ExprType_Array::deepCopy() const {
        ExprTypePtr_Array res = std::make_shared<ExprType_Array>();
		res->dimc = dimc;
		res->referable = referable;
		res->eleType = eleType->deepCopy();
		return res;
    }

    IdenFitRate ExprType_Array::fit(ExprTypePtr type, SubstiMap &substMap) const {
        if (type->category != ExprTypeCategory::Array) {
			if (type->category == ExprTypeCategory::Ref && referable)
				return toRef()->fit(type, substMap);
			return IdenFitRate::NotFit;
		}
		auto arrType = dynCastPtr<ExprType, ExprType_Array>(type);
		return arrType->dimc == dimc ? eleType->fit(arrType->eleType, substMap) : IdenFitRate::NotFit;
    }

    ExprTypePtr ExprType_Array::substitute(const SubstiMap &substMap) const {
        ExprTypePtr_Array res = std::make_shared<ExprType_Array>();
		res->dimc = dimc;
		res->referable = referable;
		res->eleType = eleType->substitute(substMap);
		return res;
    }

    ExprType_Array::~ExprType_Array() { eleType = nullptr; }

    ExprType_Array::ExprType_Array() { category = ExprTypeCategory::Array; }

	std::string ExprType_Array::toString(int dep) {
		std::string res = std::format("{0}{1}-D {2} array of\n", getIndent(dep), std::to_string(dimc), referable ? "referable" : "not referable");
		res.append(eleType->toString(dep + 1));
		return res;
	}

	ExprTypePtr ExprType_Ptr::deepCopy() const {
	   ExprTypePtr_Ptr res = std::make_shared<ExprType_Ptr>();
	   res->srcType = srcType->deepCopy();
	   res->referable = referable;
	   return res;
	}

	IdenFitRate ExprType_Ptr::fit(ExprTypePtr type, SubstiMap &substMap) const {
	    if (type->category != ExprTypeCategory::Ptr) {
			if (type->category == ExprTypeCategory::Ref && referable)
				return toRef()->fit(type, substMap);
			return IdenFitRate::NotFit;
		}
		auto ptrType = dynCastPtr<ExprType, ExprType_Ptr>(type);
		return srcType->fit(ptrType->srcType, substMap);
	}

	ExprTypePtr ExprType_Ptr::substitute(const SubstiMap &substMap) const {
		ExprTypePtr_Ptr res = std::make_shared<ExprType_Ptr>();
		res->srcType = srcType->substitute(substMap);
		res->referable = referable;
		return res;
	}

	ExprType_Ptr::~ExprType_Ptr() { srcType = nullptr; }

	ExprType_Ptr::ExprType_Ptr() {
		category = ExprTypeCategory::Ptr;
		srcType = nullptr;
	}

	std::string ExprType_Ptr::toString(int dep) {
		std::string res = std::format("{0} {1} pointer of\n", getIndent(dep), referable ? "referable" : "not referable");
		res.append(srcType->toString(dep + 1));
		return res;
	}

	ExprTypePtr ExprType_FuncPtr::deepCopy() const {
		ExprTypePtr_FuncPtr res = std::make_shared<ExprType_FuncPtr>();
		res->retType = retType->deepCopy();
		res->referable = referable;
		res->paramType.resize(paramType.size());
		for (int i = 0; i < paramType.size(); i++) res->paramType[i] = paramType[i]->deepCopy();
	    return res;
	}

	IdenFitRate ExprType_FuncPtr::fit(ExprTypePtr type, SubstiMap &substMap) const {
		if (type->category != ExprTypeCategory::FuncPtr) {
			if (type->category == ExprTypeCategory::Ref && referable)
				return std::max(IdenFitRate::TypeConvert, toRef()->fit(type, substMap));
			return IdenFitRate::NotFit;
		}
		auto funcPtrType = dynCastPtr<ExprType, ExprType_FuncPtr>(type);
		if (funcPtrType->paramType.size() != paramType.size()
			|| funcPtrType->retType->fit(funcPtrType->retType, substMap) != IdenFitRate::Prefect)
			return IdenFitRate::NotFit;
		for (int i = 0; i < paramType.size(); i++)
			if (paramType[i]->fit(funcPtrType->paramType[i], substMap) != IdenFitRate::Prefect)
				return IdenFitRate::NotFit;
		return IdenFitRate::Prefect;
	}

	ExprTypePtr ExprType_FuncPtr::substitute(const SubstiMap &substMap) const {
	   	ExprTypePtr_FuncPtr res = std::make_shared<ExprType_FuncPtr>();
		res->retType = retType->substitute(substMap);
		res->referable = referable;
		res->paramType.resize(paramType.size());
		for (int i = 0; i < paramType.size(); i++)
			res->paramType[i] = paramType[i]->substitute(substMap);
		return res;
	}

	ExprType_FuncPtr::~ExprType_FuncPtr() {
		retType = nullptr;
		for (int i = 0; i < paramType.size(); i++) paramType[i] = nullptr;

	}

	ExprType_FuncPtr::ExprType_FuncPtr() {
		category = ExprTypeCategory::FuncPtr;
		retType = nullptr;
	}

	std::string ExprType_FuncPtr::toString(int dep) {
		std::string res = std::format("{0} {1} function pointer of\n", getIndent(dep), referable ? "referable" : "not referable");
		res.append(retType->toString(dep + 1));
		res.append(getIndent(dep) + "parameters:\n");
		for (int i = 0; i < paramType.size(); i++)
			res.append(paramType[i]->toString(dep + 1));
		return res;
	}

	ExprTypePtr ExprType_Ref::deepCopy() const {
		ExprTypePtr_Ref res = std::make_shared<ExprType_Ref>();
		res->srcType = srcType->deepCopy();
		return res;
	}

	IdenFitRate ExprType_Ref::fit(ExprTypePtr type, SubstiMap &substMap) const {
		if (type->category == ExprTypeCategory::Ref)
			return srcType->fit(dynCastPtr<ExprType, ExprType_Ref>(type)->srcType, substMap);
		if (type->category == srcType->category)
			return std::max(IdenFitRate::TypeConvert, srcType->fit(type, substMap));
		return IdenFitRate::NotFit;
	}

	ExprTypePtr ExprType_Ref::substitute(const SubstiMap &substMap) const {
		ExprTypePtr_Ref res = std::make_shared<ExprType_Ref>();
		res->srcType = srcType->deepCopy();
		return res;
	}

	ExprType_Ref::~ExprType_Ref() { srcType = nullptr; }

	ExprType_Ref::ExprType_Ref() {
		category = ExprTypeCategory::Ptr;
	}

	std::string ExprType_Ref::toString(int dep) {
		std::string res = std::format("{0}reference of\n", getIndent(dep));
		res.append(srcType->toString(dep + 1));
		return res;
	}

	bool IdenFrame::insertChild(Iden *iden) {
		const std::string &name = iden->name;
		if (nsp.count(name) || cls.count(name) || var.count(name) || enm.count(name))
			return false;
		switch (iden->type) {
			case IdenType::Nsp:
				nsp.insert(std::make_pair(name, (Namespace *)iden));
				break;
			case IdenType::Cls:
				cls.insert(std::make_pair(name, (Class *)iden));
				break;
			case IdenType::Func: {
				auto &fList = funcList[name];
				Function *fIden = (Function *)iden;
				{
					bool conflict = false;
					// make a parameter list
					// in python, it should be paramList = [param.varType.deepCopy() for param in fIden.params]
					std::vector<ExprTypePtr> paramList;
					for (Variable *param : fIden->params)
						paramList.push_back(param->varType->deepCopy());
					for (Function *func : fList) {
						const auto res = func->fit(name, {}, paramList);
						if (std::get<0>(res) == IdenFitRate::Prefect) {
							conflict = true;
							break;
						}
					}
					if (conflict) return false;
				}
				fList.push_back(fIden);
				break;
			}
			case IdenType::Var: var.insert(std::make_pair(name, (Variable *)iden)); break;
			case IdenType::Enum: enm.insert(std::make_pair(name, (Enum *)iden)); break;
			case IdenType::Error: return false;
		}
		return true;
	}

	std::vector<Iden *> IdenFrame::getChildren(const std::string &name, IdenAccessType minAcc, IdenAccessType maxAcc) const {
		std::vector<Iden *> res;
		#define chkMap(mapName) \
		do { \
			auto iter = (mapName).find(name); \
			if (iter != (mapName).end() && inRange(iter->second->access, minAcc, maxAcc)) res.push_back(iter->second); \
		} while (0)
		chkMap(nsp);
		chkMap(cls);
		chkMap(enm);
		chkMap(var);
		#undef chkMap
		{
			auto iter = funcList.find(name);
			if (iter != funcList.end()) {
				for (auto func : iter->second) if (inRange(func->access, minAcc, maxAcc))
					res.push_back(func);
			}
		}
	 
		return res;
	}

	IdenFrame::~IdenFrame() {
		for (auto &nPir : nsp) delete nPir.second;
		for (auto &cPir : cls) delete cPir.second;
		for (auto &fPir : funcList)
			for (auto &func : fPir.second) delete func;
		for (auto &vPir : var) delete vPir.second;
		for (auto &ePir : enm) delete ePir.second;
	}

	std::string IdenFrame::toString(int dep) {
		std::string res;
		for (auto cPir : cls) res.append(cPir.second->toString(dep));
		for (auto nPir : nsp) res.append(nPir.second->toString(dep));
		for (auto &fPir : funcList)
			for (auto &func : fPir.second)
				res.append(func->toString(dep));
		for (auto vPir : var) res.append(vPir.second->toString(dep));
		for (auto ePir : enm) res.append(ePir.second->toString(dep));
		for (auto usg : usgList) res.append(std::format("{0}using {1}\n", getIndent(dep), usg->fullName));
		return res;
	}

	Namespace::Namespace() { type = IdenType::Nsp; }

	std::string Namespace::toString(int dep) {
		std::string res = std::format("{0}Namesapce {1}->{2}:\n", getIndent(dep), name, fullName);
		res.append(child.toString(dep + 1));
		return res;
	}

	std::tuple<IdenFitRate, ExprTypePtr> Function::fit(const std::string &idenName, const std::vector<ExprTypePtr> &generParam, const std::vector<ExprTypePtr> &paramList) const {
		if (idenName != name) return std::make_tuple(IdenFitRate::NotFit, nullptr);
		SubstiMap substMap = outSubst;
		if (generParam.size() == 0) {
			for (int i = 0; i < generic.size(); i++) substMap[generic[i].second] = nullptr;
		} else {
			if (generParam.size() != generic.size()) return std::make_tuple(IdenFitRate::NotFit, nullptr);
			for (int i = 0; i < generic.size(); i++) substMap[generic[i].second] = generParam[i];
		}
		if (paramList.size() < defaultSt) return std::make_tuple(IdenFitRate::NotFit, nullptr);
		IdenFitRate rate = IdenFitRate::Prefect;
		for (int i = 0; i < paramList.size(); i++) {
			rate = std::max(rate, paramList[i]->fit(params[i]->varType->deepCopy(), substMap));
			if (rate == IdenFitRate::NotFit) return std::make_tuple(IdenFitRate::NotFit, nullptr);
		}
		for (int i = 0; i < generic.size(); i++) if (substMap[generic[i].second] == nullptr)
			return std::make_tuple(IdenFitRate::NotFit, nullptr);
		return std::make_tuple(rate, retType->substitute(substMap));
	}

	void IdenEnvironment::setGloNsp(Namespace *glo) { gloNsp = glo; }

    void IdenEnvironment::setCurFunc(Function *target) { curFunc = target; }

    void IdenEnvironment::setCurCls(Class *target) { curClass = target; }

	void IdenEnvironment::setCurNsp(Namespace *target) { curNsp = target; }

	Namespace *IdenEnvironment::getCurNsp() { return curNsp; }

    Class *IdenEnvironment::getCurCls() { return curClass; }

    Namespace *IdenEnvironment::getGloNsp() { return gloNsp; }

	void IdenEnvironment::localPush() {
		if (local.size()) preLocalVarNum += (local.end() - 1)->var.size();
		local.push_back(IdenFrame());
	}

	void IdenEnvironment::localPop() {
		local.pop_back();
		if (local.size()) preLocalVarNum -= (local.end() - 1)->var.size();
	}

	size_t IdenEnvironment::getLocalVarNum() {
		if (local.empty()) return 0; 
		else return (local.end() - 1)->var.size() + preLocalVarNum;
	}

	IdenFrame &IdenEnvironment::localTop() { return *(local.end() - 1); }

	std::vector<Iden *> IdenEnvironment::search(const std::vector<std::string> &path) const {
		std::vector<Iden *> ret;
		auto append = [&](std::vector<Iden *> &ret, const std::vector<Iden *> list) {
			for (Iden *iden : list) ret.push_back(iden);
		};
		// just search the identifier with access type that larger or equal to MIN_ACC
		auto search = [&path](Namespace *nsp, IdenAccessType minAcc) -> std::vector<Iden *> {
			Namespace *curNsp = nsp;
			for (int i = 0; i < path.size() - 1; i--) {
				std::vector<Iden *> res = curNsp->child.getChildren(path[i], minAcc);
				if (res.size() != 1 || res[0]->type != IdenType::Nsp) return {};
				curNsp = (Namespace *)res[0];
			}
			return curNsp->child.getChildren(path[path.size() - 1], minAcc);

		};
		auto searchUsg = [&](const std::vector<Namespace *> &usgList) -> std::vector<Iden *> {
			std::vector<Iden *> ret;
			for (Namespace *nsp : usgList) append(ret, search(nsp, IdenAccessType::Public));
			return ret;
		};
		if (path.size() == 1) {
			// search local items
			for (int i = local.size() - 1; i >= 0; i--)
				append(ret, local[i].getChildren(path[0]));
			if (curFunc != nullptr)
				for (auto &gener : curFunc->generic)
					if (gener.first == path[0]) ret.push_back(gener.second);
			// search class members
			if (curClass != nullptr) {
				append(ret, curClass->child.getChildren(path[0]));
				for (auto &gener : curClass->generic)
					if (gener.first == path[0]) ret.push_back(gener.second);
			}
			if (curNsp != nullptr) {
				append(ret, curNsp->child.getChildren(path[0], IdenAccessType::Protected, IdenAccessType::Public));
				// search from parent namespace
				for (Namespace *nsp = (Namespace *)curNsp->parent; nsp != nullptr; nsp = (Namespace *)nsp->parent)
					append(ret, nsp->child.getChildren(path[0], IdenAccessType::Protected, IdenAccessType::Protected));
			}
			
		}
		// search usage list of local environment
		for (int i = local.size() - 1; i >= 0; i--)
			append(ret, searchUsg(local[i].usgList));
		// search usage list of class
		if (curClass != nullptr) 
			append(ret, searchUsg(curClass->child.usgList));
		// search for identifier of of all access type from this namespace and the children namespace
		if (curNsp != nullptr) append(ret, search(curNsp, IdenAccessType::Private));
		// search usage list of this namespace and the parent namespace
		for (Namespace *nsp = curNsp; nsp != nullptr; nsp = (Namespace *)nsp->parent)
			append(ret, searchUsg(nsp->child.usgList));
		// search from global namespace
		append(ret, search(gloNsp, IdenAccessType::Public));
		return ret;
	}

	IdenEnvironment::~IdenEnvironment() {
		delete gloNsp;
	}

	ExprTypePtr objExprType(IdenEnvironment *idenEnv) {
        ExprTypePtr_Normal exprType = std::make_shared<ExprType_Normal>();
		exprType->cls = (Class *)idenEnv->getGloNsp()->child.getChildren("object")[0];
		return exprType;
    }

    bool allSpecType(const std::vector<Iden *> &idens, IdenType type) {
        for (Iden *iden : idens) if (iden->type != type) return false;
		return true;
    }

	Class::Class() { type = IdenType::Cls; }

	std::string Class::toString(int dep) {
		std::string res = std::format("{0}Class {1}->{2} access:{3}\n", getIndent(dep), name, fullName, IdenAccessType_toString(access));
		for (auto &gener : generic)
			res.append(std::format("{0}generic {1}\n", getIndent(dep + 1), gener.first));
		if (bsCls != nullptr)
			res.append(std::format("{0}bsCls:\n{1}", getIndent(dep + 1), bsCls->toString(dep + 1)));

		res.append(child.toString(dep + 1));
		return res;
	}

	Function::Function() { type = IdenType::Func; }

	std::string Function::toString(int dep) {
		std::string res = std::format("{0}Function {1}->{2} access:{3}\n", getIndent(dep), name, fullName, IdenAccessType_toString(access));
		for (auto &gener : generic)
			res.append(std::format("{0}generic {1}\n", getIndent(dep + 1), gener.first));
		res.append(retType->toString(dep + 1));
		res.append(std::format("{0}params:\n", getIndent(dep + 1)));
		for (auto &param : params)
			res.append(param->toString(dep + 1));
		return res;
	}

	Variable::Variable() { type = IdenType::Var; }

	std::string Variable::toString(int dep) {
		std::string res = std::format("{0}Variable {1}->{2} access:{3}\n", getIndent(dep), name, fullName, IdenAccessType_toString(access));
		res.append(varType->toString(dep + 1));
		return res;
	}

	Enum::Enum() { type = IdenType::Enum; }

	std::string Enum::toString(int dep)
	{
		std::string res = std::format("{0}Enum {1}->{2} access:{3}\n", getIndent(dep), name, fullName, IdenAccessType_toString(access));
		for (auto &item : items)
			res.append(std::format("{0}{1}\n", getIndent(dep + 1), item.first));
		return res;
	}
}