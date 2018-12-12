# warden
Warden supervises jails created using jailfs (formerly fs-pkg)

Right now warden doesn't do much. It ensures a jailfs instance and a
container init script are always running, for each container configured.


Eventually, you will be able to use warden and jailfs to construct
safe jails using as many Linux isolation features as possible.

edit cf/warden.cf to configure
* Edit options in [general] section
Especially path.jails!

* Enable each jail desired by adding a line containing only it's name in [jails] section.

Example:
```
[general]
ui.detach=false
path.jailfs=./jailfs
path.jails=examples/
path.log=warden.log
path.db=:memory:

[jails]
auth-dns
syslog-master
```

* Run warden:
```
./warden
```

If you wish warden to run in the background (why?) use -d or general:ui.detach=true
