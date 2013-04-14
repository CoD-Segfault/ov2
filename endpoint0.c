// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include "vita.h"

#include "cxud.h"

#include <string.h>
#include "debug.h"


static u32 epout, epin;

static u8 setup[8];
static u8 configured;


static int XUD_SetBuffer_ResetPid_EpMax(u32 ep, const u8 buf[], u32 len, u32 reqlen, u32 packet_len, u8 pid)
{
	u32 off, this;

	this = packet_len;

	if (len > reqlen)
		len = reqlen;

	for (off = 0; off < len; off += this) {
		if (this > len - off)
			this = len - off;

		if (XUD_SetData(ep, buf, this, off, pid) < 0)
			return -1;

		pid = 0;
	}

	if (len < reqlen && this == packet_len)
		XUD_SetData(ep, buf, 0, 0, 0);

	return 0;
}

static int XUD_DoGetRequest(u32 epout, u32 epin, const u8 buf[], u32 len, u32 reqlen)
{
	u8 tmp[1024];

	int ret = XUD_SetBuffer_ResetPid_EpMax(epin, buf, len, reqlen, 64, 0x4b); // DATA1
	if (ret < 0)
		return -1;

	/* Status stage */
	return XUD_GetData(epout, tmp);
}

static int XUD_GetSetupBuffer(u32 ep, u8 buf[])
{
	return XUD_GetSetupData(ep, buf);
}

static int XUD_DoSetRequestStatus(u32 ep)
{
	unsigned char tmp[8];

	return XUD_SetData(ep, tmp, 0, 0, 0x4b); // DATA1
}

static void ep0_reply(const u8 reply[], u32 len)
{
	XUD_DoGetRequest(epout, epin, reply, len, setup[6] + 0x100*setup[7]);
}

static void ep0_ack(void)
{
	XUD_DoSetRequestStatus(epin);
}

static void ep0_stall(void)
{
	XUD_ResetEndpoint(epout, &epin);
}

static const u8 dev_desc_reply[] = {
	0x12, 0x01,		// len, device descriptor
	0x00, 0x02,		// usb version
	0xff, 0xff, 0xff,	// class, subclass, protocol
	0x40,			// max packet size for ep0
	0x69, 0x56,		// vendor id  XXX FIXME
	0x61, 0x74,		// device id  XXX FIXME
	0x01, 0x00,		// device version (v0.01)
	0x01,			// manufacturer string
	0x02,			// product string
	0x00,			// serial # string
	0x01			// # configurations
};

static const u8 qual_desc_reply[] = {
	0x0a, 0x06,		// len, device qualifier descriptor
	0x00, 0x02,		// usb version
	0xff, 0xff, 0xff,	// class, subclass, protocol
	0x40,			// max packet size for ep0
	0x01,			// # configurations
	0x00			// reserved
};

static const u8 conf_desc_reply[] = {
	0x09, 0x02,		// len, configuration descriptor
	0x30, 0x00,		// total len (9+9+7+9+7+7)
	0x02,			// # interfaces
	0x01,			// selector for this config
	0x00,			// configuration string (none)
	0x80,			// not self powered, no remote wakeup
	0xfa,			// power consumption, 1=2mA

	// data interface
	0x09, 0x04,		// len, interface descriptor
	0x00,			// interface #
	0x00,			// selector for alternate setting
	0x01,			// # endpoints
	0xff, 0xff, 0xff,	// class, subclass, protocol
	0x00,			// interface string (none)

	0x07, 0x05,		// len, endpoint descriptor
	0x81, 0x02,		// endpoint #, in, bulk
	0x00, 0x02,		// max packet size
//	0x00,			// interval (HS bulk: max NAK rate)
	0x01,			// interval (HS bulk: max NAK rate)

	// debug interface
	0x09, 0x04,		// len, interface descriptor
	0x01,			// interface #
	0x00,			// selector for alternate setting
	0x02,			// # endpoints
	0xff, 0xff, 0xff,	// class, subclass, protocol
	0x00,			// interface string (none)

	0x07, 0x05,		// len, endpoint descriptor
	0x02, 0x02,		// endpoint #, out, bulk
	0x00, 0x02,		// max packet size
//	0x00,			// interval (HS bulk: max NAK rate)
	0x01,			// interval (HS bulk: max NAK rate)

	0x07, 0x05,		// len, endpoint descriptor
	0x82, 0x02,		// endpoint #, in, bulk
	0x00, 0x02,		// max packet size
//	0x00			// interval (HS bulk: max NAK rate)
	0x01			// interval (HS bulk: max NAK rate)
};

static const u8 other_desc_reply[] = {
	0x09, 0x07,		// len, other speed configuration descriptor
	0x12, 0x00,		// total len (9+9)
	0x01,			// # interfaces
	0x01,			// selector for this config
	0x00,			// configuration string (none)
	0x80,			// not self powered, no remote wakeup
	0xfa,			// power consumption, 1=2mA

	0x09, 0x04,		// len, interface descriptor
	0x00,			// interface #
	0x00,			// selector for alternate setting
	0x00,			// # endpoints
	0xff, 0xff, 0xff,	// class, subclass, protocol
	0x00,			// interface string (none)
};

static const u8 lang_desc_reply[] = {
	0x04, 0x03,		// len, string descriptor
	0x09, 0x04		// EN US
};


static void setup_string_reply(const char reply[])
{
	u8 data[64];
	u32 i;

	u32 len = strlen(reply);
	if (len > 31)
		len = 31;

	data[0] = 2*len + 2;
	data[1] = 3;

	for (i = 0; i < len; i++) {
		data[2*i + 2] = reply[i];
		data[2*i + 3] = 0;
	}

	ep0_reply(data, 2*len + 2);
}

static void handle_string_descriptor(void)
{
	switch (setup[2]) {
	case 0:
		ep0_reply(lang_desc_reply, sizeof lang_desc_reply);
		break;
	case 1:
		setup_string_reply("OpenVizsla");
		break;
	case 2:
		setup_string_reply("Vituska");
		break;
	default:
		ep0_stall();
	}
}

static void handle_descriptor(void)
{
	switch (setup[3]) {
	case 1:
		printf("   --> device\n");
		ep0_reply(dev_desc_reply, sizeof dev_desc_reply);
		break;
	case 2:
		printf("   --> configuration\n");
		ep0_reply(conf_desc_reply, sizeof conf_desc_reply);
		break;
	case 3:
		printf("   --> string %d\n", setup[2]);
		handle_string_descriptor();
		break;
	case 6:
		printf("   --> device qualifier\n");
		ep0_reply(qual_desc_reply, sizeof qual_desc_reply);
		break;
	case 7:
		printf("   --> other speed configuration\n");
		ep0_reply(other_desc_reply, sizeof other_desc_reply);
		break;
	default:
		ep0_stall();
	}
}

static void handle_standard_device_setup(void)
{
	u8 reply[2] = {0};

	printf(" --> standard device\n");

	switch (setup[1]) {
	case 0:
		printf("   --> get status\n");
		if (conf_desc_reply[7] & 0x40)
			reply[0] = 1;
		ep0_reply(reply, 2);
		break;
	case 5:
		printf("  --> set address\n");
		ep0_ack();
		// XXX FIXME: Should not change address unless and until
		// we get an ACK!
		XUD_SetDevAddr(setup[2] + 0x100*setup[3]);
		break;
	case 6:
		printf("  --> get descriptor\n");
		handle_descriptor();
		break;
	case 8:
		printf("   --> get configuration\n");
		reply[0] = configured;
		ep0_reply(reply, 1);
		break;
	case 9:
		printf("   --> set configuration\n");
		configured = setup[2];
		ep0_ack();
		break;
	default:
		ep0_stall();
	}
}

static void handle_standard_interface_setup(void)
{
	u8 reply[2] = {0};

	printf(" --> standard interface\n");

	switch (setup[1]) {
	case 0:
		printf("   --> get status\n");
		ep0_reply(reply, 2);
		break;
	case 10:
		printf("   --> get interface\n");
		ep0_reply(reply, 1);
		break;
	default:
		ep0_stall();
	}
}

static void handle_standard_endpoint_setup(void)
{
	u8 reply[2] = {0};

	printf(" --> standard endpoint\n");

	switch (setup[1]) {
	case 0:
		printf("   --> get status\n");
		ep0_reply(reply, 2);
		break;
	default:
		ep0_stall();
	}
}

static void handle_setup(void)
{
	printf("Got a setup packet...  ");
	dump(setup, 8);

	switch (setup[0] & 0x7f) {
	case 0:
		handle_standard_device_setup();
		break;
	case 1:
		handle_standard_interface_setup();
		break;
	case 2:
		handle_standard_endpoint_setup();
		break;
	default:
		ep0_stall();
	}
}

void ep0_main(u32 c_out, u32 c_in)
{
	epout = XUD_Init_Ep(c_out);
	epin = XUD_Init_Ep(c_in);

	for (;;) {
		if (XUD_GetSetupBuffer(epout, setup) < 0)
			XUD_ResetEndpoint(epout, &epin);
		else
			handle_setup();
	}
}
