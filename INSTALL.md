Installing this is pretty straight forward.

We include major dependencies in ext/ via git submodule.

Right now, we do NOT build these, but soon will when we
make a jail builder and will need them..

# As root or via sudo: SU=sudo
cd /opt

# clone latest jailfs from github
$SU git clone https://github.org/bigfluffycloud/jailfs 

# pull sub-modules (external dependencies)
(cd jailfs; git submodule init; git submodule update)

# [RECOMMENDED] clone optional supervisor program from github
$SU git clone https://github.org/bigfluffycloud/warden

# Build jailfs
cd jailf
make config world test-pkg

# Start it without warden
./jailfs examples/auth-dns/


####
# If you want to use jailfs with warden, see /opt/warden/INSTALLING!
####

Debian
------
Try this:
```
 sudo apt install libev-dev libmagic-dev libfuse-dev fuse libunwind-dev libarchive-dev libbsd-dev
```
to install dependencies.
