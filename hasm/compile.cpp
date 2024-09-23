#include "compile.hpp"
#include <cstring>

CompilePackage::CompilePackage() {
}

CompilePackage::~CompilePackage() {
	for (auto &tup : inst) {
		HInstHdr *hinst = std::get<0>(tup);
		if (hinst != nullptr) free(hinst); 
	}
	for (auto &desc : unf)
		if (desc != nullptr) free(desc);
	for (auto &fPir : func) 
		if (fPir.second != nullptr) free(fPir.second);
	for (auto &gPir : glo)
		if (gPir.second != nullptr) free(gPir.second);
	for (auto &rPir : rely)
		if (rPir.second != nullptr) free(rPir.second);
}

static int makeInst(const std::vector<Hasm_Token> &tokens, int fr, int &to, CompilePackage *pkg) {
	HInstHdr hdr;
	u32 extType = 0;
	std::vector< std::tuple<u64, u8> > args;
	std::vector<const Hasm_Token *> argTokens;
	memset(&hdr, 0, sizeof(hdr));
	if (!tokens[fr].data) {
		printf("line %d: invalid syntax \"%s\"", tokens[fr].lineId, tokens[fr].strData.c_str());
		return 2;
	}
	hdr.cmd = tokens[fr].data - 1;
	int instSize = sizeof(HInstHdr);
	// this instruction has data type
	if (tokens[fr + 1].dataType) {
		hdr.type = tokens[fr + 1].dataType - 1;
		to = fr + 1;
		// need another token with type==Data to be the type Id
		if (hdr.type == BsData_Type_generic) {
			if (tokens[fr + 2].type != Hasm_TokenType::BsData) {
				printf("line %d: instruction with generic flag requires a type", tokens[fr].lineId);
				return 2;
			}
			to = fr + 2;
			extType = tokens[to].data;
			instSize += sizeof(u8);
		}
	} else to = fr;
	for (; tokens[to].type == Hasm_TokenType::Comma; to += 2) {
		// if this is a strData, then add an unfinished descriptor to the package
		// otherwise, just add this argument to the list
		argTokens.push_back(&tokens[to + 1]);
		if (tokens[to + 1].type != Hasm_TokenType::BsData || tokens[to + 1].type != Hasm_TokenType::StrData) {
			printf("line %d: invalid syntax for argument list \"%s\"", tokens[to + 1].lineId, tokens[to + 1].strData);
			return 2;
		}
	}
	args.resize(argTokens.size());
	// signal argument
	if (argTokens.size() == 1) {
		if (argTokens[0]->type == Hasm_TokenType::StrData) {
			hdr.bsArgFlag.argType = BsData_Type_u64, args[0] = std::make_tuple(0, BsData_Type_void);
			// make unfinished symbol descriptor
			File_UnfinishedDesc *desc = (File_UnfinishedDesc *)malloc(sizeof(File_UnfinishedDesc) + sizeof(char) * argTokens[0]->strData.size());
			strncpy(desc->fullName, argTokens[0]->strData.c_str(), sizeof(char) * argTokens[0]->strData.size());
			desc->fullNameLen = argTokens[0]->strData.size();
			desc->type = File_UnfinishedDesc_Type_Code;
			desc->codeOffset = pkg->curCodeSize + instSize;
			pkg->unf.push_back(desc);
			instSize += Lib_getBsDataSize(hdr.bsArgFlag.argType);
		} else
			hdr.bsArgFlag.argType = argTokens[0]->dataType, args[0] = std::make_tuple(argTokens[0]->data, argTokens[0]->dataType);
	} else {
		// get max size
		int argSize = 4;
		for (auto tkPtr : argTokens)
			if (tkPtr->type == Hasm_TokenType::StrData || Lib_getBsDataSize(tkPtr->dataType) > argSize)
				argSize = 8;
		for (auto tkPtr : argTokens) {
			if (tkPtr->type == Hasm_TokenType::StrData) {
				
			} else {

			}
		}
	}
	// write it to memory
	HInstHdr *hinst = (HInstHdr *)malloc(HInst_getSize(&hdr));
	memcpy(hinst, &hdr, sizeof(HInstHdr));
	HInst_setExtType(hinst, extType);
	
	pkg->inst.push_back(std::make_tuple(hinst, instSize));
	pkg->curCodeSize += instSize;
	return 0;
}

static int makeDefine(const std::vector<Hasm_Token> &tokens, int fr, int &to, CompilePackage *pkg) {
	return 0;
}

CompilePackage *Hasm_compile(const std::vector<Hasm_Token> &tokens) {
	CompilePackage *pkg = new CompilePackage();
	int res = 0;
	for (int i = 0, tokenLen = tokens.size(); i < tokenLen; i++) {
		switch (tokens[i].type) {
			case Hasm_TokenType::StrData :
				res |= makeInst(tokens, i, i, pkg);
				break;
			case Hasm_TokenType::Define :
				res |= makeDefine(tokens, i, i, pkg);
				break;
		}
		if (res & 0x2) delete pkg;
		return nullptr;
	}
	if (res) {
		delete pkg;
		return nullptr;
	}
	return pkg;
}
