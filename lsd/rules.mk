# LSD - library of Safe Datatypes (and tools)
lsd_srcdir := lsd/
lsd_lib := lib/libsd.a
lsd_lib_so := lib/libsd.so
libs += ${lsd_lib} ${lsd_lib_so}
lsd_objs += .obj/lsd/atomicio.o
lsd_objs += .obj/lsd/balloc.o
lsd_objs += .obj/lsd/dict.o
lsd_objs += .obj/lsd/dlink.o
lsd_objs += .obj/lsd/list.o
lsd_objs += .obj/lsd/str.o
lsd_objs += .obj/lsd/timestr.o
lsd_objs += .obj/lsd/tree.o

lib/libsd.a: ${lsd_objs}
	${AR} -cvq $@ $^

lib/libsd.so: ${lsd_objs}
	${CC} -nostdlib -L./lib -fPIC -lbsd -lrt -pthread -shared $^ -o $@

.obj/lsd/%.o:lsd/%.c $(wildcard ${lsd_srcdir}/*.h)
	@echo "[CC].lib $< => $@"
	@${CC} ${warn_flags} ${CFLAGS} -fPIC -o $@ -c $<
clean_objs += ${lsd_objs} ${lsd_lib} ${lsd_lib_so}
