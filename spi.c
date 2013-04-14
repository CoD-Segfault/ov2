// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include "vita.h"

#include "spi.h"


// We use two clock blocks: the first counts half bits (SCK clock
// transitions), the other is the SCK clock, which we clock manually.
// Implemented is SPI "mode 3", i.e. SCK idles at high, and MOSI and
// MISO are set at the falling edge of SCK, read at the rising edge.
// We ensure that CS is active a half SCK time before and after the
// data bits.


// The current chip select, set by spi_init().
static u32 spi_cs;


void spi_fini(void)
{
	// disable ports
	setc(spi_sck, 0);
	setc(spi_cs, 0);
	setc(spi_mosi, 0);
	setc(spi_miso, 0);

	// disable clkblks
	setc(spi_clk, 0);
	setc(spi_half_clk, 0);
}

void spi_init(u32 cs)
{
	spi_cs = cs;

	// enable clkblks
	setc(spi_clk, 8);
	setc(spi_half_clk, 8);

	setclk(spi_half_clk, 1);
	setd(spi_half_clk, 1);	// REF/2 = 50MHz
	//setd(spi_half_clk, 50);	// REF/100 = 1MHz
	//setd(spi_half_clk, 500);	// REF/1000 = 100kHz  XXX: only 8 bits
	//setd(spi_half_clk, 0);	// REF/1 = 100MHz

	// enable ports, set clkblk
	setc(spi_sck, 8);
	setc(spi_cs, 8);
	setclk(spi_sck, spi_half_clk);
	setclk(spi_cs, spi_half_clk);

	// start clkblk
	setc(spi_half_clk, 0xf);

	// deselect chip, set sck high
	out32(spi_cs, 0xffffffff);
	out32(spi_sck, 0xffffffff);
	syncr(spi_sck);

	// sck is buffered, transfer width 32
	setc(spi_sck, 0x200f);
	settw(spi_sck, 0x20);

	setc(spi_mosi, 8);
	setc(spi_miso, 8);
//setc(spi_mosi, 0x2007);
out32(spi_mosi, 0);
//syncr(spi_mosi);
	setclk(spi_clk, spi_sck);
	setclk(spi_mosi, spi_clk);
	setclk(spi_miso, spi_clk);

	// data is buffered, transfer width 32
	setc(spi_mosi, 0x200f);
	setc(spi_miso, 0x200f);
	settw(spi_mosi, 0x20);
	settw(spi_miso, 0x20);

	// start clkblk
	setc(spi_clk, 0xf);

setc(spi_mosi, 0x17);
setc(spi_miso, 0x17);

//	// set times
//	u32 t = getpt(spi_mosi);
//	t += 31;
//	setpt(spi_miso, t);
}

static void spi_start(void)
{
udelay(1);

	// select chip
	out32(spi_cs, 0);
	syncr(spi_cs);
}

static void spi_stop(void)
{
	// deselect chip
	outpw(spi_sck, 1, 1);
	syncr(spi_sck);
	out32(spi_cs, 1);
	syncr(spi_cs);
}

static u32 spi_do32(u32 x)
{
	out32(spi_mosi, bitrev(x));
	out32(spi_sck, 0xaaaaaaaa);
	out32(spi_sck, 0xaaaaaaaa);
	return bitrev(in32(spi_miso));
}

static u8 spi_do8(u8 x)
{
	outpw(spi_mosi, bitrev(x) >> 24, 8);
	outpw(spi_sck, 0xaaaa, 16);
	return bitrev(inpw(spi_miso, 8));
}


static u32 spi_rom_get_status(void)
{
	u32 x;

	spi_start();
	spi_do8(0x05);
	x = spi_do8(0);
	spi_stop();

	return x;
}

static void spi_rom_write_enable(void)
{
	spi_start();
	spi_do8(0x06);
	spi_stop();
}

#if 0
static void spi_rom_write_disable(void)
{
	spi_start();
	spi_do8(0x04);
	spi_stop();
}
#endif

void spi_rom_read(u8 data[], u32 addr, u32 n)
{
	u32 j;

	spi_start();
	spi_do32(0x0b000000 | addr);
	spi_do8(0);

	// XXX: should use 32-bit accesses
	for (j = 0; j < n; j++)
		data[j] = spi_do8(0);

	spi_stop();
}

#if 0
void spi_rom_erase_all(void)
{
	spi_rom_write_enable();
	spi_start();
	spi_do8(0xc7);
	spi_stop();

	while (spi_rom_get_status() & 1)
		;
}
#endif

void spi_rom_erase_sector(u32 addr)
{
	spi_rom_write_enable();
	spi_start();
	spi_do32(0xd8000000 | addr);
	spi_stop();

	while (spi_rom_get_status() & 1)
		;
}

void spi_rom_program_page(u32 addr, const u8 data[])
{
	u32 j;

	spi_rom_write_enable();
	spi_start();
	spi_do32(0x02000000 | addr);

	// XXX: should use 32-bit accesses
	for (j = 0; j < 256; j++)
		spi_do8(data[j]);

	spi_stop();

	while (spi_rom_get_status() & 1)
		;
}
