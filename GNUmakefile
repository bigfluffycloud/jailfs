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

world: ${bin} ${libs}

testpkg:
	./scripts/test-pkg.sh

symtab:
	nm -Clp ${bin} | \
	awk '{ printf "%s %s %s\n", $$3, $$2, $$4 }' | \
	grep -v "@@GLIBC" | \
	sort -u > ${bin}.symtab

strings:
	strings ${bin} \
	> ${bin}.strings

extra_clean += ${bin}.symtab ${bin}.strings
