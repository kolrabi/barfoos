## vim:noet

## Build ----------------------------------------------------------

CF=$(GL_CFLAGS) -Werror -Wall -Wextra -I$(top_srcdir)/src -DDATA_PATH=\"$(pkgdatadir)\"
# -D_GLIBCXX_DEBUG

if WINDOOF
RESFILE=res.rc icon.ico
LDWINFLAGS=#-mwindows -Wl,-Map=barfoos.map
endif

if MECKES
LDMACOSXFLAGS=-framework OpenGL -framework cocoa -framework IOKit -framework OpenAL
CCMACOSXFLAGS=
endif

.rc.o:
	windres $< -o $@

bin_PROGRAMS=barfoos

barfoos_CXXFLAGS=$(CF) -std=c++11 $(CCMACOSXFLAGS)
barfoos_CFLAGS=$(CF) -std=c99
barfoos_LDADD=
barfoos_LDFLAGS=$(LDWINFLAGS) $(LDMACOSXFLAGS)
barfoos_SOURCES=\
  main.cc \
  image.cc image.h \
  gfx.cc gfx.h gfxview.cc gfxview.h \
  vertexbuffer.cc vertexbuffer.h \
  audio.cc audio.h \
  fileio.cc fileio.h \
  texture.cc texture.h \
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
  monster.cc monster.h \
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
  itemproperties.cc itemproperties.h \
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


if MECKES
app:
	sh makeapp.sh
endif
