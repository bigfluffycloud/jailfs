#
# mk/help.mk:
#	Help targets for the build system
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

help: help-pre $(sort ${help_targets}) help-post


help-pre:
	@echo "jailfs build system"
	@echo ""
	@echo "Available targets:"
	@echo "*\tworld      - Build everything"

help-post:
	@echo ""
	@echo "Good luck! Please report bugs to joseph@bigfluffy.cloud"