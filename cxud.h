// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#ifndef _CXUD_H
#define _CXUD_H

#include "vita.h"

int XUD_ResetEndpoint(u32 ep, u32 *ep2);
void XUD_SetDevAddr(u32 devaddr);
int XUD_GetSetupData(u32 ep, u8 buf[]);
int XUD_GetData(u32 ep, u8 buf[]);
int XUD_SetData(u32 ep, const u8 buf[], u32 len, u32 startIndex, u32 pidToggle);
u32 XUD_Init_Ep(u32 ce);

#endif
