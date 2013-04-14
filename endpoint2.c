// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include "vita.h"

#include "cxud.h"

#include "debug.h"
#include "fpga-link.h"
#include "spi.h"


static unsigned pakout[128];
static unsigned pakin[128];


static int handle(int len)
{
	if (len < 4)
		return -1;

	switch (pakout[0]) {
	case 0x46507264:	// FPrd addr --> data
		if (len != 8)
			return -1;
		pakin[0] = fpga_read32(pakout[1]);
		return 4;

	case 0x46507772:	// FPwr addr data
		if (len != 12)
			return -1;
		fpga_write32(pakout[1], pakout[2]);
		return 0;

	case 0x464c7264:	// FLrd addr --> data
		if (len != 8)
			return -1;
//printf("read rom @ %06x...", pakout[1]);
extern u32 spi_rom_cs;
		spi_init(spi_rom_cs);
		spi_rom_read(pakin, pakout[1], 0x100);
		spi_fini();
//printf(" done.\n");
		return 0x100;
	}

	return -1;
}

void ep2_main(chanend c_out, chanend c_in)
{
	int len;

	u32 epout = XUD_Init_Ep(c_out);
	u32 epin = XUD_Init_Ep(c_in);

	for (;;) {
		len = XUD_GetData(epout, pakout);
		len = handle(len);
		if (len > 0)
			len = XUD_SetData(epin, pakin, len, 0, 0);

		if (len < 0) {
printf("ugh, ep2 reset...\n");
			XUD_ResetEndpoint(epout, &epin);
		}
	}
}
