#
# mk/clean.mk:
#	Rules for returning the source tree to a state suitable
# for redistribution or compilation.
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/fs-pkg/
#
# No warranty of any kind. Good luck!
#

clean:
	${RM} ${clean_objs} ${bin} ${extra_clean} ${libs} ${lib_objs}

distclean:
	@${MAKE} clean

clean-help:
	@echo "*\tclean      - Clean for (re)building"
	@echo "*\tdistclean  - Clean for redistribution"

help_targets += clean-help
