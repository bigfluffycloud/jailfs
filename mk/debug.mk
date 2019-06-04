#
# mk/debug.mk:
#	Debugging stuff
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

dbg/%.symtab: bin/%
	nm -Clp -f posix $< > $@

dbg/%.strings: bin/%
	strings $< > $@

debug_targets += $(foreach y,${bins},dbg/$(notdir ${y}).symtab)
debug_targets += $(foreach y,${bins},dbg/$(notdir ${y}).strings)
debug_clean += ${debug_targets}

debug: debug-pre ${debug_targets} debug-post

debug-pre:
	@echo "Building debugging data..."

debug-post:
	@echo "Finished building debugging data!"

extra_target += debug
extra_clean += ${debug_clean}
