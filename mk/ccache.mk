#
# mk/ccache.mk:
#	Support for building with ccache
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#
CCACHE := /usr/bin/ccache
CCACHE ?= $(which ccache)

# Try to use ccache, if available
ifeq (${CCACHE}, $(wildcard ${CCACHE}))
CC := ${CCACHE} ${CC}
endif
