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

world: ${libs} ${bin} symtab strings

testpkg:
	./scripts/testpkg.sh

symtab:
	nm -Clp ${bin} | \
	awk '{ printf "%s %s %s\n", $$3, $$2, $$4 }' | \
	grep -v "@@GLIBC" | \
	sort -u > dbg/${bin}.symtab

strings:
	strings ${bin} \
	> dbg/${bin}.strings

umount:
	-umount examples/auth-dns/root
#	-for i in examples/auth-dns/root; do \
#	    mountpoint $$i; \
#	    [ "$$?" == 0 ] && umount $$i; \
#	done

test: umount
	./jailfs examples/auth-dns

qa:
	${MAKE} clean world testpkg test umount

extra_clean += dbg/${bin}.symtab dbg/${bin}.strings
extra_clean += $(wildcard examples/*/state/*.pid)
