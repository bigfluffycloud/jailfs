# mk/bin.mk:
#	Rules for building host binaries
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

${bin}: $(${bin}_objs)
	@echo "[LD] ($^) => $@"
	@${CC} -o $@ ${LDFLAGS} ${extra_libs} $^
ifeq (${CONFIG_STRIP_BINS}, y)
	@echo "[STRIP] $@"
	@strip $@
endif

.obj/%.o:src/%.c
	@echo "[CC] $< => $@"
	@${CC} ${warn_flags} ${CFLAGS} -o $@ -c $<
