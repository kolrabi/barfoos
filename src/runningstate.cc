#include "runningstate.h"
#include "world.h"
#include "ivector3.h"
#include "world.h"
#include "player.h"
#include "worldedit.h"
#include "feature.h"
#include "inventorygui.h"
#include "gfx.h"
#include "gfxview.h"
#include "input.h"

#include "item.h"
#include "itementity.h"

#include "serializer.h"
#include "deserializer.h"

#include <algorithm>
#include <sys/time.h>

RunningState::RunningState(Game &game) :
  GameState(game),
  level(0),
  world(nullptr),
  nextEntityId(1),
  player(nullptr),
  showInventory(false),
  lastSaveT(0.0),
  nextTriggerId(1),
  nextLockId(1),
  saving(false)
{
  Log("+RunningState() %p %p %p %p\n", this, &GetRandom(), &game, &GetGame());
}

RunningState::~RunningState() {
  for (auto entity : this->entities) {
    delete entity.second;
  }

  Log("-RunningState()\n");
}

void
RunningState::Enter() {
}

void
RunningState::Leave(GameState *) {
  this->Save();
}

void
RunningState::NewGame() {
  this->level = 0;

  std::string seed = ToString(time(nullptr));
  GetGame().NewGame(seed);

  delete this->world;
  this->world = new World(*this, IVector3(64, 64, 64));
  this->world->Build();

  Entity *player = Entity::Create("player");
  if (player) {
    player->SetPosition(IVector3(32,32,32));
    player->SetSpawnPos(IVector3(32,32,32));
    this->AddEntity(player);
  }

  Entity *entity = Entity::Create("box");
  entity->SetPosition(IVector3(32,24,32));
  this->AddEntity(entity);
}

void
RunningState::ContinueGame() {
  Load();
}

void
RunningState::Render(Gfx &gfx) const {
  PROFILE();

  if (!player) return;

  std::vector<IColor> lightColors;
  std::vector<Vector3> lightPositions;
  std::vector<const Entity*> lightEntities = FindLightEntities(player->GetSmoothPosition(), 32);
  for (auto e : lightEntities) {
    lightColors.push_back(e->GetLight());
    lightPositions.push_back(e->GetSmoothEyePosition());
  }

  gfx.ClearColor(IColor(30, 30, 20));
  gfx.ClearDepth(1.0);
  gfx.Viewport(Rect());

  // draw world first
  gfx.SetFog(0.05, IColor(64,64,64));
  gfx.SetLights(lightPositions, lightColors);

  player->View(gfx);
  world->Draw(gfx);

  // draw all entities
  for (auto entity : this->entities) {
    entity.second->Draw(gfx);
  }

  gfx.ClearDepth(1.0);
  player->DrawWeapons(gfx);

  Point vscreen = gfx.GetScreenSize();
  gfx.Viewport(Rect(Point(vscreen.x-128, 0), Point(128, 128)));
  player->MapView(gfx);
  world->GetMap().Draw(gfx, player->GetSmoothPosition(), player->GetAngles().x * Const::rad2deg);
  gfx.Viewport(Rect());

  // next draw gui stuff
  gfx.GetView().GUI();

  player->DrawGUI(gfx);
}

GameState *
RunningState::Update() {
  PROFILE();

  // show or hide inventory
  if (GetGame().GetInput().IsKeyDown(InputKey::Inventory)) {
    if (!this->showInventory) {
      ID entityID;
      Side cellSide;
      this->player->GetSelection(*this, nullptr, cellSide, entityID);

      if (entityID == InvalidID || !entities[entityID]->GetProperties()->openInventory) {
        GetGame().SetGui(std::shared_ptr<Gui>(new InventoryGui(*this, *player)));
        this->showInventory = true;
      } else {
        if (entities[entityID]->GetLockedID()) {
          this->player->AddMessage("The "+entities[entityID]->GetName()+" is locked.");
        } else {
          GetGame().SetGui(std::shared_ptr<Gui>(new InventoryGui(*this, *player, *entities[entityID])));
          this->showInventory = true;
        }
      }
    }
  } else if (GetGame().GetInput().IsKeyUp(InputKey::Inventory)) {
    if (this->showInventory) {
      GetGame().SetGui(nullptr);
    }
    this->showInventory = false;
  }

  // update world
  this->world->Update(*this);

  // handle collision between entities
  {
    PROFILE_NAMED("Entity Collision");
    std::vector<Entity*> collideEntities;
    for (auto entity : this->entities) {
      if (entity.second->GetProperties()->nocollideEntity) continue;
      if (entity.second->IsDead()) continue;
      collideEntities.push_back(entity.second);
    }
    for (size_t i=0; i<collideEntities.size(); i++) {
      Entity *e1 = collideEntities[i];
      for (size_t j=i+1; j<collideEntities.size(); j++) {
        Entity *e2 = collideEntities[j];

        // don't collide with owners if not wanted
        if (e1->GetOwner() == e2->GetId() && e1->GetProperties()->nocollideOwner) continue;
        if (e2->GetOwner() == e1->GetId() && e2->GetProperties()->nocollideOwner) continue;

        if (e1->GetAABB().Overlap(e2->GetAABB())) {
          e1->OnCollide(*this, *e2);
          e2->OnCollide(*this, *e1);
        }
      }
    }
  }

  // update all entities
  {
    PROFILE_NAMED("Entity Update");
    for (auto entity : this->entities) {
      entity.second->Update(*this);
    }
  }

  // remove removable entities
  {
    PROFILE_NAMED("Remove Entities");
    auto entityIter = this->entities.begin();
    while(entityIter != this->entities.end()) {
      if (!entityIter->second || entityIter->second->IsRemovable()) {
        delete entityIter->second;
        entityIter = this->entities.erase(entityIter);
      } else {
        entityIter++;
      }
    }
  }

  return this;
}

void
RunningState::BuildWorld() {
}

void RunningState::HandleEvent(const InputEvent &event) {
  if (this->player) this->player->HandleEvent(event);
}

/**
 * Add an entity to this game.
 * @param entity Entity to add
 */
ID
RunningState::AddEntity(Entity *entity) {
  size_t entityId = GetNextEntityId();
  this->entities[entityId] = entity;
  entity->Start(*this, entityId);

  if (entity->IsSolid()) this->solidEntities.push_back(entity);

  if (dynamic_cast<Player*>(entity)) {
    this->player = dynamic_cast<Player*>(entity);
    GetGame().GetGfx().SetPlayer(player);
  }
  return entityId;
}

/**
 * Remove an entity from this world.
 * If the entity is the player entity, the player reference will be unset.
 * @param entity Entity to remove
 */
void
RunningState::RemoveEntity(ID entityId) {
  auto iter = this->entities.find(entityId);
  if (iter == this->entities.end()) {
    return;
  }

  if (this->player == iter->second) {
    this->player = nullptr;
    GetGame().GetGfx().SetPlayer(nullptr);
  }

  auto solidIter = std::find(this->solidEntities.begin(), this->solidEntities.end(), iter->second);
  if (solidIter != this->solidEntities.end()) {
    this->solidEntities.erase(solidIter);
  }

  delete iter->second;
  this->entities.erase(iter);
}


/**
 * Find entities within an AABB.
 * The entities' AABBs have to intersesct the given AABB to be returned.
 * @param aabb AABB within which to look for entities
 * @return A vector of entities.
 */
std::vector<ID>
RunningState::FindEntities(const AABB &aabb) const {
  std::vector<ID> entities;

  for (auto entity : this->entities) {
    if (aabb.Overlap(entity.second->GetAABB())) {
      entities.push_back(entity.first);
    }
  }

  return entities;
}

std::vector<ID>
RunningState::FindSolidEntities(const AABB &aabb) const {
  std::vector<ID> entities;

  for (auto ent : this->solidEntities) {
    if (aabb.Overlap(ent->GetAABB())) {
      entities.push_back(ent->GetId());
    }
  }

  return entities;
}

std::vector<const Entity*>
RunningState::FindLightEntities(const Vector3 &pos, float radius) const {
  std::vector<const Entity*> entities;

  for (auto &entity : this->entities) {
    if (entity.second && !entity.second->GetLight().IsBlack() && (entity.second->GetPosition()-pos).GetMag() < radius) {
      entities.push_back(entity.second);
    }
  }

  std::sort(entities.begin(), entities.end(), [&](const Entity *a, const Entity *b) -> bool {
    float d1 = (a->GetPosition()-pos).GetSquareMag();
    float d2 = (b->GetPosition()-pos).GetSquareMag();
    if (d1 == d2) return false;
    return d1 < d2;
  });

  return entities;
}

/**
 * Check if a cell is free of entities.
 * @param pos Cell position to check
 * @return true if no entities are inside the cell.
 */
bool
RunningState::CheckEntities(const IVector3 &pos) {
  AABB aabb;
  aabb.extents = Vector3(0.5, 0.5, 0.5);
  aabb.center = Vector3(pos) + aabb.extents;

  for (auto entity : this->entities) {
    if (aabb.Overlap(entity.second->GetAABB())) return false;
  }

  return true;
}

Entity *
RunningState::GetEntity(ID entityId) {
  auto iter = this->entities.find(entityId);
  if (iter == this->entities.end()) return nullptr;
  return iter->second;
}

Vector3 RunningState::MoveAABB(
  const AABB &aabb,
  const Vector3 &targ,
  uint8_t &axis
) {
  axis = 0;

  Vector3 center = aabb.center;
  Vector3 dist = targ-aabb.center;

  // are we moving at all?
  if (dist.GetSquareMag() == 0.0f) return center;

  // collect all entities in range
  AABB targAABB(aabb);
  targAABB.center = targ;

  AABB bounds = aabb.Combine(targAABB).Grow(0.1);
  std::vector<ID> entities = FindSolidEntities(bounds);
  if (entities.empty()) return targ;

  // create vertices that serve as origins for ray check. must be at most
  // one cell size apart for correct collision detection.
  std::vector<Vector3> verts;
  aabb.GetVertices(verts);

  Vector3 dd( dist.x>0 ? 1.0 : -1.0, dist.y>0 ? 1.0 : -1.0, dist.z>0 ? 1.0 : -1.0 );

  // try to move along the x axis
  if (dist.x) {
      for (size_t eid : entities) {
        Entity *ent = GetEntity(eid);
        if (!ent) continue;

        const AABB &entAABB = ent->GetAABB();

        if (entAABB.Min().z > aabb.Max().z) continue;
        if (entAABB.Max().z < aabb.Min().z) continue;
        if (entAABB.Min().y > aabb.Max().y) continue;
        if (entAABB.Max().y < aabb.Min().y) continue;

        float t = (aabb.center.x + dd.x * aabb.extents.x) - (entAABB.center.x - dd.x * entAABB.extents.x);
        if (t * dd.x < std::abs(dist.x)) {
          dist.x = -t - dd.x * 0.001;
          axis |= Axis::X;
        }

      }
  }

  // update center to new position
  center.x += dist.x;

  // try to move along the z axis
  if (dist.z) {
    for (size_t eid : entities) {
      Entity *ent = GetEntity(eid);
      if (!ent) continue;

      const AABB &entAABB = ent->GetAABB();

      if (entAABB.Min().x > aabb.Max().x) continue;
      if (entAABB.Max().x < aabb.Min().x) continue;
      if (entAABB.Min().y > aabb.Max().y) continue;
      if (entAABB.Max().y < aabb.Min().y) continue;

      float t = (aabb.center.z + dd.z * aabb.extents.z) - (entAABB.center.z - dd.z * entAABB.extents.z);
      if (t * dd.z < std::abs(dist.z)) {
        dist.z = -t - dd.z * 0.001;
        axis |= Axis::Z;
      }
    }
  }

  // update center to new position
  center.z += dist.z;

  // try to move along the y axis
  if (dist.y) {
    for (size_t eid : entities) {
      Entity *ent = GetEntity(eid);
      if (!ent) continue;

      const AABB &entAABB = ent->GetAABB();

      if (entAABB.Min().x > aabb.Max().x) continue;
      if (entAABB.Max().x < aabb.Min().x) continue;
      if (entAABB.Min().z > aabb.Max().z) continue;
      if (entAABB.Max().z < aabb.Min().z) continue;

      float t = (aabb.center.y + dd.y * aabb.extents.y) - (entAABB.center.y - dd.y * entAABB.extents.y);
      if (t * dd.y < std::abs(dist.y)) {
        dist.y = -t - dd.y * 0.001;
        axis |= Axis::Y;
      }
    }
  }

  // update center to new position
  center.y += dist.y;

  return center;
}

void
RunningState::Explosion(Entity &entity, const Vector3 &pos, size_t radius, float strength, float damage, Element element) {
  AABB aabb(pos, radius);

  ID ownerID = entity.GetOwner();
  if (ownerID == InvalidID) ownerID = entity.GetId();
  Entity &owner = *this->entities[ownerID];

  std::vector<ID> entIDs = this->FindEntities(aabb);
  for (ID entID : entIDs) {
    Entity &ent  = *this->entities[entID];
    Vector3 d = ent.GetPosition() - pos;
    float dmg = damage / (1.0 + d.GetSquareMag());

    HealthInfo info = Stats::ExplosionAttack(owner, ent, dmg, element);
    ent.AddHealth(*this, info);

    try {
      dynamic_cast<Mob&>(ent).AddImpulse(d.Normalize() * dmg * 100);
    } catch(const std::bad_cast &) { }
  }

  // leave a crater
  IVector3 ivPos(pos);
  IVector3 ivRadius(radius, radius, radius);

  IVector3(radius*2, radius*2, radius*2).For( [&] (IVector3 p) {
    IVector3 worldCellPos    = p - ivRadius + ivPos;
    Vector3  worldCellCenter = Vector3(worldCellPos) + 0.5;
    Vector3  d = worldCellCenter - pos;
    float    dsqmag = d.GetSquareMag();

    float    chance = 1.0 / dsqmag * strength / this->world->GetCell(worldCellPos).GetInfo().breakStrength;
    if (chance > 0 && GetRandom().Chance(chance)) {
      this->GetWorld().BreakBlock(worldCellPos);
    }
  });
}

ID
RunningState::SpawnInAABB(
  const std::string &type,
  const AABB &aabb,
  const Vector3 &velocity
) {
  Entity *entity = Entity::Create(type);
  if (!entity) return InvalidID;

  Vector3 s = aabb.extents - entity->GetAABB().extents;
  Vector3 p = GetRandom().Vector() * s + aabb.center;
  entity->SetPosition(p);

  Mob *mob = dynamic_cast<Mob*>(entity);
  if (mob) mob->AddVelocity(velocity);

  return AddEntity(entity);
}

void RunningState::LockCell(Cell &cell) {
  if (cell.GetLockedID()) return;

  ID id = nextLockId ++;
  cell.Lock(id);

  if (GetRandom().Chance(0.8)) {
    IVector3 keyPos = this->GetWorld().GetRandomTeleportTarget(this->GetRandom())[Side::Up];
    //while (this->GetWorld().GetCell(keyPos).GetFeatureID() >= maxFeatureID - 1) {
    //  keyPos = this->GetWorld().GetRandomTeleportTarget(this->GetRandom())[Side::Up];
    //}

    std::shared_ptr<Item> keyItem(new Item("key"));
    keyItem->SetUnlockID(id);

    ItemEntity *entity = new ItemEntity(keyItem);
    entity->SetPosition(this->GetWorld().GetCell(keyPos).GetAABB().center);
    entity->AddVelocity(Vector3(0,10,0));

    AddEntity(entity);
  }
}

void RunningState::LockEntity(Entity &ent) {
  if (ent.GetLockedID()) return;

  ID id = nextLockId ++;
  ent.Lock(id);

  if (GetRandom().Chance(0.8)) {
    IVector3 keyPos = this->GetWorld().GetRandomTeleportTarget(this->GetRandom())[Side::Up];
    //while (this->GetWorld().GetCell(keyPos).GetFeatureID() >= maxFeatureID - 1) {
    //  keyPos = this->GetWorld().GetRandomTeleportTarget(this->GetRandom())[Side::Up];
    //}

    std::shared_ptr<Item> keyItem(new Item("key"));
    keyItem->SetUnlockID(id);

    ItemEntity *entity = new ItemEntity(keyItem);
    entity->SetPosition(this->GetWorld().GetCell(keyPos).GetAABB().center);
    entity->AddVelocity(Vector3(0,10,0));

    AddEntity(entity);
  }
}

void RunningState::TriggerOn(ID id) {
  for (auto &entity : this->entities) {
    if (entity.second->GetTriggerId() == id) entity.second->TriggerOn();
  }
  this->GetWorld().TriggerOn(id);
}

void RunningState::TriggerOff(ID id) {
  for (auto &entity : this->entities) {
    if (entity.second->GetTriggerId() == id) entity.second->TriggerOff();
  }
  this->GetWorld().TriggerOff(id);
}

void
RunningState::Save() {
  PROFILE();
  Log("Saving...\n");
  this->saving = true;

  Serializer serGame("GAME");
  serGame << GetGame();
  serGame << self;

  FILE *f = createUserFile("game");
  if (f) {
// TEST    serGame.WriteToFile(f);
    fclose(f);
    f = nullptr;
  }

  SaveLevel();

  this->saving = false;
}

void
RunningState::SaveLevel() {
  Serializer ser("LEVL");
  ser << *world;
  ser << (uint32_t)this->player->GetId();
  ser << this->entities;

  FILE *f = createUserFile("level." + ToString(level));

  //std::thread([=](){
    if (f) {
// TEST      ser.WriteToFile(f);
      fclose(f);
    }
  //}).detach();
}

void
RunningState::LoadLevel() {
  FILE *f = openUserFile("level." + ToString(level));

  Deserializer deser;
  deser.LoadFromFile(f, "LEVL");
  fclose(f);

  delete world;
  world = new World(*this, deser);

  ID playerId;
  deser >> playerId;
  deser >> this->entities;

  this->player = dynamic_cast<Player*>(this->entities[playerId]);
  GetGame().GetGfx().SetPlayer(this->player);

  for (auto &e:this->entities) {
    e.second->Continue(*this, e.first);
  }

}

Serializer &operator << (Serializer &ser, const RunningState &state) {
  ser << state.level;
  ser << state.nextEntityId;
  ser << state.nextTriggerId;
  ser << state.nextLockId;
  return ser;
}

void
RunningState::Load() {
  PROFILE();
  Log("Loading...\n");

  Deserializer deser;
  FILE *f = openUserFile("game");
  deser.LoadFromFile(f, "GAME");

  Log("game %08x\n", deser.GetPos());
  deser >> GetGame();

  Log("runningstate %08x\n", deser.GetPos());
  deser >> self;

  LoadLevel();
}

Deserializer &operator >> (Deserializer &deser, RunningState &state) {
  deser >> state.level;
  deser >> state.nextEntityId;
  deser >> state.nextTriggerId;
  deser >> state.nextLockId;
  return deser;
}
