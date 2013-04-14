// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#ifndef _UART_H
#define _UART_H

struct uart {
	port rx_port;
	port tx_port;
	unsigned bittime;
};

void tx_char(const struct uart &uart, unsigned c);
unsigned rx_char(const struct uart &uart);

#endif
