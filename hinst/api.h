#ifndef __HINST_API_H__
#define __HINST_API_H__

#include "desc.h"

u32 HInst_getSize(HInstHdr *hdr);

// set the hdr->type == generic and the extend type as TYPE
void HInst_setExtType(HInstHdr *hdr, u8 type);
u8 HInst_getExtType(HInstHdr *hdr);
void HInst_setArg(HInstHdr *hdr, int idx, u64 arg);
u64 HInst_getArg(HInstHdr *hdr, int idx);

HInstHdr *HInst_read(const char *str, char **to);

#endif