all: world

include mk/config.mk
#include mk/distcc.mk
#include mk/ccache.mk
#include mk/ext.mk
include mk/indent.mk
include mk/bin.mk
#include mk/lib.mk
include mk/clean.mk
 
ifeq (${CONFIG_DEBUG}, y)
CONFIG_STRIP_BINS=n
CFLAGS += -DCONFIG_DEBUG
endif

world:${bin} ${libs}

testpkg:
	xar --compression=none -c -f /pkg/test.xar test-pkg/*
