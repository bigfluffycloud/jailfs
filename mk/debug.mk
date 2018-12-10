#
# mk/debug.mk:
#	Debugging stuff
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

ifeq (${CONFIG_DEBUG}, y)
CONFIG_STRIP_BINS=n
CFLAGS += -DCONFIG_DEBUG
endif

symtab:
	nm -Clp ${bin} | \
	awk '{ printf "%s %s %s\n", $$3, $$2, $$4 }' | \
	egrep -v "(@@|__FUNCTION)" | \
	sort -u > dbg/${bin}.symtab

strings:
	strings ${bin} \
	> dbg/${bin}.strings

extra_targets += symtab strings
extra_clean += dbg/${bin}.symtab dbg/${bin}.strings
