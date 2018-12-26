bins += bin/jailfs


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

clean_objs += ${jailfs_objs} ${warden_objs}
