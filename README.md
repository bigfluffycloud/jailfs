See examples/ for example jails

Quick start
-----------

# git submodule init
# git submodule update
# make world

## If using gentoo/funtoo, try this ;)
# for i in glibc busybox openssl pdns; do ./scripts/emege2pkg $i; done

## Start the jailfs instance
# ./jailfs examples/auth-dns


You should now have a a working chroot jail at
examples/auth-dns/root/
