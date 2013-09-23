## vim:noet

## Build ----------------------------------------------------------

CF=$(GL_CFLAGS) -Werror -Wall -Wextra -I$(top_srcdir)/src -DDATA_PATH=\"$(pkgdatadir)\"
# -D_GLIBCXX_DEBUG

if WINDOOF
RESFILE=res.rc icon.ico
endif

.rc.o:
	windres $< -o $@

PROTOS=game.pb.cc runningstate.pb.cc entity.pb.cc world.pb.cc

%.pb.cc: %.proto
	$(PROTOC) --cpp_out=. $<

common.h.gch: common.h log.h profile.h
	$(CXX) -DHAVE_CONFIG_H -I. $(CXXFLAGS) $(CF) $<

BUILT_SOURCES=$(PROTOS) common.h.gch

bin_PROGRAMS=barfoos

barfoos_CXXFLAGS=$(CF)
barfoos_CFLAGS=$(CF)
barfoos_LDADD=
barfoos_LDFLAGS=
barfoos_SOURCES=\
  $(PROTOS) \
  $(RESFILE) \
  2d.h \
  aabb.h \
  audio.cc audio.h \
  cell.cc cell.h \
  cellproperties.cc cellproperties.h \
  cellrender.cc \
  common.h common.h.gch \
  effect.cc effect.h \
  entity.cc entity.h \
  feature.cc feature.h \
  fileio.cc fileio.h \
  game.cc game.h \
  gfx.cc gfx.h \
  gfxscreen.cc gfxscreen.h \
  gfxview.cc gfxview.h \
  GLee.c GLee.h \
  gui.cc gui.h \
  image.cc image.h \
  input.cc input.h \
  inventory.cc inventory.h \
  inventorygui.cc inventorygui.h \
  item.cc item.h \
  itementity.cc itementity.h \
  itemproperties.cc itemproperties.h \
  ivector3.h \
  log.cc log.h \
  main.cc \
  mainmenugui.cc mainmenugui.h \
  mainmenustate.cc mainmenustate.h \
  matrix4.h \
  mob.cc mob.h \
  monster.cc monster.h \
  player.cc player.h \
  profile.cc profile.h \
  projectile.cc projectile.h \
  properties.cc properties.h \
  random.h \
  runningstate.cc runningstate.h \
  shader.cc shader.h \
  simplex.cc simplex.h \
  space.h \
  spell.cc spell.h \
  stats.cc stats.h \
  text.cc text.h \
  texture.cc texture.h \
  trigger.cc trigger.h \
  util.cc util.h \
  vector2.h vector3.h vector4.h \
  vertexbuffer.cc vertexbuffer.h \
  world.cc world.h \
  worldbuilder.cc worldbuilder.h \
  worldedit.cc worldedit.h

## Packaging ------------------------------------------------------

EXTRA_DIST=libgcc_s_dw2-1.dll libstdc++-6.dll icon.ico res.rc.in


if MECKES
app:
	sh makeapp.sh
endif
