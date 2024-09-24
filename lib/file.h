#ifndef __LIB_FILE_H__
#define __LIB_FILE_H__

#include "bstype.h"

// the header for compiled obj file
typedef struct File_ObjHeader {
	u32 gloSymbolNum;
	u32 funcSymbolNum;
	u32 refSymbolNum;
	
	u64 gloSpaceSize;
	u64 funcSpaceSize;
	u64 refSpaceSize;

	u64 codeLen, gloLen;

	u32 mainFuncSymbolId;
} __attribute__ ((packed)) File_ObjHeader;

// the header for executable file or library file
typedef struct File_ExecHeader {
	u32 gloSymbolNum;
	u32 funSymbolNum;
	u32 relySymbolNum;

	u64 gloSpaceSize;
	u64 funcSpaceSize;
	u64 relySpaceSize;

	u64 codeLen, gloLen;

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
	u64 attribute;
	union {
		struct {
			u32 offset;
			u32 id;
		} __attribute__ ((packed)) inner;
		struct {
			u32 relyId;
			u32 funcId;
		} __attribute__ ((packed)) outer;
	};
	u32 fullNameLen;
	char fullName[0];
} __attribute__ ((packed)) File_FuncDesc;

typedef struct File_GloDesc {
	u64 attribute;
	union {
		struct {
			u32 offset;
			u32 id;
		} __attribute__ ((packed)) inner;
		struct {
			u32 relyId;
			u32 gloOffset;
		} __attribute__ ((packed)) outer;
	};
	u32 fullNameLen;
	char fullName[0];
} __attribute__ ((packed)) File_GloDesc;

#define File_Desc_Attribute_Inner (1u << 0)

typedef struct File_RefDesc {
	u32 type;
	#define File_UnfinishedDesc_Type_Code	1
	#define File_UnfinishedDesc_Type_Glob	2
	u32 fullNameLen;
	union {
		u64 codeOffset;
		u32 descOffset;
	} __attribute__ ((packed));
	char fullName[0];
} __attribute__ ((packed)) File_RefDesc;

#define File_nextDesc(cur, type) ({ \
	type *tCur = (type *)(cur); \
	(type *)((u64)tCur + sizeof(type) + tCur->fullNameLen); \
})

#define File_descLen(desc) ((desc)->fullNameLen + sizeof(*desc))

#endif