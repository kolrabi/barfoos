# vim:noet

SUBDIRS=

## Build ----------------------------------------------------------

CF=$(GL_CFLAGS) -I$(top_srcdir)/src

if WINDOOF
RESFILE=res.rc
else
RESFILE=
endif

.rc.o:
	windres $< -o $@

bin_PROGRAMS=barfoos
barfoos_CXXFLAGS=$(CF) -Werror -Wall -Wextra --std=c++0x
barfoos_CFLAGS=$(CF) -Werror -Wall -Wextra --std=c99 
barfoos_SOURCES=main.cc io.cc texture.cc player.cc cell.cc world.cc mob.cc worldedit.cc simplex.cc text.cc GLee.c \
  feature.cc template.cc entity.cc gfx.cc game.cc item.cc weapon.cc $(RESFILE)
barfoos_LDADD=

## Packaging ------------------------------------------------------

EXTRA_DIST=

