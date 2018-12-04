# Convenience makefile...

all build clean world testpkg:
	${MAKE} -C ../ $@

test: umount
	(cd ../; ./jailfs examples/auth-dns)

umount:
	umount ../examples/auth-dns/root

qa:
	${MAKE} clean world testpkg test umount
