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

class Game final {
public:

  Game(const std::string &seed, size_t level = 0);
  ~Game();
  static Game *Instance;

  void Render() const;
  void Update(float t, float deltaT);

  float GetTime() const { return this->lastT; }
  float GetDeltaT() const { return deltaT; }
  
  void OnMouseMove(const Point &pos);
  void OnMouseClick(const Point &pos, int button, bool down);
  void OnMouseDelta(const Point &delta);
  void OnKey(int key, bool down);
  
  size_t AddEntity(Entity *entity);
  size_t AddPlayer(Player *entity);
  void RemoveEntity(size_t entity);
  bool CheckEntities(const IVector3 &pos);
  std::vector<size_t> FindEntities(const AABB &aabb);
  temp_ptr<Entity> GetEntity(size_t id);
  std::shared_ptr<World> GetWorld() { return this->world; }
  
private:

  std::shared_ptr<World> world;
  
  std::map<size_t, Entity*> entities;
  Player *player;
  size_t nextEntityId;
  size_t GetNextEntityId() { return nextEntityId++; }
  
  std::shared_ptr<InventoryGui> inventoryGui;
  std::shared_ptr<Gui> activeGui;

  float lastT;
  float deltaT;
  
  std::string seed;
  Random random;
  
  bool showInventory;
  
  void BuildWorld(size_t level);
};

#endif

