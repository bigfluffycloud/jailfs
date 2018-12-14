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

#######################
# musl - a small libc #
#######################
musl_srcdir := ext/musl
musl_lib := lib/libc.so

lib/libc.so: ${musl_srcdir}/lib/libc.so
	cp -arvx ${musl_srcdir}/lib/* lib/
	cp -avrx ${musl_srcdir}/obj/musl-gcc bin/
	cp -avrx ${musl_srcdir}/include/* inc/

${musl_srcdir}/configure:
	git submodule init; git submodule pull

${musl_srcdir}/lib/libc.so: ${musl_srcdir}/configure
	-${MAKE} -C ${musl_srcdir} distclean
	cd ${musl_srcdir}/; ./configure --prefix=/ --enable-wrapper=yes
	${MAKE} -C ${musl_srcdir}

musl-clean:
	${MAKE} -C ${musl_srcdir} distclean

libs += lib/libc.so

distclean_targets += musl-clean
extra_distclean += $(wild inc/*.h)
extra_distclean += $(wildcard lib/*.o lib/*.so lib/*.a lib/*.specs) bin/musl-gcc
