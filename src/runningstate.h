#ifndef BARFOOS_RUNNINGSTATE_H
#define BARFOOS_RUNNINGSTATE_H

#include "game.h"

class RunningState : public GameState {
public:

                        RunningState(Game &);
  virtual               ~RunningState();

  virtual void          Enter ()                    override;
  virtual void          Leave (GameState *next)    override;
  virtual GameState *   Update()                    override;
  virtual void          Render(Gfx &gfx)            const override;
  virtual void          HandleEvent(const InputEvent &evt) override;

  void                  NewGame();
  void                  ContinueGame();

  World  &              GetWorld()                  const { return *this->world;  }
  int                   GetLevel()                  const { return this->level;   }
  bool 	                IsShowingInventory()        const { return this->showInventory; }

  // entity management
  ID                    AddEntity(Entity *entity);
  Entity *              GetEntity(ID id);
  void                  RemoveEntity(ID entity);

  Player &              GetPlayer()                       { return *this->player; }

  bool                  CheckEntities(const IVector3 &pos);

  std::vector<ID>       FindEntities(const AABB &aabb)      const;
  std::vector<ID>       FindEntities(const Vector3 &center, float radius)      const;
  std::vector<ID>       FindSolidEntities(const AABB &aabb) const;

  // misc.
  Vector3               MoveAABB(const AABB &aabb, const Vector3 &dir, uint8_t &axis);

  void                  Explosion(Entity &entity, const Vector3 &pos, size_t radius, float strength, float damage, Element element);
  ID                    SpawnInAABB(const std::string &type, const AABB &aabb, const Vector3 &velocity = Vector3());
  void                  LockCell(Cell &cell);
  void                  LockEntity(Entity &entity);

  void                  TriggerOn(ID id);
  void                  TriggerOff(ID id);
  ID                    GetNextTriggerId() { return this->nextTriggerId ++; }

  void                  SaveLevel();
  void                  LoadLevel();

private:

  int32_t level;
  World *world;

  ID nextEntityId;
  ID GetNextEntityId() { return nextEntityId++; }

  std::unordered_map<ID, Entity*> entities;
  Player *player;

  std::vector<Entity*> solidEntities;

  bool showInventory;

  float lastSaveT;
  ID nextTriggerId;

  ID nextLockId;

  std::vector<const Entity*> FindLightEntities(const Vector3 &pos, float radius) const;
  void BuildWorld();

  volatile bool saving;
  void Save();
  void Load();

  friend Serializer &operator << (Serializer &ser, const RunningState &state);
  friend Deserializer &operator >> (Deserializer &deser, RunningState &state);
};

#endif

