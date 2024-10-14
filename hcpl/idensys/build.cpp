#include "desc.hpp"
#include <format>

namespace IdenSystem {
    // set the parent of IDEN and modify the fullname
    static void setParent(Iden *iden, Iden *parent) {
        iden->parent = parent;
        iden->fullName = std::format("{0}#{1}", parent->fullName, iden->name);
    }
    Namespace *buildGloNsp() {
        Namespace *glo = new Namespace();
        glo->access = IdenAccessType::Public;

        const std::vector< std::pair<std::string, size_t> > baseClassName
            = { {"int", 4}, {"long", 8}, {"short", 2}, {"char", 1},
                {"uint", 4}, {"ulong", 8}, {"ushort", 2}, {"uchar", 1}};

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
}