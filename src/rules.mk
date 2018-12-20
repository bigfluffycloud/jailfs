bins += jailfs
bins += warden
lsd_lib := lib/libsd.a
lsd_lib_so := lib/libsd.so

jailfs_objs += .obj/api.o
jailfs_objs += .obj/cell.o
jailfs_objs += .obj/conf.o
jailfs_objs += .obj/control.o
jailfs_objs += .obj/cron.o
jailfs_objs += .obj/database.o
jailfs_objs += .obj/debugger.o
jailfs_objs += .obj/file-magic.o
jailfs_objs += .obj/gc.o
jailfs_objs += .obj/hooks.o
jailfs_objs += .obj/i18n.o
jailfs_objs += .obj/kilo.o
jailfs_objs += .obj/linenoise.o
jailfs_objs += .obj/main.o
ifeq (y, ${CONFIG_MODULES})
jailfs_objs += .obj/module.o
endif
jailfs_objs += .obj/pkg.o
jailfs_objs += .obj/scripting.o
jailfs_objs += .obj/shell.o
jailfs_objs += .obj/threads.o
jailfs_objs += .obj/unix.o
jailfs_objs += .obj/vfs.o
warden_objs += .obj/warden.o

lsd_objs += .obj/atomicio.o
lsd_objs += .obj/balloc.o
lsd_objs += .obj/dict.o
lsd_objs += .obj/dlink.o
lsd_objs += .obj/list.o
lsd_objs += .obj/str.o
lsd_objs += .obj/timestr.o
lsd_objs += .obj/tree.o


lib/libsd.a: ${lsd_objs}
	${AR} -cvq $@ $^

lib/libsd.so: ${lsd_objs}
	${CC} -nostdlib -L./lib -fPIC -lbsd -lrt -pthread -shared $^ -o $@

.obj/%.o:src/lsd/%.c $(wildcard src/lsd/*.h src/*.h)
	@echo "[CC].lib $< => $@"
	@${CC} ${warn_flags} ${CFLAGS} -fPIC -o $@ -c $<

clean_objs += ${jailfs_objs} ${warden_objs} ${lsd_objs} ${lsd_lib} ${lsd_lib_so}
libs += ${lsd_lib} ${lsd_lib_so}
