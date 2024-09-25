#include "compile.hpp"
#include <cstring>
#include <set>

CompilePackage::CompilePackage() {
	memset(&objHdr, 0, sizeof(objHdr));
	curCodeSize = curGloSize = 0;
	objHdr.mainFuncSymbolId = (u32)-1;
	gloRawData = nullptr;
	codeRawData = nullptr;
	isFromFile = false;
}

CompilePackage::~CompilePackage() {
	for (auto &tup : inst) {
		HInstHdr *hinst = std::get<0>(tup);
		if (hinst != nullptr) free(hinst); 
	}
	if (gloRawData != nullptr) free(gloRawData);
	if (codeRawData != nullptr) free(codeRawData);
	for (auto &desc : ref)
		if (desc != nullptr) {
			free(desc);
			if (isFromFile) break;
		}
	for (auto &fPir : func) 
		if (fPir.second != nullptr) {
			free(fPir.second);
			if (isFromFile) break;
		}
	for (auto &gPir : glo)
		if (gPir.second != nullptr) {
			free(gPir.second);
			if (isFromFile) break;
		}
}

RelyPackage::RelyPackage() {
	memset(&execHdr, 0, sizeof(execHdr));
}

RelyPackage::~RelyPackage() {
	File_FuncDesc *firDesc = (File_FuncDesc *)(u64)-1;
	for (auto &desc : func) if (desc.second != nullptr) {
		firDesc = (File_FuncDesc *)std::min((u64)firDesc, (u64)desc.second);
		break;
	}
	free(firDesc);
	firDesc = (File_FuncDesc *)(u64)-1;
	for (auto &desc : glo) if (desc.second != nullptr) {
		firDesc = (File_FuncDesc *)std::min((u64)firDesc, (u64)desc.second);
		break;
	}
	free(firDesc);
}

static int makeInst(const std::vector<Hasm_Token> &tokens, int fr, int &to, CompilePackage *pkg) {
	HInstHdr hdr;
	u32 extType = -1;
	std::vector<u64> args;
	std::vector<const Hasm_Token *> argTokens;
	memset(&hdr, 0, sizeof(hdr));
	if (!tokens[fr].data) {
		printf("line %d: invalid syntax \"%s\"", tokens[fr].lineId, tokens[fr].strData.c_str());
		return 2;
	}
	hdr.cmd = tokens[fr].data - 1;
	int instSize = sizeof(HInstHdr);
	// this instruction has data type
	if (fr + 1 < tokens.size() && tokens[fr + 1].dataType) {
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
	} else {
		hdr.type = BsData_Type_void;
		to = fr;
	}
	for (; to + 1 < tokens.size() && tokens[to + 1].type == Hasm_TokenType::Comma; to += 2) {
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
		// single argument
		if (argTokens.size() == 1) {
			if (argTokens[0]->type == Hasm_TokenType::StrData) {
				// set the value to be zero
				hdr.bsArgFlag.argType = BsData_Type_u64, args[0] = 0;
				// add a reference descriptor
				File_RefDesc *desc = (File_RefDesc *)malloc(sizeof(File_RefDesc) + sizeof(char) * argTokens[0]->strData.size());
				strncpy(desc->fullName, argTokens[0]->strData.c_str(), sizeof(char) * argTokens[0]->strData.size());
				desc->fullNameLen = argTokens[0]->strData.size();
				desc->type = File_UnfinishedDesc_Type_Code;
				desc->codeOffset = pkg->curCodeSize + instSize;
				pkg->ref.push_back(desc);
				// update the header
				pkg->objHdr.refSpaceSize += sizeof(File_RefDesc) + sizeof(char) * argTokens[0]->strData.size();
				pkg->objHdr.refSymbolNum++;
			} else
				hdr.bsArgFlag.argType = argTokens[0]->dataType, args[0] = argTokens[0]->data;
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
					// add a reference descriptor
					File_RefDesc *desc = (File_RefDesc *)malloc(sizeof(File_RefDesc) + sizeof(char) * tkPtr->strData.size());
					desc->fullNameLen = tkPtr->strData.size();
					desc->type = File_UnfinishedDesc_Type_Code;
					desc->codeOffset = pkg->curCodeSize + instSize;
					strncpy(desc->fullName, tkPtr->strData.c_str(), desc->fullNameLen);
					// update the header
					pkg->objHdr.refSpaceSize += sizeof(File_RefDesc) + sizeof(char) * tkPtr->strData.size();
					pkg->objHdr.refSymbolNum++;
					args[i] = 0ul;
				} else
					args[i] = tkPtr->data;
				instSize += argSize;
			}
		}
	}
	if (instSize != HInst_getSize(&hdr)) {
		printf("line %d: %08x size:%d correct:%d\n", tokens[fr].lineId, *(int *)&hdr, instSize, HInst_getSize(&hdr));
		return 1;
	}
	// write it to memory
	HInstHdr *hinst = (HInstHdr *)malloc(instSize);
	memcpy(hinst, &hdr, sizeof(HInstHdr));
	if (extType != -1) HInst_setExtType(hinst, extType);
	printf("%08x size=%d ", *(u32 *)hinst, instSize);
	for (int i = 0; i < args.size(); i++) HInst_setArg(hinst, i, args[i]), printf("arg[%d]:%#018lx\t", i, args[i]);
	putchar('\n');

	pkg->inst.push_back(std::make_tuple(hinst, instSize));
	pkg->curCodeSize += instSize;
	return 0;
}

static int makeDefine(const std::vector<Hasm_Token> &tokens, int fr, int &to, CompilePackage *pkg) {
	for (int st = to = fr; ; st = ++to) {
		auto &fir = tokens[st];
		if (fir.type != Hasm_TokenType::Define) {
			printf("line %d: invalid syntax.\n", fir.lineId);
			return 0x2;
		}
		if ((Hasm_DefineType)fir.data == Hasm_DefineType::EndDefine) break;
		switch ((Hasm_DefineType)fir.data) {
			// a data definition
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
					// add a reference descriptor
					File_RefDesc *desc = (File_RefDesc *)malloc(sizeof(File_RefDesc) + sizeof(char) * sec.strData.size());
					desc->fullNameLen = sec.strData.size();
					desc->type = File_UnfinishedDesc_Type_Glob;
					strncpy(desc->fullName, sec.strData.c_str(), desc->fullNameLen);
					desc->descOffset = pkg->curGloSize;
					// update the header
					pkg->objHdr.refSpaceSize += sizeof(File_RefDesc) + sizeof(char) * sec.strData.size();
					pkg->objHdr.refSymbolNum++;
					pkg->ref.push_back(desc);
				} else if (sec.type == Hasm_TokenType::BsData) {
					// add a tuple for this data
					pkg->gloData.push_back(std::make_tuple(pkg->curGloSize, sec.data, (u8)fir.data));
				} else {
					printf("line %d: invalid syntax\n");
					return 0x2;
				}
				pkg->curGloSize += size;
				break;
			}
			// a zero data block definition
			case Hasm_DefineType::Fill: {
				auto &sec = tokens[++to];
				if (sec.type != Hasm_TokenType::BsData) {
					u64 size = sec.data;
					pkg->gloData.push_back(std::make_tuple(pkg->curGloSize, size, (u8)fir.data));
					pkg->curGloSize += size;
				} else {
					printf("line %d: invalid syntax\n");
					return 0x2;
				}
				break;
			}
			// a global symbol definition
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
				desc->id = pkg->func.size();
				desc->offset = pkg->curGloSize;
				pkg->glo[name] = desc;
				// update header
				pkg->objHdr.gloSpaceSize += sizeof(File_GloDesc) + name.size() * sizeof(char);
				pkg->objHdr.gloSymbolNum++;
				break;
			}
			// a function symbol definition
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
				desc->id = pkg->func.size();
				desc->offset = (u32)-1;
				pkg->func[name] = desc;
				// update header
				pkg->objHdr.funcSpaceSize += sizeof(File_FuncDesc) + name.size() * sizeof(char);
				pkg->objHdr.funcSymbolNum++;
				break;
			}
		}
	}
	return 0;
}

// modify the function descriptor using the label dictionary
static int modifyFuncDesc(CompilePackage *pkg) {
	for (auto &fPir : pkg->func) {
		auto iter = pkg->labels.find(fPir.first);
		// the label of this function is not defined in this file
		if (iter == pkg->labels.end()) continue;
		File_FuncDesc *func = fPir.second;
		func->offset = iter->second;
		if (fPir.first == "Main") pkg->objHdr.mainFuncSymbolId = func->id;
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
					printf("line %d: multiple definition of label \"%s\"\n", lbl.lineId, name.c_str());
					res |= 0x1;
					break;
				}
				pkg->labels.insert(std::make_pair(name, pkg->curCodeSize));
				pkg->objHdr.lblSpaceSize += sizeof(File_LabelDesc) + name.size();
				pkg->objHdr.lblSymbolNum++;
				break;
			}
			default :
				printf("line %d: invalid syntax\n", tokens[i].lineId);
				res |= 0x2;
				break;
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

	res = modifyFuncDesc(pkg);
	pkg->objHdr.codeLen = pkg->curCodeSize;
	pkg->objHdr.gloLen = pkg->curGloSize;

	pkg->gloRawData = (u8 *)calloc(1, pkg->objHdr.gloLen);
	pkg->codeRawData = (u8 *)calloc(1, pkg->objHdr.codeLen);
	// make the global raw data
	for (auto &gloData : pkg->gloData) {
		const u64 offset = std::get<0>(gloData), data = std::get<1>(gloData);
		const u8 type = std::get<2>(gloData);
		switch ((Hasm_DefineType)type) {
			case Hasm_DefineType::Byte :
				pkg->gloRawData[offset] = *(u8 *)&data;
				break;
			case Hasm_DefineType::Word :
				*(u16 *)&pkg->gloRawData[offset] = *(u16 *)&data;
				break;
			case Hasm_DefineType::Dword :
				*(u32 *)&pkg->gloRawData[offset] = *(u32 *)&data;
				break;
			case Hasm_DefineType::Quad :
				*(u64 *)&pkg->gloRawData[offset] = *(u64 *)&data;
		}
	}
	// make code raw data
	u64 offset = 0;
	for (auto &codeData : pkg->inst) {
		const HInstHdr *hdr = std::get<0>(codeData);
		const u64 instSize = std::get<1>(codeData);
		memcpy(pkg->codeRawData + offset, hdr, instSize);
		offset += instSize;
	}
	return pkg;
}

CompilePackage *Hasm_readCplPkg(const std::string &filePath) {
	FILE *file = fopen(filePath.c_str(), "rb");
	CompilePackage *pkg = new CompilePackage();
	pkg->isFromFile = 1;
	fread(&pkg->objHdr, sizeof(File_ObjHeader), 1, file);

	// read the global descriptor table
	File_GloDesc *gloDesc = (File_GloDesc *)malloc(pkg->objHdr.gloSpaceSize);
	fread(gloDesc, pkg->objHdr.gloSpaceSize, 1, file);
	for (File_GloDesc *cur = gloDesc; (u64)cur - (u64)gloDesc < pkg->objHdr.gloSpaceSize; cur = File_nextDesc(cur, File_GloDesc)) {
		auto fullName = File_FullNameStr(cur->fullName, cur->fullNameLen);
		pkg->glo[fullName] = cur;
	}

	// read the function descriptor table
	File_FuncDesc *funcDesc = (File_FuncDesc *)malloc(pkg->objHdr.funcSpaceSize);
	fread(funcDesc, pkg->objHdr.funcSpaceSize, 1, file);
	for (File_FuncDesc *cur = funcDesc; (u64)cur - (u64)funcDesc < pkg->objHdr.funcSpaceSize; cur = File_nextDesc(cur, File_FuncDesc)) {
		auto fullName = File_FullNameStr(cur->fullName, cur->fullNameLen);
		pkg->func[fullName] = cur;
	}
	
	File_LabelDesc *lblDesc = (File_LabelDesc *)malloc(pkg->objHdr.lblSpaceSize);
	fread(lblDesc, pkg->objHdr.lblSpaceSize, 1, file);
	for (File_LabelDesc *cur = lblDesc; (u64)cur - (u64)lblDesc < pkg->objHdr.lblSpaceSize; cur = File_nextDesc(cur, File_LabelDesc)) {
		auto fullName = File_FullNameStr(cur->fullName, cur->fullNameLen);
		pkg->labels[fullName] = cur->offset;
	}
	free(lblDesc);

	// read the reference descriptor table
	File_RefDesc *refDesc = (File_RefDesc *)malloc(pkg->objHdr.refSpaceSize);
	fread(refDesc, pkg->objHdr.refSpaceSize, 1, file);
	for (File_RefDesc *cur = refDesc; (u64)cur - (u64)refDesc < pkg->objHdr.refSpaceSize; cur = File_nextDesc(cur, File_RefDesc)) {
		pkg->ref.push_back(cur);
	}

	// read the code raw data
	pkg->codeRawData = (u8 *)malloc(pkg->objHdr.codeLen);
	fread(pkg->codeRawData, pkg->objHdr.codeLen, 1, file);

	// read the global raw data
	pkg->gloRawData = (u8 *)malloc(pkg->objHdr.gloLen);
	fread(pkg->gloRawData, pkg->objHdr.gloLen, 1, file);
	return pkg;
}

void Hasm_writeCplPkg(const std::string &filePath, CompilePackage *pkg) {
	FILE *file = fopen(filePath.c_str(), "wb");
	fwrite(&pkg->objHdr, sizeof(File_ObjHeader), 1, file);

	for (auto &gPir : pkg->glo)
		fwrite(gPir.second, File_descLen(gPir.second), 1, file);
	for (auto &fPir : pkg->func)
		fwrite(fPir.second, File_descLen(fPir.second), 1, file);
	for (auto &lPir : pkg->labels) {
		u64 size = sizeof(File_LabelDesc) + sizeof(char) * lPir.first.size();
		File_LabelDesc *desc = (File_LabelDesc *)malloc(size);
		desc->fullNameLen = lPir.first.size();
		desc->offset = lPir.second;
		strncpy(desc->fullName, lPir.first.c_str(), desc->fullNameLen);
		fwrite(desc, size, 1, file);
		free(desc);
	}
	for (auto &ref : pkg->ref) 
		fwrite(ref, File_descLen(ref), 1, file);
	
	fwrite(pkg->codeRawData, pkg->objHdr.codeLen, 1, file);
	fwrite(pkg->gloRawData, pkg->objHdr.gloLen, 1, file);
	fclose(file);
}

RelyPackage *Hasm_readRelyPkg(const std::string &relyPath) {
	FILE *file = fopen(relyPath.c_str(), "rb");
	RelyPackage *pkg = new RelyPackage();
	fread(&pkg->execHdr, sizeof(File_ExecHeader), 1, file);
	File_GloDesc *gloDesc = (File_GloDesc *)malloc(pkg->execHdr.gloSpaceSize);
	fread(gloDesc, pkg->execHdr.gloSpaceSize, 1, file);
	for (File_GloDesc *cur = gloDesc; (u64)cur - (u64)gloDesc < pkg->execHdr.gloSpaceSize; cur = File_nextDesc(cur, File_GloDesc)) {
		auto fullName = File_FullNameStr(cur->fullName, cur->fullNameLen);
		pkg->glo[fullName] = cur;
	}

	File_FuncDesc *funcDesc = (File_FuncDesc *)malloc(pkg->execHdr.funcSpaceSize);
	fread(funcDesc, pkg->execHdr.funcSpaceSize, 1, file);
	for (File_FuncDesc *cur = funcDesc; (u64)cur - (u64)funcDesc < pkg->execHdr.funcSpaceSize; cur = File_nextDesc(cur, File_FuncDesc)) {
		auto fullName = File_FullNameStr(cur->fullName, cur->fullNameLen);
		pkg->func[fullName] = cur;
	}

	fclose(file);
	return pkg;
}

int Hasm_link(const std::string &execPath, const std::vector<std::string> &objPath, const std::vector<std::string> &relyPath) {
	std::vector<CompilePackage *> cplPkg;
	std::vector<RelyPackage *> rely;
	cplPkg.resize(objPath.size());
	rely.resize(relyPath.size());
	for (int i = 0; i < objPath.size(); i++) cplPkg[i] = Hasm_readCplPkg(objPath[i]);
	for (int i = 0; i < relyPath.size(); i++) rely[i] = Hasm_readRelyPkg(relyPath[i]);
	
	File_ExecHeader hdr;
	memset(&hdr, 0, sizeof(File_ExecHeader));
	hdr.mainFuncSymbolId = (u32)-1;

	auto getRefData = [&](const std::string &refName, CompilePackage *curPkg, int isCode) -> std::tuple<u64, bool> {
		// search on the function list of PKG
		for (auto pkg : cplPkg) {
			{
				auto iter = pkg->func.find(refName);
				if (iter == pkg->func.end()) continue;
				return std::make_tuple(0 | iter->second->id, true);
			}
		}
		// then the label list
		if (isCode) {
			auto iter = curPkg->labels.find(refName);
			if (iter != curPkg->labels.end())
				return std::make_tuple(iter->second, true);
		}
		// search on the global list of PKG
		for (auto pkg : cplPkg) {
			auto iter = pkg->glo.find(refName);
			if (iter == pkg->glo.end()) continue;
			return std::make_tuple(0 | (u64)iter->second->id, true);
		}
		// search on the rely list
		u64 relyId = 0;
		for (auto pkg : rely) {
			relyId++;
			{
				auto iter = pkg->func.find(refName);
				if (iter != pkg->func.end())
					return std::make_tuple((relyId << 32) | iter->second->id, true);
			}
			{
				auto iter = pkg->glo.find(refName);
				if (iter != pkg->glo.end()) 
					return std::make_tuple((relyId << 32) | iter->second->id, true);
			}
		}
		return std::make_tuple(0, false);
	};

	// modify the offset of descriptors (excepts reference descriptors) in compile package
	std::set<std::string> fullNameSet;
	for (auto pkg : cplPkg) {
		for (auto &fPir : pkg->func) {
			if (fullNameSet.count(fPir.first)) {
				printf("Link error: multiple definition of symbol \"%s\"\n", fPir.first.c_str());
				return 1;
			}
			fullNameSet.insert(fPir.first);
			fPir.second->offset += hdr.codeLen;
			fPir.second->id += hdr.funcSymbolNum;
		}
		for (auto &gPir : pkg->glo) {
			if (fullNameSet.count(gPir.first)) {
				printf("Link error: multiple definition of symbol \"%s\"\n", gPir.first.c_str());
				return 1;
			}
			fullNameSet.insert(gPir.first);
			gPir.second->offset += hdr.gloLen;
			gPir.second->id += hdr.gloSymbolNum;
		}
		// update the main function symbol id
		if (pkg->objHdr.mainFuncSymbolId != (u32)-1)
			hdr.mainFuncSymbolId = pkg->objHdr.mainFuncSymbolId + hdr.funcSymbolNum;
		// update the header
		hdr.funcSpaceSize += pkg->objHdr.funcSpaceSize;
		hdr.funcSymbolNum += pkg->objHdr.funcSymbolNum;
		hdr.gloSpaceSize += pkg->objHdr.gloSpaceSize;
		hdr.gloSymbolNum += pkg->objHdr.gloSymbolNum;
		hdr.codeLen += pkg->objHdr.codeLen;
		hdr.gloLen += pkg->objHdr.gloLen;
	}
	// finish ref data
	for (auto pkg : cplPkg) {
		for (auto &ref : pkg->ref) {
			std::string fullName = File_FullNameStr(ref->fullName, ref->fullNameLen);
			auto data = getRefData(fullName, pkg, ref->type == File_UnfinishedDesc_Type_Code);
			if (!std::get<1>(data)) {
				printf("Link error: can not find refered data \"%s\"\n", fullName.c_str());
				return 1;
			}
			switch (ref->type) {
				case File_UnfinishedDesc_Type_Code :
					*(u64 *)(pkg->codeRawData + ref->codeOffset) = std::get<0>(data);
					break;
				case File_UnfinishedDesc_Type_Glob :
					*(u64 *)(pkg->gloRawData + ref->descOffset) = std::get<0>(data);
					break;
			}
		}
	}

	// write down the data
	FILE *file = fopen(execPath.c_str(), "wb");
	fwrite(&hdr, sizeof(File_ExecHeader), 1, file);
	// write global descriptor
	for (auto pkg : cplPkg)
		if (pkg->glo.size() > 0) fwrite(pkg->glo.begin()->second, pkg->objHdr.gloSpaceSize, 1, file);
	// write function descriptor
	for (auto pkg : cplPkg)
		if (pkg->func.size() > 0) fwrite(pkg->func.begin()->second, pkg->objHdr.funcSpaceSize, 1, file);
	// write rely descriptor
	int relyId = 0;
	for (auto rely : relyPath) {
		u64 size = sizeof(File_RelyDesc) + rely.size();
		File_RelyDesc *desc = (File_RelyDesc *)malloc(size);
		desc->fullNameLen = rely.size();
		strncpy(desc->fullName, rely.c_str(), rely.size());
		desc->relyId = ++relyId;
		fwrite(desc, size, 1, file);
		free(desc);
	}
	// write code
	for (auto pkg : cplPkg)
		fwrite(pkg->codeRawData, pkg->objHdr.codeLen, 1, file);
	for (auto pkg : cplPkg)
		fwrite(pkg->gloRawData, pkg->objHdr.gloLen, 1, file);

	for (auto pkg : cplPkg) delete pkg;
	for (auto pkg : rely) delete pkg;

 	fclose(file);
	return 0;
}