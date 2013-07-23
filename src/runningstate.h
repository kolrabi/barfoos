#ifndef BARFOOS_RUNNINGSTATE_H
#define BARFOOS_RUNNINGSTATE_H 

#include "game.h"

class RunningState : public GameState {
public:

                        RunningState(Game &);
  virtual               ~RunningState();
  
  virtual void          Enter ()                    override;
  virtual void          Leave (GameState *)         override;
  virtual GameState *   Update()                    override;
  virtual void          Render(Gfx &)               const override;
  virtual void          HandleEvent(const InputEvent &) override;

  void                  NewGame();
  
  World  &              GetWorld()                  const { return *this->world;  }
  int                   GetLevel()                  const { return this->level;   }

  // entity management
  size_t                AddEntity(Entity *entity);
  Entity *              GetEntity(size_t id);
  void                  RemoveEntity(size_t entity);
  
  size_t                AddPlayer(Player *entity);
  Player &              GetPlayer()                       { return *this->player; }
  
  bool                  CheckEntities(const IVector3 &pos);
  
  std::vector<size_t>   FindEntities(const AABB &aabb)      const;
  std::vector<size_t>   FindSolidEntities(const AABB &aabb) const;

  // misc.
  Vector3               MoveAABB(const AABB &aabb, const Vector3 &dir, uint8_t &axis);
  
  void                  Explosion(Entity &entity, const Vector3 &pos, size_t radius, float strength, float damage, Element element);
  
private:

  int    level;
  World *world;
  
  size_t nextEntityId;
  size_t GetNextEntityId() { return nextEntityId++; }
  
  std::unordered_map<size_t, Entity*> entities;
  Player *player;

  bool showInventory;
  std::shared_ptr<InventoryGui> inventoryGui;
  
  std::vector<const Entity*> FindLightEntities(const Vector3 &pos, float radius) const;
  void BuildWorld();
};

#endif

