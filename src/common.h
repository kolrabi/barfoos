#ifndef BARFOOS_COMMON_H
#define BARFOOS_COMMON_H

// ====================================================================================

// autoconf
#include <config.h>

// often needed types and functions
#include <cstdint>
#include <cmath>

#include <string>

// pointer types
#include <memory>

// containers
#include <vector>

// ====================================================================================

// deal with old compilers
#if __cplusplus < 201103L
  #define final
  #define override
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
enum class  Element       : size_t;

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
struct Stats;
struct Texture;
struct Vector3;
struct Vertex;

class Cell;
class Entity;
class Game;
class GameState;
class Gfx;
class GLFWwindow;
class Gui;
class Item;
class Input;
class InventoryGui;
class Mob;
class Player;
class Random;
class RenderString;
class RunningState;
class Serializer;
class Shader;
class World;

namespace Const {
  constexpr float pi          = std::atan(1.0)*4.0;
  constexpr float pi_2        = std::atan(1.0)*2.0;
  constexpr size_t InvalidID  = ~0UL;
}

constexpr float operator"" _deg(long double f)        { return        f / 180 * Const::pi; }
constexpr float operator"" _deg(unsigned long long f) { return (float)f / 180 * Const::pi; }

#endif

