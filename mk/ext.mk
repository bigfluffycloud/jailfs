#
# mk/ext.mk:
#	Rules to build external libraries we need, if missing from the host
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

# Build rules for third party (git-hosted) code...
ext_libs += libfuse.a

###################################
# FUSE - Filesystems in USErspace #
###################################
fuse_srcdir := ext/libfuse/lib/
fuse_objdir := .obj/libfuse/
fuse_src += buffer fuse fuse_loop fuse_loop_mt
fuse_src += fuse_lowlevel fuse_opt fuse_signals
fuse_src += helper mount mount_bsd mount_util
libfuse_cflags := ${libfuse_srcdir}
libfuse_ldflags :=
libfuse_obj := $(foreach y,${fuse_src},${fuse_objdir}/${y})
libfuse.a: $(foreach y,${fuse_src},${fuse_objdir}/${y})
	@echo "[LD] $^ => $@"
	@${CC} ${libfuse_ldflags} -o $@ $^

${fuse_objdir}/%.o:${fuse_srcdir}/%.c
	@echo "[CC] $< => $@"
	@${CC} ${libfuse_cflags} -o $@ -c $^

objs += ${libfuse_obj}
