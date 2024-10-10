#include "desc.hpp"

bool Iden::isClsMember() { return parent->type == IdenType::Nsp; }

bool Iden::isGlobal() { return parent->type == IdenType::Cls; }

bool Iden::isLocal() { return parent->type == IdenType::Func; }

ExprType *ExprType_Normal::deepCopy() const {
	ExprType_Normal *cpy = new ExprType_Normal();
	cpy->cls = cls;
	cpy->substitude.resize(substitude.size());
	for (int i = 0; i < substitude.size(); i++)
		cpy->substitude[i] = substitude[i]->deepCopy();
	return (ExprType *)cpy;
}

IdenFitRate ExprType_Normal::fit(ExprType * type, std::map<Class *, ExprType *> & substitude) const {
	
}
