#
# GNUmakefile:
#	Top level Makefile for building this project with GNU make
#
# Copyright (C) 2012-2019 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

# Redirect the common 'all' rule to 'world' and ensure it's default rule
all: world

# Include sub-makefiles, where all the real magic happens
include mk/version.mk
include mk/config.mk

# For now, this stuff is disabled as it may not even work anymore
#include mk/distcc.mk
#include mk/ccache.mk
include mk/ext.mk

# Project rules
include doc/rules.mk
include lsd/rules.mk
include src/rules.mk
include tests/rules.mk

# Core rules
include mk/bin.mk
include mk/indent.mk
include mk/release.mk
include mk/tests.mk
include mk/debug.mk
include mk/clean.mk
include mk/help.mk
include mk/git.mk

# Build *everything* suitable for a release
world: ${libs} ${bins} ${debug_targets} ${extra_targets}
