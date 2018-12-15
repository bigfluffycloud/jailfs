#
# mk/clean.mk:
#	Rules for returning the source tree to a state suitable
# for redistribution or compilation.
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

clean: ${clean_targets}
	${RM} ${clean_objs} ${bins} ${extra_clean} ${libs} ${lib_objs}

distclean: ${distclean_targets} clean
	${RM} ${distclean_objs} ${extra_distclean}

clean-help:
	@echo -e "*\tclean      - Clean for (re)building"
	@echo -e "*\tdistclean  - Clean for redistribution"

help_targets += clean-help
