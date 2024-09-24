#pragma once

#include "tokenize.hpp"
#include "../lib/file.hpp"

#include <map>

struct CompilePackage {
	File_ObjHeader objHdr;

	std::map<std::string, File_FuncDesc *> func;
	std::map<std::string, File_GloDesc *> glo;
	std::map<std::string, File_RelyDesc *> rely;
	std::vector<File_SymbolUsageDesc *> usage;

	std::vector< std::tuple<HInstHdr *, u64> > inst;
	std::vector< std::tuple<u64, u64, u8> > gloData;
	std::map<std::string, u64> labels;
	u64 curCodeSize, curGloSize;

	CompilePackage();
	~CompilePackage();
};

CompilePackage *Hasm_compile(const std::vector<Hasm_Token> &tokens);

CompilePackage *Hasm_readCplPkg(const std::string &filePath);

CompilePackage *Hasm_link(const std::vector<CompilePackage *> &pkg);

void Hasm_writeExec(const std::string &filePath);