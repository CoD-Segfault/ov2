// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include <xs1.h>
#include <platform.h>
#include <print.h>

#include "vita.h"

#include "xud.h"

#include "service.h"
#include "fpga-link.h"
#include "uart.h"
#include "spi.h"
#include "debug.h"

#define N_OUT 3
#define N_IN 3

#define USB_RST_PORT    PORT_ULPI_RST


/* Endpoint type tables */
XUD_EpType epTypeTableOut[N_OUT] = {
	XUD_EPTYPE_CTL, XUD_EPTYPE_DIS, XUD_EPTYPE_BUL
};
XUD_EpType epTypeTableIn[N_IN] = {
	XUD_EPTYPE_CTL, XUD_EPTYPE_BUL, XUD_EPTYPE_BUL
};

/* USB Port declarations */
on stdcore[0]: out port p_usb_rst = USB_RST_PORT;
on stdcore[0]: clock    usbclk    = XS1_CLKBLK_3;

void ep0_main(chanend c_out, chanend c_in);
void ep1_main(chanend c_in);
void ep2_main(chanend c_out, chanend c_in);




void udelay(unsigned n)
{
	timer t;
	unsigned time;

	t :> time;
	t when timerafter(time + n * (XS1_TIMER_HZ / 1000000)) :> void;
}


on stdcore[0]: static struct uart debug_uart_0 = {
        PORT_UART_RX0,          // RX
        PORT_UART_TX0,          // TX
        XS1_TIMER_HZ / 115200   // bittime
};
on stdcore[1]: static struct uart debug_uart_1 = {
        PORT_UART_RX1,          // RX
        PORT_UART_TX1,          // TX
        XS1_TIMER_HZ / 115200   // bittime
};

static void uart_tx_worker(const struct uart &uart, chanend tx_channel)
{
        uart.tx_port <: 1;

        for (;;) {
                unsigned c;

                tx_channel :> c;
                tx_char(uart, c);
        }
}

void sink(chanend c)
{
	unsigned x;

//	set_thread_fast_mode_on();

	for (;;)
		asm("in %0,res[%1]" : "=r"(x) : "r"(c));
}

void sink2(chanend c[2])
{
	unsigned x;

//	set_thread_fast_mode_on();

	for (;;) {
		asm("in %0,res[%1]" : "=r"(x) : "r"(c[0]));
		asm("in %0,res[%1]" : "=r"(x) : "r"(c[1]));
	}
}

on stdcore[0]: clock spi_half_clk = XS1_CLKBLK_1;
on stdcore[0]: clock spi_clk = XS1_CLKBLK_2;
on stdcore[0]: out port spi_rom_cs = PORT_SPI_ROM_CS;
on stdcore[0]: out port spi_sck = PORT_SPI_SCK;
on stdcore[0]: out port spi_mosi = PORT_SPI_MOSI;
on stdcore[0]: in port spi_miso = PORT_SPI_MISO;

on stdcore[0]: out port target_reset = PORT_TARGET_PHY_RESET;


static u8 buffie[256];

static void fdump(u32 addr)
{
	printf("Reading from %06x...", addr);
	spi_rom_read(buffie, addr, 256);
	printf(" done.\n");
	dump(buffie, 256);
	printf("\n");
}

static void play_with_flash(void)
{
	u32 j;

	spi_init(spi_rom_cs);
	fdump(0);
	fdump(0x010000);

	printf("Erasing 010000...");
	spi_rom_erase_sector(0x010000);
	printf(" done.\n");
	fdump(0x010000);

	printf("Writing 010000...");
	for (j = 0; j < 256; j++)
		buffie[j] = j;
	spi_rom_program_page(0x010000, buffie);
	printf(" done.\n");
	fdump(0x010000);

	spi_fini();
}

/*static*/ void main0(chanend service_channel, chanend bench_channel[2])
{
	chan c_ep_out[N_OUT], c_ep_in[N_IN];

	unsigned uart_service;

	printf("OHAI from Vita (core 0)!\n");
	printf("Build: %s\n", version);

	par {
		service_server(0, service_channel);
		uart_service = service_add(SERVICE_UART);
	}
	service_print();

	printf("Resetting target PHY...  ");
	target_reset <: 0;
	udelay(1000);
	target_reset <: 1;
	printf("Done.\n");

//play_with_flash();

	par {
		XUD_Manager(c_ep_out, N_OUT, c_ep_in, N_IN, null, epTypeTableOut, epTypeTableIn, p_usb_rst, usbclk, -1, XUD_SPEED_HS, null);

		{
			set_thread_fast_mode_on();
			ep0_main(c_ep_out[0], c_ep_in[0]);
		}

		{
			set_thread_fast_mode_on();
			ep1_main(c_ep_in[1]);
		}

		{
			set_thread_fast_mode_on();
			ep2_main(c_ep_out[2], c_ep_in[2]);
		}

//sink(bench_channel[0]);
//sink(bench_channel[1]);
sink2(bench_channel);
	}
}

static void cylon(void)
{
	unsigned x;
	const char eyes[8] = { 0, 1, 2, 4, 4, 2, 1, 0 };

	for (x = 0; ; x = (x + 1) & 7) {
		fpga_write32(0x01000000, eyes[x]);
		udelay(100000);
	}
}

static u8 ulpi_read(u8 reg)
{
	u32 x, y, z, w, v, v2;

x = fpga_read32(0x03000000);
y = fpga_read32(0x04000000);
z = fpga_read32(0x04000001);
w = fpga_read32(0x04000003);
v = fpga_read32(0x04000004);
printf("---> BEFORE: %08x, %08x, %08x, %08x, %08x...\n", x, y, z, w, v);
	fpga_write32(0x03000000, 0x40000000 + 0x10000*reg);
printf("---> WRITING: %08x...\n", 0x40000000 + 0x10000*reg);
	do {
		x = fpga_read32(0x03000000);
y = fpga_read32(0x04000000);
z = fpga_read32(0x04000001);
w = fpga_read32(0x04000003);
v = fpga_read32(0x04000004);
udelay(1000000);
v2 = fpga_read32(0x04000004);
printf("---> %08x, %08x, %08x, %08x, %08x (%08x)...\n", x, y, z, w, v, v2-v);
	} while (x & 0x20000000);

	return x;
}

static void testulpi(void)
{
	u32 j;

printf("ohai, resetting phy...\n");
udelay(100000);
printf("write 1...\n");
fpga_write32(0x04000002, 1);
udelay(100000);
printf("write 0...\n");
fpga_write32(0x04000002, 0);
udelay(100000);
printf("write 2, write 0, for good luck...\n");
fpga_write32(0x04000002, 2);
udelay(100000);
fpga_write32(0x04000002, 0);
udelay(100000);
printf("keeping fingers crossed...\n");
	for (;;) {
		printf(".......\n");
		for (j = 0; j < 16; j++)
			printf("%02x: %02x\n", j, ulpi_read(j));
		udelay(200000);
	}
}

void consumer_stoopid(chanend c)
{
	unsigned x;

//	set_thread_fast_mode_on();

	for (;;)
		c :> x;
}

void producer_stoopid(chanend c)
{
	unsigned j;

//	set_thread_fast_mode_on();

	for (j = 0; j < 50000000; j++)
		c <: 0x12345678;
}

void consumer_simple(chanend c)
{
	unsigned x;

//	set_thread_fast_mode_on();

	for (;;)
		asm("in %0,res[%1]" : "=r"(x) : "r"(c));
}

void producer_simple(chanend c)
{
	unsigned j;

//	set_thread_fast_mode_on();

	for (j = 0; j < 50000000; j++)
		asm("out res[%0],%1" : : "r"(c), "r"(0x12345678));
}

void producer_two(chanend c[2])
{
	unsigned j;

//	set_thread_fast_mode_on();

	for (j = 0; j < 50000000; j++) {
		asm("out res[%0],%1" : : "r"(c[0]), "r"(0x12345678));
		asm("out res[%0],%1" : : "r"(c[1]), "r"(0x12345678));
	}
}

void consumer(chanend c)
{
	//consumer_stoopid(c);
	consumer_simple(c);
}

void producer(chanend c[2])
{
	//producer_stoopid(c);
	//producer_simple(c);
	producer_two(c);
}

static void benchit(chanend b[2])
{
	timer t;
	unsigned t1, t2;
	chan c;

printf("WOT\n");
//	par {
//		consumer(c);
		for (;;) {
			unsigned x, xx;
			t :> t1;
			producer(b);
			t :> t2;
			x = t2 - t1;
			x /= 50000;
			xx = x % 1000;
			x /= 1000;
			printf("ohai, took %u.%03u ticks/iteration\n", x, xx);
		}
//	}
}


static void main1(chanend service_channel, chanend bench_channel[2])
{
	unsigned fpga_reg_service;

	printf("OHAI from Vita (core 1)!\n");
	printf("Build: %s\n", version);

	//fpga_init();

	par {
		service_server(1, service_channel);
		fpga_reg_service = service_add(SERVICE_FPGA_REG);
	}
	service_print();

	fpga_link_init();

	par {
		fpga_reg_main(fpga_reg_service);
		//cylon();
		benchit(bench_channel);
		///testulpi();
	}
}

on stdcore[0]: out port led0 = PORT_XLED0;
on stdcore[1]: out port led1 = PORT_XLED1;

static void blinkie(out port p, unsigned x)
{
	for (;;) {
		p <: x;
		udelay(x ? 350000 : 150000);
		x ^= 1;
	}
}

static void debugging_main0(chanend service_channel, chanend bench_channel[2])
{
	chan debug_channel_out_0;

	debug_init(debug_channel_out_0);
	udelay(1000000);

        par {
		uart_tx_worker(debug_uart_0, debug_channel_out_0);
		main0(service_channel, bench_channel);
		blinkie(led0, 0);
	}
}

static void debugging_main1(chanend service_channel, chanend bench_channel[2])
{
	chan debug_channel_out_1;

	debug_init(debug_channel_out_1);
	udelay(1000000);

        par {
		uart_tx_worker(debug_uart_1, debug_channel_out_1);
		main1(service_channel, bench_channel);
		blinkie(led1, 1);
	}
}


int main(void)
{
	chan service_channel;
	chan bench_channel[2];

	par {
		on stdcore[0]: debugging_main0(service_channel, bench_channel);
		on stdcore[1]: debugging_main1(service_channel, bench_channel);
	}

	return 0;
}
