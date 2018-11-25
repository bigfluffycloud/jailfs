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

testpkg:
#	xar --compression=none -c -f /pkg/test.xar test-pkg/*
	tar -cvf /pkg/tar.tar test-pkg/*
#	equery list \* --format="\$category/\$name" | \
#	  while read line; do \
#	     PKNAME=$(echo $$line|cut -f 2 -d '/'); \
#	     echo $$PKNAME; \
#	     tar -cvf /pkg/$$PKNAME.tar $(equery files $$line); \
#	  done
#	tar -cvf /pkg/irss.tar $(equery files irssi)
	equery -C files irssi | tar -cvf /pkg/irsi.tar -T-
dump-syms:
	nm -Clp ${bin} | \
	awk '{ printf "%s %s %s\n", $$3, $$2, $$4 }' |sort -u|less
