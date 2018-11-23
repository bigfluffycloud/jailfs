This is a pretty old version of fs-pkg. I'm working to restore it to a
usable state. Unfortunately the newest version is on a drive that will not
spin up...

* Install libev xar and fuse.
# apt install xar libxar-dev libev-dev fuse-dev

* Build stuff
# make clean world

* Configure it
# joe fs-pkg.cf

* Run it

# ./fs-pkg



# Bugs
There's plenty of them. This is REALLY old code... a version before it was
"ruined" to meet the needs of a small dirty hack of a project at
$FormerEmployer.

If you can fix them, submit a diff -u or git diff to me at
joseph@bigfluffy.cloud

If you can at least send a crash report (edit fs-pkg.cf and set the below):

  log.level=debug
  log.stack_unwind=true

Then send your log file (default is fs-pkg.log) by email.

Debug log level WILL reduce performance. stack_unwind should not have a
noticable effect on normal operation.
