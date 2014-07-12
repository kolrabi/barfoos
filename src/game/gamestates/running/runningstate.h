#ifndef BARFOOS_RUNNINGSTATE_H
#define BARFOOS_RUNNINGSTATE_H

#include "game/gamestates/gamestate.h"

#include "runningstate.pb.h"

#include <vector>
#include <unordered_map>

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
  int                   GetLevel()                  const { return this->proto.level();   }
  bool 	                IsShowingInventory()        const { return this->showInventory; }

  // entity management
  ID                    AddEntity(Entity *entity);
  Entity *              GetEntity(ID id);
  void                  RemoveEntity(ID entity);

  Player &              GetPlayer()                       { return *this->player; }

  bool                  CheckEntities(const IVector3 &pos);

  std::vector<ID>       FindEntities(const Vector3 &center, float radius)      const;
  std::vector<ID>       FindSolidEntities(const AABB &aabb) const;

  // misc.
  Vector3               MoveAABB(const AABB &aabb, const Vector3 &dir, uint8_t &axis);

  void                  Explosion(Entity &entity, const Vector3 &pos, size_t radius, float strength, float damage, Element element, bool magical = false);
  ID                    SpawnInAABB(const std::string &type, const AABB &aabb, const Vector3 &velocity);
  void                  LockCell(Cell &cell);
  void                  LockEntity(Entity &entity);

  void                  TriggerOn(ID id);
  void                  TriggerOff(ID id);
  ID                    GetNextTriggerId();

  void                  SaveLevel();
  void                  LoadLevel();

private:

  RunningState_Proto    proto;

  World *world;

  ID GetNextEntityId();

  std::unordered_map<ID, Entity*> entities;
  Player *player;

  std::vector<Entity*> solidEntities;

  bool showInventory;

  float lastSaveT;

 std::vector<const Entity*> FindLightEntities(const Vector3 &pos, float radius) const;
  void BuildWorld();

  volatile bool saving;
  void Save();
  void Load();

  friend Serializer &operator << (Serializer &ser, const RunningState &state);
  friend Deserializer &operator >> (Deserializer &deser, RunningState &state);
};

#endif

