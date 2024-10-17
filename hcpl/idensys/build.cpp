#include "desc.hpp"
#include <format>
#include <iostream>

namespace IdenSystem {
    // set the parent of IDEN and modify the fullname
    static void setParent(Iden *iden, Iden *parent) {
        iden->parent = parent;
        iden->fullName = std::format("{0}#{1}", parent->fullName, iden->name);
    }

	ExprTypePtr cvtToExprType(IdenEnvironment *idenEnv, TypeNode *typeNode) {
        if (typeNode->attr & TypeNode_Attr_isArr) {
            ExprTypePtr_Array arrType = std::make_shared<ExprType_Array>();
            arrType->dimc = typeNode->dimc;
            arrType->eleType = cvtToExprType(idenEnv, typeNode->subType);
            return arrType->eleType != nullptr ? arrType : nullptr;
        } if (typeNode->attr & TypeNode_Attr_isRef) {
            ExprTypePtr_Ref refType = std::make_shared<ExprType_Ref>();
            refType->srcType = cvtToExprType(idenEnv, typeNode->subType);
            return refType->srcType != nullptr ? refType : nullptr;
        } else if (typeNode->attr & TypeNode_Attr_isPtr) {
            ExprTypePtr_Ptr ptrType = std::make_shared<ExprType_Ptr>();
            ptrType->srcType = cvtToExprType(idenEnv, typeNode->subType);
            return ptrType->srcType != nullptr ? ptrType : nullptr;
        } else if (typeNode->attr & TypeNode_Attr_isFunc) {
            ExprTypePtr_FuncPtr funcPtrType = std::make_shared<ExprType_FuncPtr>();
            funcPtrType->retType = cvtToExprType(idenEnv, typeNode->subType);
            if (funcPtrType->retType == nullptr) return nullptr;
            funcPtrType->paramType.resize(typeNode->params.size());
            for (size_t i = 0; i < funcPtrType->paramType.size(); i++) {
                if ((funcPtrType->paramType[i] = cvtToExprType(idenEnv, typeNode->params[i])) == nullptr)
                    return nullptr;
            }
            return funcPtrType;
        } else {
            ExprTypePtr_Normal type = std::make_shared<ExprType_Normal>();
            auto res = idenEnv->search(split(typeNode->token.strData, "#"));
            if (res[0]->type != IdenType::Cls) return nullptr;
            type->cls = (Class *)res[0];
            if (typeNode->attr & TypeNode_Attr_hasGener) {
                type->substList.resize(typeNode->params.size());
                for (size_t i = 0; i < type->substList.size(); i++)
                    if ((type->substList[i] = cvtToExprType(idenEnv, typeNode->params[i])) == nullptr)
                        return nullptr;
            }
            return type;
        }
        return nullptr;
	}
	
	Namespace *buildGloNsp() {
		Namespace *glo = new Namespace();
        glo->access = IdenAccessType::Public;

        const std::vector< std::pair<std::string, size_t> > baseClassName
            = { {"int", 4}, {"long", 8}, {"short", 2}, {"char", 1},
                {"uint", 4}, {"ulong", 8}, {"ushort", 2}, {"uchar", 1}, 
				{"object", 8}};

        for (const auto &clsDesc : baseClassName) {
            Class *cls = new Class();
            cls->size = clsDesc.second;
            cls->name = clsDesc.first;
            cls->dep = 1;
            cls->access = IdenAccessType::Public;
            setParent(cls, glo);
            glo->child.insertChild(cls);
        }
        return glo;
	}

	// create a namespace list using PATH, return nullptr if one identifier of the path is used by other types
	Namespace *createNsp(Namespace *glo, const std::vector<std::string> &path) {
		Namespace *cur = glo;
		for (int i = 0; i < path.size(); i++) {
			std::vector<Iden *> idens = cur->child.getChildren(path[i]);
			if (idens.size() == 0) {
				Namespace *nsp = new Namespace();
				nsp->name = path[i], setParent(nsp, cur);
				cur->child.insertChild(nsp);
				nsp->access = IdenAccessType::Public;
			// it is used by other types of identifiers
			} else if (idens.size() > 1 || !allSpecType(idens, IdenType::Nsp)) return nullptr;
			cur = (Namespace *)idens[0];
		}
		return cur;
	}

	// create a class in the namespace BLG with identifier NAME
    // When second element of the return val is true, it means this class is created, and the first element is the class structure pointer
    // When is false, it means this clas has already been created and the first element is the structure created previously.
	std::tuple<Class *, bool> createCls(Namespace *blg, const std::string &name) {
		std::vector<Iden *> idens = blg->child.getChildren(name);
		if (idens.size() == 0) {
			Class *cls = new Class();
			cls->name = name;
			setParent(cls, blg);
			blg->child.insertChild(cls);
			return std::make_tuple(cls, true);
		} else if (idens.size() > 1 || !allSpecType(idens, IdenType::Cls)) return std::make_tuple(nullptr, false);
		return std::make_tuple((Class *)idens[0], false);
	}

	bool buildClsNode(IdenEnvironment *idenEnv, ClsNode *clsNode) {
		auto createRes = createCls(idenEnv->getCurNsp(), clsNode->token.strData);
		Class *cls = std::get<0>(createRes);
        if (idenEnv->getCurCls() != nullptr) {
            std::cout << std::format("line {0}: unsupported feature: inner class.\n", clsNode->token.lineId, clsNode->token.strData);
            return false;
        }
		if (cls == nullptr) {
			std::cout << std::format("line {0}: multiple definition of identifier {1}\n", clsNode->token.lineId, clsNode->token.strData);
			return false;
        }
        cls->nodes.push_back(clsNode);

        // set up access type
        {
            IdenAccessType acc = clsNode->access;
    		if (std::get<1>(createRes)) cls->access = acc;
    		else {
    			if (cls->access != acc) {
    				std::cout << std::format("line {0}: conflict of access type of class {1}\n", clsNode->token.lineId, cls->fullName);
    				return false;
    			}
    		}
        }
        
        // setup generic template list
        {
            if (std::get<1>(createRes)) {
                if (clsNode->tmplList.size() != cls->generic.size()) goto Error_SetupGeneric;
                for (int i = 0; i < clsNode->tmplList.size(); i++) {
                    const std::string &tmplName = clsNode->tmplList[i]->token.strData;
                    if (tmplName != cls->generic[i].first) goto Error_SetupGeneric;
                }
            } else {
                cls->generic.resize(clsNode->tmplList.size());
                for (int i = 0; i < clsNode->tmplList.size(); i++) {
                    const std::string &tmplName = clsNode->tmplList[i]->token.strData;
                    Class *generCls = new Class();
                    generCls->isGeneric = true;
                    generCls->parent = cls;
                    generCls->name = tmplName;
                    cls->generic[i] = std::make_pair(tmplName, generCls);
                }
            }
            goto End_SetupGeneric;

            Error_SetupGeneric:
            std::cout << std::format("line {0}: conflict of generic class definition of class \"{1}\"\n", clsNode->token.lineId, cls->fullName);
            return false;
        }
        End_SetupGeneric:
        // do not analysis other part
        return true;
	}

	void scanCls(IdenEnvironment *idenEnv, Namespace *glo, NspNode *nspNode) {
		auto path = split(nspNode->token.strData, "#");
		Namespace *nsp = createNsp(glo, path);
        nsp->nodes.push_back(nspNode);
        
	}
	IdenEnvironment *build(Namespace *glo, const std::vector<CplNode *> roots) {
        IdenEnvironment *idenEnv = new IdenEnvironment;
        idenEnv->setGloNsp(glo);
        // build class nodes
		for (CplNode *root : roots) {
			if (root->type != CplNodeType::SrcRoot || root->type != CplNodeType::SymRoot)
				continue;
			BlkNode *blk = (BlkNode *)root;
			
		}
        // build class trees
        // build function and variables
		return idenEnv;
    }
}