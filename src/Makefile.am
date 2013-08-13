## vim:noet

## Build ----------------------------------------------------------

CF=$(GL_CFLAGS) -Werror -Wall -Wextra -I$(top_srcdir)/src -DDATA_PATH=\"$(pkgdatadir)\" -pthread -D_GLIBCXX_DEBUG

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
  main.cc \
  gfx.cc gfx.h gfxview.cc gfxview.h \
  io.cc texture.cc \
  GLee.c GLee.h \
  game.cc game.h \
  runningstate.cc runningstate.h \
  mainmenustate.cc mainmenustate.h \
  mainmenugui.cc mainmenugui.h \
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
  projectile.cc projectile.h \
  \
  trigger.cc trigger.h \
  \
  cell.cc cell.h \
  cellproperties.cc cellproperties.h \
  cellrender.cc \
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
  deserializer.cc deserializer.h \
  input.cc input.h \
  profile.cc profile.h \
  log.cc log.h \
  $(RESFILE)

## Packaging ------------------------------------------------------

EXTRA_DIST=libgcc_s_dw2-1.dll libstdc++-6.dll icon.ico res.rc.in
