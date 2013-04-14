# Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
# Licensed to you under the terms of the GPL version 3; see file COPYING.

XCC := xcc
XCFLAGS := -Wall -W -O2 vita.xn -Wno-switch-fallthrough
LDFLAGS :=


# Build with "V=1" to see the commands executed; be quiet otherwise.

ifeq ($(V),1)
	Q :=
else
	Q := @
endif


OBJS := main.o version.o service.o
OBJS += fpga-link.o debug.o uart.o spi.o
OBJS += endpoint0.o endpoint1.o endpoint2.o
OBJS += XUD_EpFunctions.o XUD_UIFM_Ports.o XUD_EpFuncs.o

LIBS := libxud.a


.PHONY: all
all: tt


# Laziness rules, and lazy rules rule most of all.
*.o *.s *.i: *.h Makefile vita.xn


tt: $(OBJS)
	@echo "  LINK      $@"
	$(Q)$(XCC) $(XCFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.xc
	@echo "  COMPILE   $@"
	$(Q)$(XCC) $(XCFLAGS) -o $@ -c $<

%.o: %.c
	@echo "  COMPILE   $@"
	$(Q)$(XCC) $(XCFLAGS) -o $@ -c $<

%.i: %.c
	@echo "  COMPILE   $@"
	$(Q)$(XCC) $(XCFLAGS) -o $@ -E $<

%.s: %.c
	@echo "  COMPILE   $@"
	$(Q)$(XCC) $(XCFLAGS) -o $@ -S $<

%.o: %.S
	@echo "  ASSEMBLE  $@"
	$(Q)$(XCC) $(XCFLAGS) -o $@ -c $<


.version: FORCE
	$(Q)./describe.sh > .$@-tmp
	$(Q)cmp -s $@ .$@-tmp || cp .$@-tmp $@
	$(Q)rm .$@-tmp

version.c: .version
	@echo "  VERSION   $@"
	$(Q)echo "const char version[] = \"`cat $^` (`whoami`@`hostname -s`)\";" > $@

FORCE:

.PHONY: clean
clean:
	-rm -f tt $(OBJS) .version version.c
