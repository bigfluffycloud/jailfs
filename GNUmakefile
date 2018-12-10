#
# GNUmakefile:
#	Top level Makefile for building this project with GNU make
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

# Redirect the common 'all' rule to 'world' and ensure it's default rule
all: world

# Include sub-makefile, where all the real magic happens
include mk/config.mk
#include mk/distcc.mk
#include mk/ccache.mk
#include mk/ext.mk
include mk/bin.mk
include mk/debug.mk
include mk/clean.mk
include mk/indent.mk
include mk/tests.mk

# Build *everything* 
world: ${libs} ${bin} symtab strings
