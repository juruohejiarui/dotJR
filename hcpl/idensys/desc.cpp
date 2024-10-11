#include "desc.hpp"
#include <type_traits>

bool Iden::isClsMember() { return parent->type == IdenType::Nsp; }

bool Iden::isGlobal() { return parent->type == IdenType::Cls; }

bool Iden::isLocal() { return parent->type == IdenType::Func; }

ExprType::~ExprType() { }

bool ExprType::isGeneric() const { return false; }

ExprTypePtr ExprType_Normal::deepCopy() const {
	ExprTypePtr_Normal cpy = std::make_shared<ExprType_Normal>();
	cpy->cls = cls;
	cpy->substList.resize(substList.size());
	for (int i = 0; i < substList.size(); i++)
		cpy->substList[i] = substList[i]->deepCopy();
	return cpy;
}

IdenFitRate ExprType_Normal::fit(ExprTypePtr type, SubstiMap &substMap) const {
	if (type->category != ExprTypeCategory::Normal) return IdenFitRate::NotFit;
	ExprTypePtr_Normal normalType = Lib_dynCastPtr<ExprType, ExprType_Normal>(type);
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
		if (substMap.count(normalType->cls)) {
			ExprTypePtr_Normal substi;
			{
				auto substi_temp = substMap[normalType->cls];
				if (substi_temp->category != ExprTypeCategory::Normal) return IdenFitRate::NotFit;
				substi = Lib_dynCastPtr<ExprType, ExprType_Normal>(substi_temp);
			}
			
			bool flag1 = substi->isGeneric(), flag2 = isGeneric();
			if (flag1 != flag2) return IdenFitRate::NotFit;
			// the substitution of this generic class is a generic class, then check if the substitution is the same as this expression type
			if (flag1 && flag2) 
				return substi->cls == cls ? IdenFitRate::Prefect : IdenFitRate::NotFit;
			else return fitNormal(substi);
		} else {
			substMap.insert(std::make_pair(normalType->cls, this->deepCopy()));
			return IdenFitRate::Prefect;
		}
	} else return fitNormal(normalType);
}

ExprType_Normal::~ExprType_Normal() {
	for (int i = 0; i < substList.size(); i++) substList[i] = nullptr;
}

ExprTypePtr_Normal ExprType_Normal::toBsType() const {
	if (cls->dep == 0) return nullptr;
    ExprTypePtr_Normal res = std::make_shared<ExprType_Normal>();
	SubstiMap substMap;
	for (int i = 0; i < cls->generic.size(); i++) substMap.insert(std::make_pair(cls->generic[i].second, substList[i]));
	return Lib_dynCastPtr<ExprType, ExprType_Normal>(Lib_dynCastPtr<ExprType, ExprType_Normal>(cls->bsCls->deepCopy())->susbtitude(substMap));
}

bool ExprType_Normal::isGeneric() const {
	return cls->isGeneric && substList.size() == 0;
}

ExprTypePtr ExprType_Normal::susbtitude(const SubstiMap &substMap) const {
	if (cls->isGeneric && substMap.count(cls)) return substMap.find(cls)->second;
    ExprTypePtr_Normal res = std::make_shared<ExprType_Normal>();
	res->cls = cls;
	res->substList.resize(substList.size());
	for (int i = 0; i < res->substList.size(); i++)
		res->substList[i] = substList[i]->susbtitude(substMap);
	return res;
}

ExprType_Normal::ExprType_Normal() {
	category = ExprTypeCategory::Normal;
	cls = nullptr;
}

ExprTypePtr ExprType_Ptr::deepCopy() const {
   ExprTypePtr_Ptr res = std::make_shared<ExprType_Ptr>();
   res->srcType = srcType->deepCopy();
   return res;
}

IdenFitRate ExprType_Ptr::fit(ExprTypePtr type, SubstiMap &substMap) const {
    if (type->category != ExprTypeCategory::Ptr) return IdenFitRate::NotFit;
	auto ptrType = Lib_dynCastPtr<ExprType, ExprType_Ptr>(type);
	return srcType->fit(ptrType->srcType, substMap);
}

ExprTypePtr ExprType_Ptr::susbtitude(const SubstiMap &substMap) const {
	ExprTypePtr_Ptr res = std::make_shared<ExprType_Ptr>();
	res->srcType = srcType->susbtitude(substMap);
	return res;
}

ExprType_Ptr::~ExprType_Ptr() { srcType = nullptr; }

ExprType_Ptr::ExprType_Ptr() {
	category = ExprTypeCategory::Ptr;
	srcType = nullptr;
}

ExprTypePtr ExprType_FuncPtr::deepCopy() const {
	ExprTypePtr_FuncPtr res = std::make_shared<ExprType_FuncPtr>();
	res->retType = retType->deepCopy();
	res->paramType.resize(paramType.size());
	for (int i = 0; i < paramType.size(); i++) res->paramType[i] = paramType[i]->deepCopy();
    return res;
}

IdenFitRate ExprType_FuncPtr::fit(ExprTypePtr type, SubstiMap &substMap) const {
	if (type->category != ExprTypeCategory::FuncPtr) return IdenFitRate::NotFit;
	auto funcPtrType = Lib_dynCastPtr<ExprType, ExprType_FuncPtr>(type);
	if (funcPtrType->paramType.size() != paramType.size()
		|| funcPtrType->retType->fit(funcPtrType->retType, substMap) != IdenFitRate::Prefect)
		return IdenFitRate::NotFit;
	for (int i = 0; i < paramType.size(); i++)
		if (paramType[i]->fit(funcPtrType->paramType[i], substMap) != IdenFitRate::Prefect)
			return IdenFitRate::NotFit;
	return IdenFitRate::Prefect;
}

ExprTypePtr ExprType_FuncPtr::susbtitude(const SubstiMap &substMap) const {
   	ExprTypePtr_FuncPtr res = std::make_shared<ExprType_FuncPtr>();
	res->retType = retType->susbtitude(substMap);
	res->paramType.resize(paramType.size());
	for (int i = 0; i < paramType.size(); i++)
		res->paramType[i] = paramType[i]->susbtitude(substMap);
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
