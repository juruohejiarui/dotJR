#ifndef __HINST_DESC_H__
#define __HINST_DESC_H__
#include "../lib/bstype.h"

enum HInstCmd {
	HInstCmd_getLoc,	HInstCmd_getGlo,	HInstCmd_getMem,	HInstCmd_getArr,
	HInstCmd_setLoc,	HInstCmd_setGlo,	HInstCmd_setMem,	HInstCmd_setArr,
	HInstCmd_locAddr,	HInstCmd_gloAddr,	HInstCmd_memAddr,	HInstCmd_arrAddr,
	HInstCmd_loadfptr,	
	HInstCmd_get,		HInstCmd_set,
	HInstCmd_push,		HInstCmd_pop,
	HInstCmd_add,		HInstCmd_sub,		HInstCmd_mul,		HInstCmd_div,		HInstCmd_mod,
	HInstCmd_and,		HInstCmd_or,		HInstCmd_xor,		HInstCmd_shl,		HInstCmd_shr,
	HInstCmd_addMov, 	HInstCmd_subMov,	HinstCmd_mulMov,	HInstCmd_divMov,	HInstCmd_modMov,
	HInstCmd_AndMov,	HInstCmd_orMov,		HInstCmd_xorMox,	HInstCmd_shlMov,	HinstCmd_shrMov,
	HInstCmd_not,		HInstCmd_lnot,
	HInstCmd_inc,		HInstCmd_dec,		
	HInstCmd_ptrAdd,	HInstCmd_ptrSub,	HInstCmd_ptrInc,	HInstCmd_ptrDec,
	HInstCmd_new,		HInstCmd_arrnew,
	HInstCmd_cmp,		HInstCmd_ptrCmp,
	HInstCmd_jmp,
	HInstCmd_jz,		HInstCmd_jnz,		HInstCmd_jul,		HInstCmd_jus,		HInstCmd_jsl,	HInstCmd_jss,
	HInstCmd_setFlag,	HInstCmd_pushFlag,	
	HInstCmd_switch,	
	HInstCmd_call,		HInstCmd_callv,		HInstCmd_ret,		HInstCmd_retVal,
	HInstCmd_setLocNum,
	HInstCmd_setArg,	HInstCmd_getArg,	HInstCmd_setGener,	
	HInstCmd_sysCall,	
};

#define HInst_cmdNum 68

extern const char *HInst_cmdStr[];

// header of a HInst
// format :
// [byte type]
// [word argFlag]
// 	[bit 7] isVarNum
//		if isVarNum == 1:
//			[bit 6] isLong
//			[bit 0:5] argNum
//		else:
//			[bit 0:7] argType # this field can not be generic
//		endif
// [byte] cmd
// if type == generic :
// 	[byte] extType
// if isVarNum == 1:
// 	if isLong == 1:
//			[Qword * argNum] args
//		else :
//			[Dword * argNum] args
// else :
//		[argType] arg
typedef struct HInstHdr {
	u8 type;
	union {
		struct {
			u8 argNum : 6;
			u8 isLong : 1;
			u8 isVarNum : 1;
		} __attribute__ ((packed)) varArgFlag;
		struct {
			u8 argType : 7;
			u8 isVarNum : 1;
		} __attribute__ ((packed)) bsArgFlag;
		u16 argFlag;
	};
	u8 cmd;
} __attribute__ ((packed)) HInstHdr;



#endif