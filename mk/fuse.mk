ifeq (${CONFIG_VFS_FUSE}, y)
CFLAGS += -DCONFIG_VFS_FUSE
objs += .obj/vfs_fuse.o
endif

# Until i figure out how to work around these, let them throw warnings but dont abort comile....
.obj/fuse/%.o:fuse/%.c
	@echo "[CC] $< => $@"
	@${CC} ${warn_noerror} ${CFLAGS} -DFUSE_USE_VERSION=26 -DPACKAGE_VERSION="\"2.8.0-pre1\"" -DFUSERMOUNT_DIR="\"/bin\"" -DFUSERMOUNT_BIN="\"fusermount\"" -o $@ -c $<
.obj/vfs_fuse.o:vfs_fuse.c vfs_fuse.h vfs_pkg.h vfs_spill.h
	@echo "[CC] $< => $@"
	@${CC} ${warn_noerror} ${CFLAGS} -o $@ -c $<
