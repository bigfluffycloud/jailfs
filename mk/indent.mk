#
# mk/indent.mk:
#	Mangle^WIndent the source code to make it more readable???
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#
# This is bad... We need to clean it up before someone runs this target again....
id_blanks := -nbad -bap -nbbb -sob -hnl
id_comments := -lc110 -fc1 -c40 -cd40 -cp40 -cdb -sc -d0 -nfc1
id_statements := -br -ce -cdw -cli3 -npcs -ss -ncs -nbs -saf
id_statements += -sai -saw -nprs
id_declr := -di12 -nbc -nbfda -nbfde -npsl -brs -brf
id_misc := -lp -ip0 -nlps -ppi0 -il3 -nbbo -i3 -ci4 -l100
id_misc += -ts3 -i3 -nut

indent:
	false
	@for i in *.[ch]; do \
	    echo "* indenting $$i"; \
	    indent ${id_blanks} ${id_comments} ${id_statements} ${id_declr} ${id_misc} $$i; \
	    ${RM} $$i~; \
	 done
