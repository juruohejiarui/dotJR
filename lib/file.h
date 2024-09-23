#ifndef __LIB_FILE_H__
#define __LIB_FILE_H__

#include "bstype.h"

typedef struct File_ObjHeader {

	u32 gloSymbolNum;
	u32 funcSymbolNum;
	u32 relySymbolNum;
	u32 unfinishedSymbolNum;
	
	u64 gloSpaceSize;
	u64 funcSpaceSize;
	u64 relySpaceSize;
	u64 unfinishedSpaceSize;

	u64 codeLen;

	u32 mainFuncSymbolId;
} __attribute__ ((packed)) File_ObjHeader;

typedef struct File_ExecHeader {
	
	u32 gloSymbolNum;
	u32 funSymbolNum;
	u32 relySymbolNum;

	u64 gloSpaceSize;
	u64 funcSpaceSize;
	u64 relySpaceSize;

	u64 codeLen;

	u32 mainFuncSymbolId;
} __attribute__ ((packed)) File_ExecHeader;

typedef struct File_RelyDesc {
	u32 isCustom;
	u32 len;
	union {
		char pkgHash[0];
		char cusPath[0];
	};
} __attribute__ ((packed)) File_RelyDesc;

typedef struct File_FuncDesc {
	u32 isOuter;
	u64 offset; // for unfinished symbol, this field should be -1
	union {
		u64 innerOffset;
		struct {
			u32 relyId;
			u32 funcId;
		} __attribute__ ((packed)) outer;
	};
	u32 fullNameLen;
	char fullName[0];
} __attribute__ ((packed)) File_FuncDesc;

typedef struct File_GloDesc {
	u32 isOuter;
	u64 offset; // for unfinished symbol, this field should be -1
	union {
		u64 innerOffset;
		struct {
			u32 relyId;
			u32 funcId;
		} __attribute__ ((packed)) outer;
	};
	u32 fullNameLen;
	char defVal[0];
} __attribute__ ((packed)) File_GloDesc;

typedef struct File_UnfinishedDesc {
	u32 type;
	#define File_UnfinishedDesc_Type_Code	1
	#define File_UnfinishedDesc_Type_Glob	2
	u32 fullNameLen;
	union {
		u64 codeOffset;
		u32 descOffset;
	} __attribute__ ((packed));
	char fullName[0];
} __attribute__ ((packed)) File_UnfinishedDesc;

#endif