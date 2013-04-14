// Copyright 2009-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include <stdarg.h>


typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;


static u32 debug_channel;


void debug_init(u32 x)
{
	debug_channel = x;
}


static void put1(char c)
{
	asm("outct res[%0],1 ; chkct res[%0],1" : : "r"(debug_channel));
	asm("out res[%0],%1" : : "r"(debug_channel), "r"(c));
	asm("outct res[%0],1 ; chkct res[%0],1" : : "r"(debug_channel));
}

static void put(char c)
{
	if (c == '\n')
		put1('\r');
	put1(c);
}


// __umoddi3() and friends are very big, and more general than we need:
// radix is always (very) small, so we can work by much bigger chunks
// than single bits, always.
static int extract_dig(u64 *x, u32 radix)
{
	u32 hi = *x >> 32;
	u32 lo = *x;
	u32 mod = hi % radix;
	hi /= radix;
	u32 n = (mod << 16) | (lo >> 16);
	mod = n % radix;
	n /= radix;
	lo = (mod << 16) | (lo & 0xffff);
	mod = lo % radix;
	lo /= radix;
	lo |= (n << 16);
	*x = ((u64)hi << 32) | lo;
	return mod;
}


// This implements conversions %{0}{number}{l,ll}[%cdsux] only.
// Field length is obeyed for numbers only.
// Always returns 0.

int printgrr(const char *restrict format, ...)
{
	va_list ap;

	va_start(ap, format);

	while (*format) {
		if (*format != '%') {
			put(*format++);
			continue;
		}
		format++;

		int zero = 0;
		int prec = 0;

		if (*format == '0') {
			zero = 1;
			format++;
		}

		while (*format >= '0' && *format <= '9')
			prec = 10*prec + (*format++ - '0');

		int ll = 0;
		while (*format == 'l') {
			ll++;
			format++;
		}

		int radix = 10;
		int is_signed = 1;

		switch (*format++) {
		case '%':
			put('%');
			break;

		case 'c':
			put(va_arg(ap, int));
			break;

		case 's':
			;
			char *s = va_arg(ap, char *);
			while (*s)
				put(*s++);
			break;

		case 'x':
			radix = 16;

		case 'u':
			is_signed = 0;

		case 'd':
			;
			u64 x;
			if (is_signed) {
				if (ll == 0)
					x = va_arg(ap, int);
				else if (ll == 1)
					x = va_arg(ap, long);
				else
					x = va_arg(ap, long long);
			} else {
				if (ll == 0)
					x = va_arg(ap, unsigned int);
				else if (ll == 1)
					x = va_arg(ap, unsigned long);
				else
					x = va_arg(ap, unsigned long long);
			}

			if (is_signed) {
				if ((long long)x < 0)
					x = -x;
				else
					is_signed = 0;
			}

			char hold[22];
			char *hld = &hold[sizeof hold];
			*--hld = 0;

			int len = 0;
			do {
				int dig = extract_dig(&x, radix);
				if (dig >= 10)
					dig += 'a' - 10;
				else
					dig += '0';
				*--hld = dig;
				len++;
			} while (x);
			if (is_signed)
				*--hld = '-';

			while (len < prec) {
				put(zero ? '0' : ' ');
				len++;
			}
			while (*hld)
				put(*hld++);
		}
	}

	va_end(ap);

	return 0;
}

#define printf printgrr

void dump(const u8 x[], u32 n)
{
	u32 j, k;

	for (j = 0; j < n; j += 16) {
		printf("%02x:", j);
		for (k = 0; k < 16 && j + k < n; k++)
			printf(" %02x", x[j + k]);
		printf("\n");
	}
}
