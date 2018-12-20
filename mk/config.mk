#
# mk/config.mk:
#	Build configuration support
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#
# enable gprof profiling (slower, noisy)
CONFIG_PROFILING=n

# debug block allocator? should be unneeded
CONFIG_DEBUG_BALLOC=n

# strip binaries? (breaks debugging, makes smaller binaries)
CONFIG_STRIP_BINS=n

# strip libraries (breaks debugging, makes smaller libraries)
CONFIG_STRIP_LIBS=n

# enable building the jailfs master process as a static linked
# single binary?
CONFIG_STATIC=y

# enable loadable modules: THIS HAS SECURITY IMPLICATIONS
# you should understand them before enabling it
CONFIG_MODULES=n

####################
# Compiler options #
####################
# Eventually we plan to build an entirely self-contained static binary...
CFLAGS += -O1 -g -pipe -ansi  -std=gnu99

ifeq (y, ${CONFIG_STATIC})
CC := bin/musl-gcc
CFLAGS += -nostdinc -I./include -I./src
endif

ifeq (y, ${CONFIG_MODULES})
CFLAGS += -DMODULES
endif

CFLAGS += -D_DEFAULT_SOURCE -D_FILE_OFFSET_BITS=64 -fPIC -D_GNU_SOURCE
warn_noerror := -Wall -Wno-unused -Wno-strict-aliasing
#warn_flags := ${warn_noerror} #-Werror
warn_flags :=
LDFLAGS := -L./lib/ -lz -lcrypto -pthread -lrt -lsqlite3 
LDFLAGS += -lm -lev -lunwind -lfuse -lmagic -ldl -larchive
LDFLAGS += -lbsd -ltomcrypt
lib_ldflags += -shared -ldl

#################
# Autoconfigure #
#################
ifeq (y, ${CONFIG_PROFILING})
CFLAGS += -pg
LDFLAGS += -pg
endif
