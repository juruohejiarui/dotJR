#include "desc.hpp"
#include <format>
#include <iostream>

namespace IdenSystem {
    // set the parent of IDEN and modify the fullname
    static void setParent(Iden *iden, Iden *parent) {
        iden->parent = parent;
        iden->fullName = std::format("{0}#{1}", parent->fullName, iden->name);
    }

	ExprTypePtr getExprType(TypeNode *typeNode) {

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

	// return 
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

	bool analyClsNode(IdenEnvironment *idenEnv, ClsNode *clsNode) {
		auto createRes = createCls(idenEnv->getCurNsp(), clsNode->token.strData);
		Class *cls = std::get<0>(createRes);
		if (cls == nullptr) {
			std::cout << std::format("line {0}: multiple definition of identifier {1}\n", clsNode->token.lineId, clsNode->token.strData);
			return false;
		}
		IdenAccessType acc = clsNode->access;
		if (std::get<1>(createRes)) cls->access = acc;
		else {
			if (cls->access != acc) {
				std::cout << std::format("line {0}: conflict of access type of class {1}\n", clsNode->token.lineId, cls->fullName);
				return false;
			}
		}
		// get the basic class
		ExprTypePtr bsType = clsNode->bsCls != nullptr ? getExprType(clsNode->bsCls;
		if (std::get<1>(createRes)) cls->bsCls = bsType;
		else 
	}

	void analyNspNode(IdenEnvironment *idenEnv, Namespace *glo, NspNode *nspNode) {
		auto path = split(nspNode->token.strData, "#");
		Namespace *nsp = createNsp(glo, path);
	}
	bool build(Namespace *glo, const std::vector<CplNode *> roots) {
		for (CplNode *root : roots) {
			if (root->type != CplNodeType::SrcRoot || root->type != CplNodeType::SymRoot)
				continue;
			BlkNode *blk = (BlkNode *)root;
			
		}
		return true;
	}
}