#!/bin/bash
#
# Generate some test packages

for i in glibc openssl pdns bash; do
   ./scripts/host2pkg $i
done
