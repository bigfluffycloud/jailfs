#
# mk/release.mk:
#	Build release packages
#
# Copyright (C) 2018 Bigfluffy.cloud <joseph@bigfluffy.cloud>
#
# Distributed under a MIT license. Send bugs/patches by email or
# on github - https://github.com/bigfluffycloud/jailfs/
#
# No warranty of any kind. Good luck!
#

release: release-pre ${release_targets} ${extra_release_targets} release-post

release-pre:
	@echo "Building release packages"

release-post:
	@echo "Done!"
