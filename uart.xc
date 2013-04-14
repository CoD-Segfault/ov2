// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include <xs1.h>

#include "vita.h"
#include "uart.h"


static void tx_bit(const struct uart &uart, unsigned bit)
{
	unsigned time;
	timer t;

	t :> time;
	uart.tx_port <: bit;
	t when timerafter(time + uart.bittime) :> void;
}

static unsigned rx_bit(const struct uart &uart, unsigned half)
{
	timer t;
	unsigned time;
	unsigned bit;

	t :> time;
	t when timerafter(time + uart.bittime / (1 + half)) :> void;

	uart.rx_port :> bit;
	return bit;
}

void tx_char(const struct uart &uart, unsigned c)
{
	unsigned j;

	/* STFU */
	for (j = 0; j < 10; j++)
		tx_bit(uart, 1);

	tx_bit(uart, 0);

	/* output data bits */
	for (j = 0; j < 8; j++) {
		tx_bit(uart, c);
		c >>= 1;
	}

	tx_bit(uart, 1);
}

unsigned rx_char(const struct uart &uart)
{
	unsigned c, bit;
	unsigned j;

	uart.rx_port when pinseq(1) :> void;
	uart.rx_port when pinseq(0) :> void;

	bit = rx_bit(uart, 1);
	if (bit)
		return -1;

	c = 0;
	for (j = 0; j < 8; j++)
		c |= rx_bit(uart, 0) << j;

	bit = rx_bit(uart, 0);
	if (bit == 0)
		return -1;

	return c;
}
