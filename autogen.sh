#!/bin/sh

rm -f config.cache acconfig.h

echo "- aclocal." && aclocal -I . &&
echo "- autoconf." && autoconf && 
echo "- autoheader."&& autoheader && 
echo "- automake." && automake --add-missing --gnu && 
./configure "$@"

