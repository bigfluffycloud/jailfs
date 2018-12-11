#
# mk/version.sh:
#	Tools to brand the resulting binaries and libraries
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

PKG_VERSION=$(cat .version)
CFLAGS += -DPKG_VERSION="\"${PKG_VERSION}\""
