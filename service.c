// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include <xs1.h>

#include "vita.h"
#include "service.h"

#include "debug.h"


u32 services[N_SERVICES];
static u32 master[N_MASTERS];
static volatile u32 ready;


void service_server(u32 n, u32 c)
{
	u32 j;

	// FIXME, assumes 2 masters only
	master[n] = c;
	master[1 - n] = getd(c);

	ready = 1;

	for (j = 0; j < N_SERVICES; j++) {
		u32 what = in32(c);
		services[what] = in32(c);
		inend(c);
	}
printf("service init done\n");
}

u32 service_add(u32 what)
{
	u32 c;
	u32 j;

	while (!ready)
		;

printf("adding service #%d\n", what);

	c = new_chanend();

	for (j = 0; j < N_MASTERS; j++) {
		setd(c, master[j]);
		out32(c, what);
		out32(c, c);
		outend(c);
	}

	return c;
}

void service_print(void)
{
	u32 j;

	printf("Masters:");
	for (j = 0; j < N_MASTERS; j++)
		printf(" %08x", master[j]);
	printf("\n");

	printf("Services:");
	for (j = 0; j < N_SERVICES; j++)
		printf(" %08x", services[j]);
	printf("\n");
}
