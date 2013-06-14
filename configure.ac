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

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)

AM_CONDITIONAL([WINDOOF], [test x$OS = xWindows_NT])
if test x$OS = xWindows_NT; then
  AC_DEFINE(WIN32)
  AC_CHECK_LIB(winmm, main)
  AC_CHECK_LIB(opengl32, main)
  AC_CHECK_LIB(gdi32, main)
  AC_CHECK_LIB(glu32, main)
else
  # check for x11 only if no win32 is available
  AC_CHECK_LIB(X11, [XOpenDisplay], [AC_DEFINE(X11)])
  AC_CHECK_LIB(GL, main)
  AC_CHECK_LIB(GLU, main)
fi

AC_CHECK_LIB(glfw, main)
AC_CHECK_LIB(z, main)
AC_CHECK_LIB(png, main)

AH_TEMPLATE(WIN32, [define if compiling for windows])
AH_TEMPLATE(X11, [define if compiling for X11])

#CFLAGS="-O0 -ggdb -pg"
#CXXFLAGS="-O0 -ggdb -pg"
CFLAGS="-O2 -g3 -gdwarf-2 -pg"
CXXFLAGS="-O2 -g3 -gdwarf-2 -pg"

AC_CONFIG_FILES([Makefile 
                 src/Makefile
                 src/res.rc
                 ])
AC_OUTPUT

