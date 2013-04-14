// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include <xs1.h>
#include <platform.h>

#include "xud.h"
#include "debug.h"


static unsigned char pak[512];
static unsigned int log[128];
static unsigned int logn;

void ep1_main(chanend c_in)
{
	timer t;
	unsigned time, prev_time;
	int err;
	int j;

	XUD_ep ep1 = XUD_Init_Ep(c_in);

	for (j = 0; j < 512; j++)
		pak[j] = 0;

	for (;;) {
		t :> time;
		log[logn] = time - prev_time;
		prev_time = time;
		logn++;
		if (logn == 128) {
			logn = 0;
			err = XUD_SetBuffer(ep1, (log, unsigned char[]), 512);
		} else
			err = XUD_SetBuffer(ep1, pak, 512);

		if (err < 0) {
printf("ugh, reset...\n");
			XUD_ResetEndpoint(ep1, null);
		}
//printf("buffer sent!\n");
	}
}
