# enable more debugging output (slower, noisier)
CONFIG_DEBUG=y

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
CFLAGS += -Os -g -pipe
CFLAGS += -I. -I/usr/include/libxml2
CFLAGS += -D_DEFAULT_SOURCE -D_FILE_OFFSET_BITS=64 -fPIC -D_GNU_SOURCE
warn_noerror := -Wall -Wno-unused -Wno-strict-aliasing -ansi -std=c99
warn_flags := ${warn_noerror} #-Werror
LDFLAGS := -lxml2 -lz -lcrypto -pthread -lrt -lsqlite3 -lm -lev -lunwind -lfuse -lxar
lib_ldflags += -shared -ldl
