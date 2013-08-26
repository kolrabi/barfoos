#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_INIT(barfoos, 0.1, barfoos@kolrabi.de)
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADER([src/config.h])
AM_INIT_AUTOMAKE

AC_SUBST(VER_MAJ, 0)
AC_SUBST(VER_MIN, 1)
AC_SUBST(VER_SUB, 0)
AC_SUBST(VER_BLD, 0)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

LDFLAGS="-L/opt/local/lib"
CFLAGS="-I/opt/local/include"
CXXFLAGS="-I/opt/local/include"

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)

AM_CONDITIONAL([WINDOOF], [test x$OS = xWindows_NT])
AM_CONDITIONAL([MECKES], [test x$OSTYPE = xdarwin11])
if test x$OS = xWindows_NT; then
  AC_DEFINE(WIN32)
  AC_CHECK_LIB(winmm, main)
  AC_CHECK_LIB(opengl32, main)
  AC_CHECK_LIB(gdi32, main)
  AC_CHECK_LIB(glu32, main)
elif test x$OSTYPE = xdarwin11; then
  AC_DEFINE(MACOSX)
else
  AC_CHECK_LIB(GL, main)
  AC_CHECK_LIB(GLU, main)
fi

AC_CHECK_HEADER(GLFW/glfw3.h)
AC_CHECK_HEADER(AL/al.h)
AC_CHECK_HEADER(vorbis/vorbisfile.h)
AC_CHECK_HEADER(vorbis/vorbisenc.h)

AC_CHECK_LIB(X11, [XOpenDisplay], [AC_DEFINE(X11)])
AC_CHECK_LIB(X11, XConvertSelection)
AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryExtension)
AC_CHECK_LIB(Xrandr, main)
AC_CHECK_LIB(Xi, main)
AC_CHECK_LIB(glfw3, main)
AC_CHECK_LIB(z, main)
AC_CHECK_LIB(png, main)
AC_CHECK_LIB(openal, main)
AC_CHECK_LIB(OpenAL32, main)
AC_CHECK_LIB(ogg, main)
AC_CHECK_LIB(vorbis, main)
AC_CHECK_LIB(vorbisfile, main)

AC_CHECK_FUNCS([clock_gettime])
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_C_INLINE

AC_HEADER_STDBOOL

AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T

AH_TEMPLATE(WIN32, [define if compiling for windows])
AH_TEMPLATE(X11, [define if compiling for X11])
AH_TEMPLATE(MACOSX, [define if compiling for Mac OS X])

#CFLAGS="-O0 -ggdb"
#CXXFLAGS="-O0 -ggdb"
CFLAGS="$CFLAGS -O2 -ggdb"
CXXFLAGS="$CXXFLAGS -O2 -ggdb"

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/res.rc
                 ])
AC_OUTPUT

