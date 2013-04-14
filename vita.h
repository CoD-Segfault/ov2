// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#ifndef __VITA_H
#define __VITA_H

typedef unsigned char u8;
typedef unsigned int u32;

void udelay(unsigned);

extern const char version[];


#ifndef __XC__

typedef u32 clock;
typedef u32 port;
typedef u32 chanend;
#define in
#define out


static inline u32 bitrev(u32 x)
{
	asm("bitrev %0,%1" : "=r"(x) : "r"(x));

	return x;
}

static inline u32 new_chanend(void)
{
	u32 c;

	asm volatile("getr %0,2" : "=r"(c));

	return c;
}

static inline void free_chanend(u32 c)
{
	asm("freer res[%0]" : : "r"(c));
}

static inline void setc(u32 c, u32 d)
{
	asm("setc res[%0],%1" : : "r"(c), "ri"(d));
}

static inline void settw(u32 c, u32 d)
{
	asm("settw res[%0],%1" : : "r"(c), "r"(d));
}

static inline void setclk(u32 c, u32 d)
{
	asm("setclk res[%0],%1" : : "r"(c), "r"(d));
}

static inline void setd(u32 c, u32 d)
{
	asm("setd res[%0],%1" : : "r"(c), "r"(d));
}

static inline u32 getd(u32 c)
{
	u32 d;

	asm volatile("getd %0,res[%1]" : "=r"(d) : "r"(c));

	return d;
}

static inline void setpt(u32 c, u32 d)
{
	asm("setpt res[%0],%1" : : "r"(c), "r"(d));
}

static inline u32 getpt(u32 c)
{
	u32 d;

	asm volatile("getts %0,res[%1]" : "=r"(d) : "r"(c));

	return d;
}

static inline void syncr(u32 c)
{
	asm("syncr res[%0]" : : "r"(c));
}

static inline void out32(u32 c, u32 d)
{
	asm("out res[%0],%1" : : "r"(c), "r"(d));
}

static inline void out8(u32 c, u8 d)
{
	asm("outt res[%0],%1" : : "r"(c), "r"(d));
}

static inline void outc8(u32 c, u8 d)
{
	asm("outct res[%0],%1" : : "r"(c), "ri"(d));
}

static inline void outend(u32 c)
{
	outc8(c, 1);
}

static inline void outpw(u32 c, u32 d, u32 n)
{
	asm("outpw res[%0],%1,%2" : : "r"(c), "r"(d), "i"(n));
}

static inline u32 in32(u32 c)
{
	u32 d;

	asm volatile("in %0,res[%1]" : "=r"(d) : "r"(c));

	return d;
}

static inline u8 in8(u32 c)
{
	u32 d;

	asm volatile("int %0,res[%1]" : "=r"(d) : "r"(c));

	return d;
}

static inline u8 inc8(u32 c)
{
	u32 d;

	asm volatile("inct %0,res[%1]" : "=r"(d) : "r"(c));

	return d;
}

static inline void chkc8(u32 c, u8 d)
{
	asm("chkct res[%0],%1" : : "r"(c), "ri"(d));
}

static inline void inend(u32 c)
{
	chkc8(c, 1);
}

static inline u32 inpw(u32 c, u32 n)
{
	u32 d;

	asm volatile("inpw %0,res[%1],%2" : "=r"(d) : "r"(c), "i"(n));

	return d;
}

#endif

#endif
