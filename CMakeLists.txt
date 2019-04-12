if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the SDL source code and call cmake from there")
endif()

cmake_minimum_required(VERSION 3.10)
project(BarfoosGame CXX C)

SET (PACKAGE_NAME "Barfoos")

enable_testing()

# Initialize CXXFLAGS.
set(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# Compiler-specific C++11 activation.
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7 or greater.")
    endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else ()
    message(FATAL_ERROR "Your C++ compiler does not support C++11.")
endif ()

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# prefer static
IF (MINGW)
  SET (CMAKE_FIND_LIBRARY_SUFFIXES .a .dll.a)
ENDIF ()

find_package( Protobuf   REQUIRED )
find_package( OpenGL     REQUIRED )
find_package( OpenAL     REQUIRED )
find_package( OggVorbis   )
find_package( PNG        REQUIRED )
find_package( ZLIB       REQUIRED )
find_package( PkgConfig  REQUIRED )
find_package( GLEW REQUIRED )

pkg_search_module( GLFW3 REQUIRED glfw3 )


SET ( HAVE_LIBOPENAL ${OPENAL_FOUND} )
SET ( HAVE_LIBOGG    ${OGG_LIBRARY} )
SET ( HAVE_LIBVORBISFILE  ${VORBISFILE_LIBRARY} )
SET ( HAVE_LIBVORBIS      ${VORBIS_LIBRARY} )

ADD_SUBDIRECTORY(src)
