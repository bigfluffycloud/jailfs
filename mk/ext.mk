# Build rules for third party (git-hosted) code...
ext_libs += libfuse.a
ext_libs += libxar.a


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

#############################
# xar - eXtensible ARchiver #
#############################
xar_srcdir := ext/xar/xar/lib/
xar_objdir := .obj/xar/
xar_src += archive arcmod b64 bzxar data ea err ext2
xar_src += filetree
xar_src += hash io linuxattr lzmaxar script
xar_src += signature stat subdoc util zxar
libxar_cflags := -I${xar_srcdir}
libxar_ldflags :=

libxar_obj := $(foreach y,${xar_src},${xar_objdir}/${y})
libxar.a: $(foreach y,${xar_src},${xar_objdir}/${y})
	@echo "[LD] $^ => $@"
	@${CC} ${libxar_ldflags} -o $@ $^

${xar_objdir}/%.o:${xar_srcdir}/%.c
	@echo "[CC] $< = $@"
	@${CC} ${libxar_cflags} -o $@ $^

objs += ${libxar_obj} ${libfuse_obj}
