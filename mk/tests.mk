#
# mk/tests.mk
#	Testing framework for the build system
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

extra_clean += $(wildcard examples/*/state/*.db)
extra_clean += $(wildcard examples/*/state/*.pid)
extra_clean += $(wildcard examples/*/log/*.log)

testpkg:
	./scripts/testpkg.sh

test: ${bin} umount
	rm -f examples/auth-dns/state/jailfs.pid
	./jailfs examples/auth-dns

qa:
	${MAKE} clean world testpkg test umount

umount:
	# This ensures even if somehow the service died/got killed we clean up
	# after ourselves
	-umount examples/auth-dns/root
	-umount examples/auth-dns/cache
