// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#ifndef __SERVICE_H
#define __SERVICE_H

#include <xccompat.h>


#define SERVICE_UART 0
#define SERVICE_FPGA_REG 1
#define N_SERVICES 2
#define N_MASTERS 2


extern unsigned services[N_SERVICES];


void service_server(unsigned n, chanend);
unsigned service_add(unsigned what);

void service_print(void);

#endif
