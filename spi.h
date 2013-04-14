// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#ifndef __SPI_H
#define __SPI_H

#include "vita.h"


extern clock spi_half_clk, spi_clk;
extern out port spi_sck, spi_mosi;
extern in port spi_miso;


void spi_init(out port cs);
void spi_fini(void);


void spi_rom_erase_all(void);
void spi_rom_erase_sector(u32 addr);
void spi_rom_program_page(u32 addr, const u8 data[]);
void spi_rom_read(u8 data[], u32 addr, u32 n);

#endif
