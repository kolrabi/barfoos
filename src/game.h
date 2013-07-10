#ifndef BARFOOS_GAME_H
#define BARFOOS_GAME_H 

#include "common.h"
#include "feature.h"
#include "random.h"

class World;
class Player;
class Mob;
class InventoryGui;
class Gui;
class Entity;
class Gfx;
class Input;
struct InputEvent;

class Game final {
public:

  Game(const std::string &seed, size_t level = 0, const Point &screenSize = Point(320, 240));
  Game(const Game &game) = delete;
  Game(Game &&game) = delete;
  ~Game();
  
  bool Init();

  bool Frame();
  
  Gfx    *GetGfx()    const { return gfx;           }
  Input  *GetInput()  const { return input;         }
  World  &GetWorld()  const { return *this->world;  }
  Random &GetRandom()       { return random;        }

  float   GetTime()   const { return this->lastT;   }
  float   GetDeltaT() const { return this->deltaT;  }
  float   GetFPS()    const { return this->fps;     }
  float   GetThinkFraction() const; // TODO: move to entity

  int     GetLevel()  const { return level;         }
  
  void HandleEvent(const InputEvent &event);

  size_t AddEntity(Entity *entity);
  size_t AddPlayer(Player *entity);
  void RemoveEntity(size_t entity);
  bool CheckEntities(const IVector3 &pos);
  std::vector<size_t> FindEntities(const AABB &aabb);
  std::vector<size_t> FindSolidEntities(const AABB &aabb);
  temp_ptr<Entity> GetEntity(size_t id);

  Vector3 MoveAABB(const AABB &aabb, const Vector3 &dir, uint8_t &axis);
  
private:

  Input *input;
  Gfx *gfx;
  bool isInit;
  
  size_t handlerId;
  
  void Deinit();
  
  void Render() const;
  void Update(float t, float deltaT);
  
  std::shared_ptr<World> world;
  int level;
  
  std::map<size_t, Entity*> entities;
  Player *player;
  size_t nextEntityId;
  size_t GetNextEntityId() { return nextEntityId++; }
  
  std::shared_ptr<InventoryGui> inventoryGui;
  std::shared_ptr<Gui> activeGui;

  float startT;
  float lastT;
  float deltaT;
  float nextThinkT;
  size_t frame;
  float lastFPST;
  
  float fps;
  
  std::string seed;
  Random random;
  
  bool showInventory;
  
  void BuildWorld();
};

#endif

