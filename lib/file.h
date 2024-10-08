#ifndef __LIB_FILE_H__
#define __LIB_FILE_H__

#include "bstype.h"

// the header for compiled obj file
PACK(typedef struct File_ObjHeader {
	u32 gloSymbolNum;
	u32 funcSymbolNum;
	u32 lblSymbolNum;
	u32 refSymbolNum;
	
	u64 gloSpaceSize;
	u64 funcSpaceSize;
	u64 lblSpaceSize;
	u64 refSpaceSize;

	u64 codeLen, gloLen;

	u32 mainFuncSymbolId;
}) File_ObjHeader;

// the header for executable file or library file
PACK(typedef struct File_ExecHeader {
	u64 hash;
	u32 gloSymbolNum;
	u32 funcSymbolNum;
	u32 relySymbolNum;

	u64 gloSpaceSize;
	u64 funcSpaceSize;
	u64 relySpaceSize;

	u64 codeLen, gloLen;

	u32 mainFuncSymbolId;
}) File_ExecHeader;

PACK(typedef struct File_RelyDesc {
	u64 attribute;
	u32 relyId;
	u32 fullNameLen;
	char fullName[0];
}) File_RelyDesc;

PACK(typedef struct File_FuncDesc {
	u64 attribute;
	u64 offset;
	u32 id;
	u32 fullNameLen;
	char fullName[0];
}) File_FuncDesc;

PACK(typedef struct File_GloDesc {
	u64 attribute;
	u64 offset;
	u32 id;
	u32 fullNameLen;
	char fullName[0];
}) File_GloDesc;

PACK(typedef struct File_LabelDesc {
	u64 offset;
	u32 fullNameLen;
	char fullName[0];
}) File_LabelDesc;

#define File_Desc_Attribute_Relevant	(1u << 0)

PACK(typedef struct File_RefDesc {
	u32 type;
	#define File_UnfinishedDesc_Type_Code	1
	#define File_UnfinishedDesc_Type_Glob	2
	u32 fullNameLen;
	PACK(union {
		u64 codeOffset;
		u32 descOffset;
	});
	char fullName[0];
}) File_RefDesc;

// this macros can be compiled successfully on any platform, but the C++ extension of vscode on Windows insists that there is syntax error
#define File_nextDesc(cur, type) ({ \
	type *tCur = (type *)(cur); \
	(type *)((u64)tCur + sizeof(type) + tCur->fullNameLen); \
})

#define File_descLen(desc) ((desc)->fullNameLen + sizeof(*desc))

#endif