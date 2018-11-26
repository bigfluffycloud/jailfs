jailfs:	A FUSE and libarchive filesystem for safe jails


There are a few dependencies:
* libarchive
* libbsd
* libev 
* libfuse 
* libmagic 
* libnwind 
* sqlite3

I'm working on bundling versions of these to allow building
a jailfs against musl+integrated trees... This will happen soon-ish

------------------

See examples/ for example jails

Quick start
-----------

```
# git submodule init
# git submodule update
# make world
```

## If using gentoo/funtoo, try this ;)
# for i in glibc busybox openssl pdns; do ./scripts/emege2pkg $i; done

## Start the jailfs instance
# ./jailfs examples/auth-dns


You should now have a a working chroot jail at
examples/auth-dns/root/
