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

#######################
# musl - a small libc #
#######################
ARCH := $(uname -m)
kernel_headers_srcdir := ext/kernel-headers
musl_srcdir := ext/musl
musl_lib := lib/libc.so
libs += ${musl_lib}

lib/libc.so: ${musl_srcdir}/lib/libc.so
#	cp -arvx ${musl_srcdir}/lib/* lib/
#	cp -avrx ${musl_srcdir}/obj/musl-gcc bin/
#	cp -avrx ${musl_srcdir}/include/* include/
	${MAKE} -C ${musl_srcdir} DESTDIR=${PWD} install

${musl_srcdir}/configure:
	git submodule init; git submodule pull

${musl_srcdir}/config.mak:
	-${MAKE} -C ${musl_srcdir} distclean
	cd ${musl_srcdir}/; ./configure --prefix=/ --enable-wrapper=yes

${musl_srcdir}/lib/libc.so: ${musl_srcdir}/config.mak include/linux/utsname.h
	${MAKE} -C ${musl_srcdir}

include/linux/utsname.h:
	${MAKE} -C ${kernel_headers_srcdir} ARCH=x86_64 prefix=/ DESTDIR=${PWD} install

musl-clean:
	${MAKE} -C ${musl_srcdir} distclean

#######################
# ev - event handling #
#######################
ev_lib := lib/libev.so
ev_srcdir := ext/libev/
libs += ${ev_lib}

${ev_lib}: ${ev_srcdir}/libev.so
	${MAKE} -C ${ev_srcdir} DESTDIR=${PWD} install
#	./ellcc build x86_64-linux

${ev_srcdir}/libev.so:
	${MAKE} -C ${ev_srcdir}

${ev_srcdir}/Makefile: 
	./configure --prefix=/

${ev_srcdir}/configure:
	git submodule init; git submodule pull

##################################
# libbsd - bsd safe string stuff #
##################################
libbsd_lib := lib/libbsd.a
libbsd_srcdir := ext/libbsd
libs += ${libbsd_lib}

${libbsd_lib}: ${libbsd_srcdir}/src/.libs/libbsd.a
	${MAKE} -C ${libbsd_srcdir} DESTDIR=${PWD} install

${libbsd_srcdir}/src/.libs/libbsd.a:
	${MAKE} -C ${libbsd_srcdir}

${libbsd_srcdir}/Makefile: ${libbsd_srcdir}/configure
	(cd ${libbsd_srcdir}; ./configure --prefix=/ \
	--with-sysroot=${PWD}/lib)

${libbsd_srcdir}/configure: ${libbsd_srcdir}/autogen.sh
	(cd ${libbsd_srcdir}; ./autogen.sh)

${libbsd_srcdir}/autogen.sh:
	git submodule init; git submodule pull

####################################
# sqlite - Liteweight SQL database #
####################################
sqlite_srcdir := ext/sqlite
sqlite_lib := lib/libsqlite.a
libs += ${sqlite_lib}

${sqlite_lib}: ${sqlite_srcdir}/
${sqlite_srcdir}/lib:
	${MAKE} -C ${sqlite_srcdir}

${sqlite_srcdir}/Makefile: ${sqlite_srcdir}/configure
	CC=musl-gcc (cd ${sqlite_srcdir}; \
	./configure --prefix=/ --enable-tempstore=always \
	--disable-load-extension

${sqlite_srcdir}/configure:
	git submodule init; git submodule pull

###################################
# FUSE - Filesystems in USErspace #
###################################
ext_libs += libfuse.a
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

###############
# libtomcrypt #
###############
distclean_targets += musl-clean
extra_distclean += $(wild include/*.h)
extra_distclean += $(wildcard lib/*.o lib/*.so lib/*.a lib/*.specs) bin/musl-gcc

ext/libtomcrypt/libtomcrypt.a ext/libtomcrypt/libtomcrypt.so:
	CC=./bin/musl-gcc ${MAKE} -C ext/libtomcrypt -f makefile.shared

lib/libtomcrypt.a: lib/libtomcrypt.so
lib/libtomcrypt.so: ext/libtomcrypt/.libs/libtomcrypt.so
	cp ext/libtomcrypt/.libs/* lib/

libs += lib/libtomcrypt.a
libs += lib/libtomcrypt.so
