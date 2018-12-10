#
# mk/distcc.mk:
#	Support for distcc (Distributed C Compiler) building
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

DISTCC := /usr/bin/distcc
# Is DISTCC_HOSTS set?
ifneq (y${DISTCC_HOSTS}, y)
# Try to use distcc, if available
ifeq (${DISTCC}, $(wildcard ${DISTCC}))
CC := ${DISTCC}
endif
endif

export DISTCC_HOSTS
