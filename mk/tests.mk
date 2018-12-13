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

# Here we only build packages that don't exist
# See make clean-pkg for an attempt to avoid using 'make clean'
testpkg:
	@for i in glibc openssl pdns bash nginx; do \
	   if [ ! -f pkg/$$i.tar ]; then \
	      ./scripts/host2pkg $$i; \
	   fi; \
	done

clean-pkgs:
	rm -f pkg/*.tar

test: ${bin} umount
	rm -f examples/auth-dns/state/jailfs.pid
	./jailfs examples/auth-dns

qa:
	${MAKE} clean world testpkg test umount

# This ensures even if somehow the service died/got killed we clean up
# after ourselves
umount:
	-umount examples/auth-dns/root
	-umount examples/auth-dns/cache
	-rm -f examples/auth-dns/state/jailfs.pid

tests-help:
	@echo -e "*\ttest       - Run a test session"
	@echo -e "*\ttestpkg    - Build packages for examples"
	@echo -e "*\tclean-pkgs - Clean out package dir"
	@echo -e "*\tqa         - Quality Assurance mode"

help_targets += tests-help
distclean_targets += clean-pkgs
