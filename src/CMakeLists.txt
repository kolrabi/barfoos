CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( Barfoos C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR})
include_directories(${GLEW_INCLUDE_DIR})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS 
  proto/entity.proto
  proto/game.proto
  proto/runningstate.proto
  proto/world.proto
)

CONFIGURE_FILE          ( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h )

# ADD_DEFINITIONS(-DDATA_PATH="${CMAKE_INSTALL_PREFIX}/share/barfoos/assets")
ADD_DEFINITIONS(-DHAVE_CONFIG)

SET ( SOURCES 

      main.cc

      audio/audio.cc

      game/game.cc
      
      game/entities/entity.cc 
      game/entities/mob.cc
      game/entities/monster.cc
      game/entities/player.cc
      game/entities/projectile.cc

      game/gameplay/stats.cc

      game/gamestates/mainmenu/mainmenustate.cc
      game/gamestates/mainmenu/mainmenugui.cc
      game/gamestates/running/runningstate.cc

      game/items/inventory.cc
      game/items/inventorygui.cc
      game/items/item.cc
      game/items/itemproperties.cc
      game/items/itementity.cc

      game/gameplay/effect.cc
      game/gameplay/spell.cc
      game/gameplay/trigger.cc

      game/world/cells/cell.cc
      game/world/cells/cellproperties.cc
      game/world/cells/cellrender.cc
      game/world/feature.cc
      game/world/world.cc
      game/world/worldbuilder.cc
      game/world/worldedit.cc

      # gfx/GLee.c
      gfx/gfx.cc
      gfx/gfxscreen.cc
      gfx/gfxview.cc
      gfx/shader.cc
      gfx/text.cc
      gfx/texture.cc
      gfx/vertexbuffer.cc

      gui/gui.cc

      io/fileio.cc
      io/input.cc
      io/properties.cc

      math/simplex.cc

      util/image.cc
      util/log.cc
      util/profile.cc
      util/util.cc

      ${PROTO_SRCS}
    )

add_executable( Barfoos ${SOURCES} )
target_link_libraries( Barfoos
  ${GLFW3_LIBRARIES}
  ${OPENGL_LIBRARIES} 
  ${PNG_LIBRARY} 
  ${ZLIB_LIBRARY}
  ${PROTOBUF_LIBRARY} 
  ${OPENAL_LIBRARY}
  ${VORBISFILE_LIBRARY}
  ${VORBIS_LIBRARY}
  ${OGG_LIBRARY}
  ${GLEW_LIBRARY_RELEASE}
)


