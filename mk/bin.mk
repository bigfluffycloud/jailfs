ifeq (${CONFIG_TOC_LIBXML2}, y)
CFLAGS += -DCONFIG_TOC_LIBXML2
endif
CFLAGS += -DCONFIG_VFS_FUSE

include src/rules.mk

${bin}: $($(bin)_objs)
	@echo "[LD] ($^) => $@"
	@${CC} -o $@ ${LDFLAGS} ${extra_libs} $^
ifeq (${CONFIG_STRIP_BINS}, y)
	@echo "[STRIP] $@"
	@strip $@
endif

.obj/%.o:src/%.c
	@echo "[CC] $< => $@"
	@${CC} ${warn_flags} ${CFLAGS} -o $@ -c $<
