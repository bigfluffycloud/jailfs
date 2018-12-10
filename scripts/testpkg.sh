#!/bin/bash
#
# Generate some test packages

for i in glibc openssl pdns bash nginx; do
   [ ! -f pkg/$i.tar ] && ./scripts/host2pkg $i
done
