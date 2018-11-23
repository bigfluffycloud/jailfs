This is a pretty old version of fs-pkg. I'm working to restore it to a
usable state. Unfortunately the newest version is on a drive that will not
spin up...

# Install libev xar and fuse.
* apt install xar libxar-dev libev-dev fuse-dev

# Build stuff
* make clean world

# Configure it
* joe fs-pkg.cf

# Run it
* ./fs-pkg &

# Build a test package
* make testpkg

# Confirm that inotify, etc work ---

[2018/11/23 01:04:57]      info: importing pkg pkg/irssi.pkg
[2018/11/23 01:04:57]     debug: SQL query [0]: BEGIN; 
[2018/11/23 01:04:57]     debug: SQL query [0]: DELETE FROM files WHERE package IN (SELECT id FROM packages WHERE path = 'pkg/irssi.pkg'); 
[2018/11/23 01:04:57]     debug: SQL query [0]: DELETE FROM packages WHERE path = 'pkg/irssi.pkg'; 
[2018/11/23 01:04:57]     debug: SQL query [0]: INSERT INTO packages (path, version) VALUES ('pkg/irssi.pkg', 1542953097); 
[2018/11/23 01:04:57]     debug: SQL query [1]: SELECT id FROM packages WHERE path = 'pkg/irssi.pkg' AND version = 1542953097; 
id: 2
[2018/11/23 01:04:57]     debug: package pkg/irssi.pkg seems valid, committing...
[2018/11/23 01:04:57]     debug: SQL query [0]: COMMIT; 


# You should see files in your mountpoint (path.mountpoint) if everything is working

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
