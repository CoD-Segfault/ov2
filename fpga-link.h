// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#ifndef __FPGA_LINK_H
#define __FPGA_LINK_H


void fpga_link_init(void);

void fpga_write32(unsigned reg, unsigned x);
unsigned fpga_read32(unsigned reg);
void fpga_reg_main(unsigned c);

#endif
