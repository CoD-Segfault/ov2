// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#ifndef _DEBUG_H
#define _DEBUG_H

void debug_init(chanend debug_channel);
int printgrr(const char fmt[], ...);
#define printf printgrr
void dump(const unsigned char x[], unsigned int);

#endif
