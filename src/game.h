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

class Game final {
public:

  Game(const std::string &seed, size_t level = 0, const Point &screenSize = Point(320, 240));
  ~Game();
  static Game *Instance;
  
  bool Init();

  bool Frame();
  
  Gfx *GetGfx() const { return gfx; }

  float GetTime() const { return this->lastT; }
  float GetDeltaT() const { return deltaT; }
  float GetThinkFraction() const;
  
  void OnMouseMove(const Point &pos);
  void OnMouseClick(const Point &pos, int button, bool down);
  void OnMouseDelta(const Point &delta);
  
  size_t AddEntity(Entity *entity);
  size_t AddPlayer(Player *entity);
  void RemoveEntity(size_t entity);
  bool CheckEntities(const IVector3 &pos);
  std::vector<size_t> FindEntities(const AABB &aabb);
  temp_ptr<Entity> GetEntity(size_t id);
  std::shared_ptr<World> GetWorld() { return this->world; }
  
private:

  Gfx *gfx;
  bool isInit;
  
  void Deinit();
  
  void Render() const;
  void Update(float t, float deltaT);
  
  std::shared_ptr<World> world;
  
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
  
  std::string seed;
  Random random;
  size_t level;
  
  bool showInventory;
  
  void BuildWorld(size_t level);
};

#endif

