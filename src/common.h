#ifndef BARFOOS_COMMON_H
#define BARFOOS_COMMON_H

// ====================================================================================

// autoconf
#include "config.h"

#include <cmath>
#include <cstdint>
#include <string>

// pointer types
#include <memory>

#include <sstream>

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

enum        Axis          : int8_t;
enum        Corner        : int8_t;
enum class  Side          : int8_t;
enum class  SpawnClass    : char;
enum class  InventorySlot : uint8_t;
enum class  Element       : uint8_t;

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

class Audio;
class Cell;
class Deserializer;
class Entity;
class Game;
class GameState;
class Gfx;
class GfxView;
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
class VertexBuffer;
class World;

namespace Const {
  constexpr float pi          = std::atan(1.0)*4.0;
  constexpr float pi_2        = std::atan(1.0)*2.0;
  constexpr float rad2deg     = 180.0 / pi;
}

constexpr float operator"" _deg(long double f)        { return        f / 180 * Const::pi; }
constexpr float operator"" _deg(unsigned long long f) { return (float)f / 180 * Const::pi; }

typedef uint32_t ID;
constexpr ID InvalidID = (ID)0xFFFFFFFF;

template<class T>
std::string ToString(const T &v) {
  std::stringstream str;
  str << v;
  return str.str();
}

#endif
