#ifndef BARFOOS_COMMON_H
#define BARFOOS_COMMON_H

// ====================================================================================

// autoconf
#include <config.h>

// often needed types and functions
#include <cstdint>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <string>
#include <algorithm>
#include <functional>

// pointer types
#include <memory>

// containers
#include <vector>
#include <list>
#include <map>

// io
#include <iostream>
#include <sstream>

// ====================================================================================

// deal with old compilers
#if __cplusplus < 201103L
  #define final
  #define override

  #ifndef PRIu64
    #define PRIu64 "I64u"
  #endif
#endif

// useful if you need a reference to yourself
#define self (*this)

// ====================================================================================

#include "profile.h"
#include "log.h"

enum        Axis          : int;
enum        Corner        : int;
enum class  Side          : int;
enum class  InventorySlot : size_t;

struct AABB;
struct Animation;
struct FeatureInstance;
struct IColor;
struct IVector3;
struct InputEvent;
struct Matrix4;
struct Point;
struct Rect;
struct Sprite;
struct Texture;
struct Vector3;
struct Vertex;

class Cell;
class Entity;
class Game;
class Gfx;
class GLFWwindow;
class Gui;
class Item;
class Input;
class InventoryGui;
class Mob;
class Player;
class Random;
class Serializer;
class Shader;
class World;

namespace Const {
  constexpr float pi   = std::atan(1.0)*4.0;
  constexpr float pi_2 = std::atan(1.0)*2.0;
}

#endif

