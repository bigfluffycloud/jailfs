# enable more debugging output (slower, noisier)
CONFIG_DEBUG=y

# enable gprof profiling (slower, even noisier)
CONFIG_PROFILING=n

# debug block allocator? should be unneeded
CONFIG_DEBUG_BALLOC=n

# strip binaries? (debug disables, makes smaller)
CONFIG_STRIP_BINS=n

# strip libraries (debug disable, not recommended)
CONFIG_STRIP_LIBS=n

# use LD_PRELOAD (doesn't support setuid)
CONFIG_VFS_LDPRELOAD=y


####################
# Compiler options #
####################
CFLAGS += -O1 -g -pipe -ansi  -std=gnu99
CFLAGS += -I./src
CFLAGS += -D_DEFAULT_SOURCE -D_FILE_OFFSET_BITS=64 -fPIC -D_GNU_SOURCE
warn_noerror := -Wall -Wno-unused -Wno-strict-aliasing
#warn_flags := ${warn_noerror} #-Werror
warn_flags :=
LDFLAGS := -lz -lcrypto -pthread -lrt -lsqlite3 -lm -lev -lunwind -lfuse -lmagic -ldl -larchive -lbsd
lib_ldflags += -shared -ldl


#################
# Autoconfigure #
#################
ifeq (y, ${CONFIG_PROFILING})
CFLAGS += -pg
LDFLAGS += -pg
endif
