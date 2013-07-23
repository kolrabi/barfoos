## vim:noet

## Build ----------------------------------------------------------

CF=$(GL_CFLAGS) -Werror -Wall -Wextra -I$(top_srcdir)/src -DDATA_PATH=\"$(pkgdatadir)\"

if WINDOOF
RESFILE=res.rc icon.ico
LDWINFLAGS=-mwindows 
#-Wl,-Map=barfoos.map
else
RESFILE=
LDWINFLAGS=
endif

.rc.o:
	windres $< -o $@

bin_PROGRAMS=barfoos

barfoos_CXXFLAGS=$(CF) --std=c++0x
barfoos_CFLAGS=$(CF) --std=c99 
barfoos_LDADD=
barfoos_LDFLAGS=$(LDWINFLAGS)
barfoos_SOURCES=\
  gfx.cc main.cc io.cc texture.cc \
  GLee.c GLee.h \
  game.cc game.h \
  runningstate.cc \
  simplex.cc simplex.h \
  text.cc text.h \
  shader.cc shader.h \
  stats.cc stats.h \
  \
  properties.cc properties.h \
  \
  gui.cc gui.h \
  inventorygui.cc inventorygui.h \
  \
  entity.cc entity.h \
  mob.cc mob.h \
  player.cc player.h \
  itementity.cc itementity.h \
  particle.cc particle.h \
  projectile.cc projectile.h \
  \
  cell.cc cell.h \
  feature.cc feature.h \
  world.cc world.h \
  worldedit.cc worldedit.h \
  \
  item.cc item.h \
  inventory.cc inventory.h \
  effect.cc effect.h \
  \
  common.h ivector3.h space.h vector3.h \
  random.h \
  util.cc util.h \
  aabb.h 2d.h \
  serializer.cc serializer.h \
  input.cc input.h \
  profile.cc profile.h \
  log.cc log.h \
  $(RESFILE)

## Packaging ------------------------------------------------------

EXTRA_DIST=libgcc_s_dw2-1.dll libstdc++-6.dll icon.ico res.rc.in
