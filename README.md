This tree is currently in a kinda broken state.

I'm actively working to repair the bitrot and get the
repo to a minimal usable state this weekend.

Hopefully you'll enjoy and find time to contribute!

The end goal is to provide a mechanism for packaging
software for use in containers.

---
* Goals for 0.1.0
- Working target mounts
- libarchive support
- code de-duplication
- sanitization and security checks

---
* Examples

Example: ./fs-pkg --jail pdns
* /pkg/*.pkg		- Main package repo (all packages)
* /rw/*.spill		- Spillover files (edits to pkg)
*/config/$jailname/	- Configuration for fs-etc
* /jails/$jailname/	- Mountpoint for jailfs

Packages:
/pkg/pdns.pkg
*	/usr/bin/pdns_server

jailname: auth-dns
::: On the host FS :::
*	/config/auth-dns/etc/powerdns/named.conf
*	/config/auth-dns/etc/powerdns/zones/bigfluffy.cloud
*	/config/auth-dns/fs-pkg.cf	- fs-pkg configuration
*	/pkg/pdns.pkg			- PowerDNS package
*	/rw/authdns-pdns.spill		- Spillover for pdns

::: Creates this in the jail (dir /jails/auth-dns) :::
*	/etc/powerdns/named.conf	- bind zone config
*	/etc/powerdns/zones/bigfluffy.cloud
*	/usr/bin/pdns_server		- pdns daemon
*    ... Add all the other missing files in pdns.pkg ;) ...

