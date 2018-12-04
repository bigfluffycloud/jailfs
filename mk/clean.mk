clean:
	${RM} ${clean_objs} ${bin} ${extra_clean} ${libs} ${lib_objs}

distclean:
	@${MAKE} clean
