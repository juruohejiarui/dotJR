#include "api.h"

const char *HInst_cmdStr[] =  {
	"getLoc",	"getGlo",	"getMem",	"getArr",
	"setLoc",	"setGlo",	"setMem",	"setArr",
	"locAddr",	"gloAddr",	"memAddr",	"arrAddr",
	"get",
	"push",		"pop",
	"add",		"sub",		"mul",		"div",		"mod",
	"and",		"or",		"xor",
	"not",		"lnot",
	"inc",		"dec",
	"new",		"arrnew",
	"cmp",
	"jmp",
	"jz",		"jul",		"jus",		"jsl",	"jss",
	"setFlag",	"pushFlag",	
	"switch",	
	"call",		"callv",		"ret",		"retVal",
	"setLocNum",
	"setarg",	"getarg",
	"syscall",
};

u32 HInst_getSize(HInstHdr *hdr) {
	u32 sz = sizeof(HInstHdr);
	if (hdr->type == HInstHdr_Type_generic) sz += sizeof(u8);
	if (hdr->varArgFlag.isVarNum)
		sz += hdr->varArgFlag.argNum * (hdr->varArgFlag.isLong ? sizeof(u64) : sizeof(u32));
	else sz += (8 << (hdr->bsArgFlag.argType & 0x3));
	return sz;
}

void HInst_setExtType(HInstHdr *hdr, u8 type) {
	hdr->type = HInstHdr_Type_generic;
	*(u8 *)(hdr + 1) = type;
}

u8 HInst_getExtType(HInstHdr *hdr) {
	if (hdr->type != HInstHdr_Type_generic) return (u8)-1;
	else *(u8 *)(hdr + 1);
}

void HInst_setArg(HInstHdr *hdr, int idx, u64 arg) {
	void *ptr = (hdr + 1);
	if (hdr->type == HInstHdr_Type_generic) ptr = ((u8 *)ptr) + 1;
	if (hdr->varArgFlag.isVarNum) {
		if (hdr->varArgFlag.isLong)
			*(((u32 *)ptr) + idx) = *(u32 *)arg;
		else *(((u64 *)ptr) + idx) = arg;
	} else {
		switch (hdr->type) {
			case HInstHdr_Type_u8 :
			case HInstHdr_Type_i8 :
				*(u8 *)ptr = *(u8 *)arg;
				break;
			case HInstHdr_Type_u16:
			case HInstHdr_Type_i16:
				*(u16 *)ptr = *(u16 *)arg;
				break;
			case HInstHdr_Type_i32:
			case HInstHdr_Type_u32:
			case HInstHdr_Type_f32:
				*(u32 *)ptr = *(u32 *)arg;
				break;
			case HInstHdr_Type_i64:
			case HInstHdr_Type_u64:
			case HInstHdr_Type_f64:
			case HInstHdr_Type_obj:
				*(u64 *)ptr = *(u64 *)arg;
				break;
		}
	}
}

u64 HInst_getArg(HInstHdr *hdr, int idx) {
	void *ptr = (hdr + 1);
	if (hdr->type == HInstHdr_Type_generic) ptr = ((u8 *)ptr) + 1;
	if (hdr->varArgFlag.isVarNum) {
		if (hdr->varArgFlag.isLong) return *(((u32 *)ptr) + idx);
		else return *(((u64 *)ptr) + idx);
	} else {
		switch (hdr->type) {
			case HInstHdr_Type_u8 :
			case HInstHdr_Type_i8 :
				return *(u8 *)ptr;
			case HInstHdr_Type_u16:
			case HInstHdr_Type_i16:
				return *(u16 *)ptr;
			case HInstHdr_Type_i32:
			case HInstHdr_Type_u32:
			case HInstHdr_Type_f32:
				return *(u32 *)ptr;
			case HInstHdr_Type_i64:
			case HInstHdr_Type_u64:
			case HInstHdr_Type_f64:
			case HInstHdr_Type_obj:
				return *(u64 *)ptr;
		}
	}
}

HInstHdr *HInst_read(const char *str, char **to) {
	HInstHdr hdr;
	memset(hdr, 0, sizeof(HInstHdr));
	
}
