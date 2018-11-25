all: world

include mk/config.mk
include mk/distcc.mk
#include mk/ccache.mk
#include mk/ext.mk
include mk/indent.mk
include mk/bin.mk
include mk/clean.mk
 
ifeq (${CONFIG_DEBUG}, y)
CONFIG_STRIP_BINS=n
CFLAGS += -DCONFIG_DEBUG
endif

world:${bin} ${libs}

#	equery list \* --format="\$$name" |
testpkg:
	for line in "irssi tar"; do \
	   equery -C files $$line |  tar --no-recursion -T- -c -v -f /pkg/$$line.tar; \
	done

dump-syms:
	nm -Clp ${bin} | \
	awk '{ printf "%s %s %s\n", $$3, $$2, $$4 }' |sort -u|less
