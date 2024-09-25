#pragma once

#include "tokenize.hpp"
#include "../lib/file.hpp"

#include <map>

struct CompilePackage {
	File_ObjHeader objHdr;

	std::map<std::string, File_FuncDesc *> func;
	std::map<std::string, File_GloDesc *> glo;
	std::vector<File_RefDesc *> ref;

	std::vector< std::tuple<HInstHdr *, u64> > inst;
	std::vector< std::tuple<u64, u64, u8> > gloData;
	std::map<std::string, u64> labels;
	u64 curCodeSize, curGloSize;

	u8 *gloRawData;
	u8 *codeRawData;

	u64 isFromFile;

	CompilePackage();
	~CompilePackage();
};

struct RelyPackage {
	File_ExecHeader execHdr;

	std::string path;
	int isRel;

	std::map<std::string, File_FuncDesc *> func;
	std::map<std::string, File_GloDesc *> glo;

	RelyPackage();
	~RelyPackage();
};

CompilePackage *Hasm_compile(const std::vector<Hasm_Token> &tokens);

CompilePackage *Hasm_readCplPkg(const std::string &filePath);

void Hasm_writeCplPkg(const std::string &filePath, CompilePackage *pkg);

RelyPackage *Hasm_readRelyPkg(const std::string &relyPath);

int Hasm_link(const std::string &execPath, const std::vector<std::string> &objPath, const std::vector<std::string> &relyPath);

