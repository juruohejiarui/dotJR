#include "compile.hpp"
#include <cstring>

CompilePackage::CompilePackage() {
}

CompilePackage::~CompilePackage() {
	for (auto &tup : inst) {
		HInstHdr *hinst = std::get<0>(tup);
		if (hinst != nullptr) free(hinst); 
	}
	for (auto &desc : usage)
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
	for (; tokens[to + 1].type == Hasm_TokenType::Comma; to += 2) {
		// if this is a strData, then add an unfinished descriptor to the package
		// otherwise, just add this argument to the list
		argTokens.push_back(&tokens[to + 2]);
		if (tokens[to + 2].type != Hasm_TokenType::BsData && tokens[to + 2].type != Hasm_TokenType::StrData) {
			printf("line %d: invalid syntax for argument list \"%s\"", tokens[to + 2].lineId, tokens[to + 2].strData);
			return 2;
		}
	}
	if (!argTokens.size()) {
		hdr.varArgFlag.isVarNum = 1;
		hdr.varArgFlag.argNum = 0;
	} else {
		args.resize(argTokens.size());
		// signal argument
		if (argTokens.size() == 1) {
			if (argTokens[0]->type == Hasm_TokenType::StrData) {
				// set the value to be zero
				hdr.bsArgFlag.argType = BsData_Type_u64, args[0] = std::make_tuple(0, BsData_Type_void);
				// make symbol usage descriptor
				File_SymbolUsageDesc *desc = (File_SymbolUsageDesc *)malloc(sizeof(File_SymbolUsageDesc) + sizeof(char) * argTokens[0]->strData.size());
				strncpy(desc->fullName, argTokens[0]->strData.c_str(), sizeof(char) * argTokens[0]->strData.size());
				desc->fullNameLen = argTokens[0]->strData.size();
				desc->type = File_UnfinishedDesc_Type_Code;
				desc->codeOffset = pkg->curCodeSize + instSize;
				pkg->usage.push_back(desc);
			} else
				hdr.bsArgFlag.argType = argTokens[0]->dataType, args[0] = std::make_tuple(argTokens[0]->data, argTokens[0]->dataType);				instSize += Lib_getBsDataSize(hdr.bsArgFlag.argType);
			instSize += Lib_getBsDataSize(hdr.bsArgFlag.argType);
		} else {
			hdr.varArgFlag.isVarNum = 1;
			// get max size
			int argSize = 4;
			for (auto tkPtr : argTokens)
				if (tkPtr->type == Hasm_TokenType::StrData || Lib_getBsDataSize(tkPtr->dataType) > argSize)
					argSize = 8, hdr.varArgFlag.isLong = 1;
			for (int i = 0; i < argTokens.size(); i++) {
				auto tkPtr = argTokens[i];
				if (tkPtr->type == Hasm_TokenType::StrData) {
					File_SymbolUsageDesc *desc = (File_SymbolUsageDesc *)malloc(sizeof(File_SymbolUsageDesc) + sizeof(char) * tkPtr->strData.size());
					desc->fullNameLen = tkPtr->strData.size();
					desc->type = File_UnfinishedDesc_Type_Code;
					desc->codeOffset = pkg->curCodeSize + instSize;
					strncpy(desc->fullName, tkPtr->strData.c_str(), desc->fullNameLen);
					args[i] = std::make_tuple(0ul, BsData_Type_void);
				} else
					args[i] = std::make_tuple(tkPtr->data, tkPtr->dataType);
				instSize += argSize;
			}
		}
	}
	if (instSize != HInst_getSize(&hdr)) {
		printf("line %d: %08x\n", tokens[fr].lineId, *(int *)&hdr);
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
	for (int st = to = fr; ; st = ++to) {
		auto &fir = tokens[st];
		if (fir.type != Hasm_TokenType::Define) {
			printf("line %d: invalid syntax\n", fir.lineId);
			return 2;
		}
		if ((Hasm_DefineType)fir.data == Hasm_DefineType::EndDefine) break;
		switch ((Hasm_DefineType)fir.data) {
			case Hasm_DefineType::Byte:
			case Hasm_DefineType::Word:
			case Hasm_DefineType::Dword:
			case Hasm_DefineType::Quad: {
				auto &sec = tokens[++to];
				int size = (1 << ((int)fir.data));
				if (sec.type == Hasm_TokenType::StrData) {
					if (size != 8) {
						printf("line %d: data size for symbol \"%s\" must be 8 bytes\n", sec.lineId, sec.strData.c_str());
						return 0x2;
					}
					// add an unfinished descriptor
					File_SymbolUsageDesc *desc = (File_SymbolUsageDesc *)malloc(sizeof(File_SymbolUsageDesc) + sizeof(char) * sec.strData.size());
					desc->fullNameLen = sec.strData.size();
					desc->type = File_UnfinishedDesc_Type_Glob;
					strncpy(desc->fullName, sec.strData.c_str(), desc->fullNameLen);
				} else if (sec.type == Hasm_TokenType::BsData) {
					// add a tuple for this data
					pkg->gloData.push_back(std::make_tuple(pkg->curGloSize, sec.data, (u8)fir.type));
				} else {
					printf("line %d: invalid syntax\n");
					return 0x2;
				}
				pkg->curGloSize += size;
				break;
			}
			case Hasm_DefineType::Fill: {
				auto &sec = tokens[++to];
				if (sec.type != Hasm_TokenType::BsData) {
					u64 size = sec.data;
					pkg->gloData.push_back(std::make_tuple(pkg->curGloSize, size, (u8)fir.type));
					pkg->curGloSize += size;
				} else {
					printf("line %d: invalid syntax\n");
					return 0x2;
				}
				break;
			}
			case Hasm_DefineType::Global: {
				// make a global descriptor
				auto &sec = tokens[++to];
				if (sec.type != Hasm_TokenType::StrData) {
					printf("line %d: invalid syntax\n");
					return 0x2;
				}
				auto &name = sec.strData;
				if (pkg->glo.count(name)) {
					printf("line %d: multiple definition of global symbol \"%s\"\n", sec.lineId, name.c_str());
					return 0x1;
				}
				File_GloDesc *desc = (File_GloDesc *)malloc(sizeof(File_GloDesc) + name.size() * sizeof(char));
				desc->fullNameLen = name.size();
				strncpy(desc->fullName, name.c_str(), desc->fullNameLen);
				desc->attribute = 0;
				desc->inner.id = pkg->func.size();
				desc->inner.offset = pkg->curGloSize;
				pkg->glo[name] = desc;
				break;
			}
			case Hasm_DefineType::Func : {
				// make a function descriptor
				auto &sec = tokens[++to];
				if (sec.type != Hasm_TokenType::StrData) {
					printf("line %d: invalid syntax\n");
					return 1;
				}
				auto &name = sec.strData;
				if (pkg->func.count(name)) {
					printf("line %d: multiple definition of function symbol \"%s\"\n", sec.lineId, name.c_str());
					return 0x1;
				}
				File_FuncDesc *desc = (File_FuncDesc *)malloc(sizeof(File_FuncDesc) + name.size() * sizeof(char));
				desc->fullNameLen = name.size();
				strncpy(desc->fullName, name.c_str(), name.size() * sizeof(char));
				desc->attribute = 0;
				desc->inner.id = pkg->func.size();
				desc->inner.offset = (u32)-1;
				pkg->func[name] = desc;
				break;
			}
		}
	}
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
			case Hasm_TokenType::Label : {
				auto &lbl = tokens[i];
				auto &name = lbl.strData;
				if (pkg->labels.count(name)) {
					printf("line %d: multiple definition of label\"%s\"\n", lbl.lineId, name.c_str());
					res |= 0x1;
					break;
				}
				pkg->labels.insert(std::make_pair(name, pkg->curCodeSize));
			}
		}
		if (res & 0x2) {
			delete pkg;
			return nullptr;
		}
	}
	if (res) {
		delete pkg;
		return nullptr;
	}
	return pkg;
}
