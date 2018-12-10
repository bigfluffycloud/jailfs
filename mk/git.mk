#
# mk/git.mk:
#	Some helper rules for dealing with git
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#
git-pull:
	git pull
	git submodule update

git-push:
	git push

git-commit:
	git commit

git-init:
	git submodule init
