#!/bin/sh

~/tmp/bin/IncludeChecker \
 -I J:/msys/local/include \
 -I J:/msys/include \
 -I j:/mingw64/include \
 -I j:/mingw64/lib/gcc/x86_64-w64-mingw32/4.8.1/include \
 -I j:/mingw64/lib/gcc/x86_64-w64-mingw32/4.8.1/include-fixed \
 -I j:/mingw64/include/c++/4.8.1 \
 -I j:/mingw64/x86_64-w64-mingw32/include \
 *.cc

