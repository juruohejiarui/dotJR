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
            if (res.size() < 1 || res[0]->type != IdenType::Cls) {
                std::cout << std::format("line {0}: Cannot find class {1}\n", typeNode->token.lineId, typeNode->token.strData);
                return nullptr;
            }
            type->cls = (Class *)res[0];
            if (typeNode->attr & TypeNode_Attr_hasGener) {
                type->substList.resize(typeNode->params.size());
                for (size_t i = 0; i < type->substList.size(); i++)
                    if ((type->substList[i] = cvtToExprType(idenEnv, typeNode->params[i])) == nullptr)
                        return nullptr;
            }
            if (type->cls->generic.size() != type->substList.size()) {
                std::cout << std::format("line {0}: Invalid number of substitutions for class {1}.\n", typeNode->token.lineId, type->cls->fullName);
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
            cls->isBaseType = true;
            setParent(cls, glo);
            glo->child.insertChild(cls);
        }
        return glo;
	}

	// create a namespace list using PATH, return nullptr if one identifier of the path is used by other types
	Namespace *createNsp(Namespace *glo, const std::vector<std::string> &path) {
        if (path.size() == 0) return glo;
		Namespace *cur = glo;
		for (int i = 0; i < path.size(); i++) {
			std::vector<Iden *> idens = cur->child.getChildren(path[i]);
			if (idens.size() == 0) {
				Namespace *nsp = new Namespace();
				nsp->name = path[i], setParent(nsp, cur);
				cur->child.insertChild(nsp);
				nsp->access = IdenAccessType::Public;
                cur = nsp;
			// it is used by other types of identifiers
			} else {
                if (idens.size() > 1 || !allSpecType(idens, IdenType::Nsp)) return nullptr;
			    cur = (Namespace *)idens[0];
            }
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
            if (!std::get<1>(createRes)) {
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
                    generCls->name = tmplName;
                    setParent(generCls, cls);
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

    

	bool scanCls(IdenEnvironment *idenEnv, NspNode *nspNode) {
		auto path = split(nspNode->token.strData, "#");
		Namespace *nsp = createNsp(idenEnv->getGloNsp(), path), *prev = idenEnv->getCurNsp();
        if (nsp == nullptr) {
            std::cout << std::format("line {0}: Failed to create namespace {1}, becaus of multiple definition of this identifier.\n",
                nspNode->token.lineId, nspNode->token.strData);
            return false;
        }
        nsp->nodes.push_back(nspNode);
        bool res = true;

        idenEnv->setCurNsp(nsp);

        for (ClsNode *clsNode : nspNode->cls)
            res &= buildClsNode(idenEnv, clsNode);

        idenEnv->setCurNsp(prev);

        return res;
	}

    bool scanUsg(Namespace *glo, Namespace *nsp) {
        auto res = true;
        
        for (NspNode *node : nsp->nodes) {
            for (UsingNode *usg : node->usng) {
                auto path = split(usg->token.strData, "#");
                Namespace *cur = glo;
                for (auto &part : path) {
                    auto searchRes = cur->child.getChildren(part);
                    if (searchRes.size() != 1 || !allSpecType(searchRes, IdenType::Nsp)) {
                        std::cout << std::format("line {0}: failed to build shortcut {1}\n", usg->token.lineId, usg->token.strData);
                        res = false;
                        cur = nullptr;
                        break;
                    }
                    cur = (Namespace *)searchRes[0];
                }
                if (cur != nullptr) nsp->child.usgList.push_back(cur);
            }
        }
        for (auto childPair : nsp->child.nsp)
            res &= scanUsg(glo, childPair.second);
        return res;
    }

    bool buildClsGraph(IdenEnvironment *idenEnv, Namespace *cur, std::map<Class *, std::vector<Class *> > &graph) {
        idenEnv->setCurNsp(cur);
        bool res = true;
        for (auto &clsPir : cur->child.cls) {
            Class *cls = clsPir.second;
            if (cls->isBaseType) continue;
            idenEnv->setCurCls(cls);
            // get the first definition node to parse the base class expression
            for (ClsNode *node : cls->nodes) {
                ExprTypePtr_Normal chkTemp = std::make_shared<ExprType_Normal>();
                auto &tg = (cls->bsCls == nullptr ? cls->bsCls : chkTemp);
                auto succ = true;
                if (node->bsCls == nullptr) {
                    ExprTypePtr_Normal objType = std::make_shared<ExprType_Normal>();
                    objType->cls = idenEnv->getGloNsp()->child.cls["object"];
                    cls->bsCls = objType;
                } else {
                    ExprTypePtr bsType_tmp = cvtToExprType(idenEnv, cls->nodes[0]->bsCls);
                    if (bsType_tmp->category != ExprTypeCategory::Normal) {
                        printf("line {0}: base class type must be normal type\n", cls->nodes[0]->bsCls->token.lineId);
                        succ = false;
                        break;
                    }
                    tg = dynCastPtr<ExprType, ExprType_Normal>(bsType_tmp);
                }
                if (succ) {
                    if (cls->bsCls != nullptr) succ &= ExprType::equal(cls->bsCls, chkTemp);
                } else res = false;
            }
            graph[cls->bsCls->cls].push_back(cls);
        }
        for (auto &nspPir : cur->child.nsp) {
            Namespace *nsp = nspPir.second;
            res &= buildClsGraph(idenEnv, nsp, graph);
        }
        return res;
    }

    bool analyClsGraph(IdenEnvironment *idenEnv, Class *cls) {
        idenEnv->setCurCls(cls);
        idenEnv->setCurNsp((Namespace *)cls->parent);
		bool succ = true;

        for (ClsNode *clsNode : cls->nodes) {
			// the i-th offset record the offset of a variable with size=2^i
			u64 offset[4] = {0, 0, 0, 0};
			// build the variable list
            for (VarDefNode *varDefBlk : clsNode->var) {
                IdenAccessType access = varDefBlk->access;
                for (VarNode *varDef : varDefBlk->vars) {
                    const std::string &name = varDef->token.strData;
					Variable *var = new Variable();
					var->node = varDef;
					var->access = access;
					setParent(var, cls);
                    {
                        auto searchRes = cls->child.getChildren(name);
                        if (searchRes.size() > 0) {
							if (searchRes.size() > 1 || !allSpecType(searchRes, IdenType::Var) || ((Variable *)searchRes[0])->oriVar->parent == cls) {
								std::cout << std::format("line {0}: invalid operation: multiple definition of identifier: {1}\n",
									varDef->token.lineId, name);
								succ = false;
								break;
							}
						}
						
                    }
					// override the variable identifier if it is from parent
					// or create a new variable identifier
					cls->child.var[name] = var; 
					// parse the variable type, calculate the offset and size of this variable
					if (varDef->varType == nullptr) {
						std::cout << std::format("line {0}: class member must define type.\n", varDef->token.lineId);
						succ = false;
						break;
					}
					if (varDef->initExpr != nullptr) {
						std::cout << std::format("line {0}: no support for initlization of this style.\n", varDef->token.lineId);
						succ = false;
						break;
					}
					var->varType = cvtToExprType(idenEnv, varDef->varType);
					if (var->varType == nullptr) {
						std::cout << std::format("line {0}: failed to parse variable type of variable \"{1}\"\n", varDef->varType->token.lineId, var->fullName);
						succ = false;
						break;
					}
					u64 size = std::min(8ull, var->varType->size());
                }
            }
        }
		if (!succ) return false;
        return succ;
    }

	IdenEnvironment *build(Namespace *glo, const std::vector<CplNode *> &roots) {
        IdenEnvironment *idenEnv = new IdenEnvironment;
        idenEnv->setGloNsp(glo);
        idenEnv->setCurNsp(glo);

        bool res = true;
        // build class nodes
		for (CplNode *root : roots) {
            NspNode *nspNode = (NspNode *)root;
			res &= scanCls(idenEnv, nspNode);
            for (auto &nsp : nspNode->nsp) res &= scanCls(idenEnv, nsp);
        }
        if (!res) {
            delete idenEnv;
            return nullptr;
        }

        // build using list and class tree
        res = scanUsg(idenEnv->getGloNsp(), idenEnv->getGloNsp());
        if (!res) { delete idenEnv; return nullptr; }
        std::map<Class *, std::vector<Class *> > graph;
        res = buildClsGraph(idenEnv, idenEnv->getGloNsp(), graph);
        if (!res) { delete idenEnv; return nullptr; }
        // build member function and variables
        res = analyClsGraph(idenEnv, idenEnv->getGloNsp()->child.cls["object"]);
        // build global function and variables
		return idenEnv;
    }
}