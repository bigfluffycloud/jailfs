jailfs:	A FUSE and libarchive filesystem for safe jails

Third Party Sites - 
[![Waffle.io - Issues in progress](https://badge.waffle.io/bigfluffycloud/jailfs.png?label=in%20progress&title=In%20Progress)](http://waffle.io/bigfluffycloud/jailfs)
[![Build Status](https://travis-ci.org/bigfluffycloud/jailfs.svg?branch=master)](https://travis-ci.org/bigfluffycloud/jailfs)

There are a few dependencies:
* libarchive
* libbsd
* libev 
* libfuse 
* libmagic 
* libunwind
* sqlite3

I'm working on bundling versions of these to allow building
a jailfs against musl+integrated trees... This will happen soon-ish

------------------

See examples/ for example jails

Quick start
-----------

## If using debian-based OS using shared libs (not needed for full-static)
```
apt install libarchive-dev libbsd-dev libev-dev libmagic-dev libfuse-dev libunwind-dev libsqlite3-dev
```
## All hosts:
```
# git submodule init
# git submodule update
# make world
```
## If using gentoo/funtoo, try this ;)
```
# for i in glibc busybox openssl pdns; do ./scripts/emerge2pkg $i; done
```
## Start the jailfs instance
```
# ./jailfs examples/auth-dns
```

You should now have a a working chroot jail at
examples/auth-dns/root/


--
If jailfs crashes and you aren't using warden to supervise it, you likely
will end up with a broken mount on the mountpoints in example/.


If you see errors similar to examples/auth-dns/root/: Transport endpoint is not connected

Please do the following (edited to the jail path of course):
```
umount examples/auth-dns/root/
```


--
If make complains similiar to this: make: *** No rule to make target 'src/linenoise.c', needed by '.obj/linenoise.o'.  Stop.

```
git submodule init; git submodule update
```

Then try again. You really should read the instructions above, if you want
to be successful using this mess....
