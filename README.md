This tree is currently in a kinda broken state.

I'm actively working to repair the bitrot and get the
repo to a minimal usable state this weekend.

Hopefully you'll enjoy and find time to contribute!

The end goal is to provide a mechanism for packaging
software for use in containers.

Example:
/pkg/*.pkg		- Main package repo (all packages)
/rw/*.spill		- Spillover files (edits to pkg)
/config/$jailname/	- Configuration for fs-etc
/jails/$jailname/	- Mountpoint for fs-pkg


Packages:
/pkg/pdns.pkg
	/usr/bin/pdns_server

jailname: auth-dns
	/config/auth-dns/etc/powerdns/named.conf
	/pkg/pdns.pkg
	/jails/auth-dns/		- Root directory
	/jails/pdns/usr/bin/pdns_server	- pdns daemon
	/jails/etc/powerdns/named.conf	- bind zone config

... Add all the other missing files ;) ...
