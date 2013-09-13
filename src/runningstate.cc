#include "common.h"

#include "runningstate.h"

#include "world.h"
#include "worldbuilder.h"
#include "ivector3.h"
#include "world.h"
#include "player.h"
#include "worldedit.h"
#include "feature.h"
#include "inventorygui.h"
#include "gfx.h"
#include "gfxview.h"

#include "audio.h"
#include "input.h"

#include "item.h"
#include "itementity.h"

#include "fileio.h"

#include "serializer.h"
#include "deserializer.h"

#include <algorithm>
#include <sys/time.h>
#include <fstream>

RunningState::RunningState(Game &game) :
  GameState(game),
  world(nullptr),
  player(nullptr),
  showInventory(false),
  lastSaveT(0.0),
  saving(false)
{
  Log("+RunningState() %p %p %p %p\n", this, &GetRandom(), &game, &GetGame());
}

RunningState::~RunningState() {
  for (auto entity : this->entities) {
    delete entity.second;
  }
  this->entities.clear();

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
  this->proto.set_level(0);
  this->proto.set_next_entity_id(1);
  this->proto.set_next_lock_id(1);
  this->proto.set_next_trigger_id(1);

  std::string seed = ToString(time(nullptr));
  GetGame().NewGame(seed);

  delete this->world;
  this->world = new World(*this, IVector3(64, 64, 64));

  Random &random = GetRandom();

  Theme theme;
  theme.featureCount  = random.Integer(400)+400;             // 400 - 800
  theme.useLastChance = 0.1 + random.Float01()*0.8;          // 0.1 - 0.9
  theme.useLastDirChance = 0.6;
  theme.caveLengthMin = random.Integer(20);
  theme.caveLengthMax = theme.caveLengthMin + random.Integer(100);
  theme.caveRepeat    = random.Integer(20)+10;

  theme.teleportCount = 50; //random.Integer(10)+2;
  theme.trapCount = 50; //random.Integer(10)+10;
  theme.decoCount = 50; //500+random.Integer(200);
  theme.itemCount = 50; //100+random.Integer(120);
  theme.monsterCount = 0; //50+random.Integer(100);

  WorldBuilder builder(*this->world);
  builder.Build(*this, theme);

  Log("adding player\n");
  Entity *player = Entity::Create("player");
  if (player) {
    player->SetPosition(IVector3(32,32,32));
    player->SetSpawnPosition(IVector3(32,32,32));
    this->AddEntity(player);
  }

  Log("adding box\n");
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
  gfx.GetScreen().Viewport(Rect());

  // draw world first
  gfx.SetFog(0.05, IColor(64,64,64));
  gfx.SetLights(lightPositions, lightColors);

  player->View(gfx);
  world->Draw(gfx);

  // draw all entities
  for (auto entity : this->entities) {
    if (entity.second) entity.second->Draw(gfx);
  }

  gfx.ClearDepth(1.0);
  player->DrawWeapons(gfx);

  Point vscreen = gfx.GetScreen().GetSize();
  gfx.GetScreen().Viewport(Rect(Point(vscreen.x-128, 0), Point(128, 128)));
  player->MapView(gfx);
  world->GetMap().Draw(gfx, player->GetSmoothPosition(), player->GetYaw() * Const::rad2deg);
  gfx.GetScreen().Viewport(Rect());

  // next draw gui stuff
  gfx.GetView().GUI();

  player->DrawGUI(gfx);
}

GameState *
RunningState::Update() {
  PROFILE();

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

  // show or hide inventory
  if (GetGame().GetInput().IsKeyDown(InputKey::Inventory)) {
    if (!this->showInventory) {
      ID entityID;
      Side cellSide;
      // TODO: get range
      this->player->GetSelection(*this, 5.0, nullptr, cellSide, entityID);

      Entity *entity = this->GetEntity(entityID);

      if (!entity || !entity->GetProperties()->openInventory) {
        GetGame().SetGui(std::shared_ptr<Gui>(new InventoryGui(*this, *player)));
        this->showInventory = true;
      } else {
        if (entities[entityID]->GetLockedID()) {
          this->player->AddMessage("The "+entity->GetName()+" is locked.");
        } else {
          GetGame().SetGui(std::shared_ptr<Gui>(new InventoryGui(*this, *player, *entity)));
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
      if (!entity.second) continue;
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
    for (auto &entity : this->entities) {
      if (entity.second) {
        entity.second->Update(*this);
      } else {
        Log("Entity %u is null!\n", entity.first);
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
  ID entityId = GetNextEntityId();
  this->entities[entityId] = entity;
  entity->Start(*this, entityId);

  if (entity->IsSolid()) this->solidEntities.push_back(entity);

  if (dynamic_cast<Player*>(entity)) {
    this->player = dynamic_cast<Player*>(entity);
    GetGame().GetGfx().SetPlayer(player);
    GetGame().GetAudio().SetPlayer(player);
    this->proto.set_player_id(entityId);
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
    GetGame().GetAudio().SetPlayer(nullptr);
    this->proto.set_player_id(0);
  }

  auto solidIter = std::find(this->solidEntities.begin(), this->solidEntities.end(), iter->second);
  if (solidIter != this->solidEntities.end()) {
    this->solidEntities.erase(solidIter);
  }

  delete iter->second;
  this->entities.erase(iter);
}

std::vector<ID>
RunningState::FindEntities(const Vector3 &center, float radius) const {
  std::vector<ID> entities;

  for (auto entity : this->entities) {
    if (!entity.second || entity.second->IsDead()) continue;
    float d = (center - entity.second->GetPosition()).GetMag();
    if (d < radius) {
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
RunningState::Explosion(Entity &entity, const Vector3 &pos, size_t radius, float strength, float damage, Element element, bool magical) {
  ID ownerID = entity.GetOwner();
  if (ownerID == InvalidID) ownerID = entity.GetId();

  Entity *owner = this->GetEntity(ownerID);
  float damageFactor = 1.0;

  if (owner) {
    Stats stats = owner->GetEffectiveStats();
    damageFactor += 0.2 * (stats.GetMagicAttack() + stats.GetSkill("magic")); //Stats::GetLevelForSkillExp(stats.skills["magic"]));
  }

  std::vector<ID> entIDs = this->FindEntities(pos, radius);
  for (ID entID : entIDs) {
    Entity &ent  = *this->entities[entID];
    Vector3 d = ent.GetPosition() - pos;
    float dmg = damage / (1.0 + d.GetSquareMag());

    HealthInfo info = magical ? Stats::ExplosionAttack(ownerID, ent, dmg * damageFactor, element) : Stats::ExplosionAttack(ownerID, ent, dmg, element);
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
  if (cell.GetLockID()) return;

  ID id = this->proto.next_lock_id();
  this->proto.set_next_lock_id(id + 1);
  cell.Lock(id);

  if (GetRandom().Chance(0.8)) {
    size_t keyIndex = this->GetWorld().GetCellIndex(this->GetWorld().GetRandomTeleportTarget(this->GetRandom()));
    //while (this->GetWorld().GetCell(keyPos).GetFeatureID() >= maxFeatureID - 1) {
    //  keyPos = this->GetWorld().GetRandomTeleportTarget(this->GetRandom())[Side::Up];
    //}

    std::shared_ptr<Item> keyItem(new Item("key"));
    keyItem->SetUnlockID(id);

    ItemEntity *entity = new ItemEntity(keyItem);
    entity->SetPosition(this->GetWorld().GetCell(keyIndex)[Side::Up].GetAABB().center);
    entity->AddVelocity(Vector3(0,10,0));

    AddEntity(entity);
  }
}

void RunningState::LockEntity(Entity &ent) {
  if (ent.GetLockedID()) return;

  ID id = this->proto.next_lock_id();
  this->proto.set_next_lock_id(id + 1);
  ent.Lock(id);

  if (GetRandom().Chance(0.8)) {
    size_t keyIndex = this->GetWorld().GetCellIndex(this->GetWorld().GetRandomTeleportTarget(this->GetRandom()));
    //while (this->GetWorld().GetCell(keyPos).GetFeatureID() >= maxFeatureID - 1) {
    //  keyPos = this->GetWorld().GetRandomTeleportTarget(this->GetRandom())[Side::Up];
    //}

    std::shared_ptr<Item> keyItem(new Item("key"));
    keyItem->SetUnlockID(id);

    ItemEntity *entity = new ItemEntity(keyItem);
    entity->SetPosition(this->GetWorld().GetCell(keyIndex)[Side::Up].GetAABB().center);
    entity->AddVelocity(Vector3(0,10,0));

    AddEntity(entity);
  }
}

void RunningState::TriggerOn(ID id) {
  for (auto &entity : this->entities) {
    if (entity.second && entity.second->GetTriggerId() == id) entity.second->TriggerOn();
  }
  this->GetWorld().TriggerOn(id);
}

void RunningState::TriggerOff(ID id) {
  for (auto &entity : this->entities) {
    if (entity.second && entity.second->GetTriggerId() == id) entity.second->TriggerOff();
  }
  this->GetWorld().TriggerOff(id);
}

ID
RunningState::GetNextTriggerId() { 
  ID id = this->proto.next_trigger_id();
  this->proto.set_next_trigger_id(id + 1); 
  return id;
}

ID
RunningState::GetNextEntityId() { 
  ID id = this->proto.next_entity_id();
  this->proto.set_next_entity_id(id + 1); 
  return id;
}

void
RunningState::Save() {
  PROFILE();
  Log("Saving...\n");
  this->saving = true;

  std::fstream out;
  if (createUserStream("save.game", out)) {
    this->GetGame().Serialize(out);
    this->proto.SerializeToOstream(&out);
  } else {
    Log("could not open save.game file\n");
  }

  SaveLevel();

  this->saving = false;
}

void
RunningState::SaveLevel() {
  std::fstream out;
  if (createUserStream("save.level."+ToString(this->GetLevel()), out)) {
    ::EntityList_Proto entityList;
    for (auto &e:this->entities) {
      if (e.second) *entityList.add_entities() = e.second->GetProto();
    }
    entityList.SerializeToOstream(&out);
  } else {
    Log("could not open level save file\n");
  }

  world->GetProto().SerializeToOstream(&out);

  // TODO: serialize nondefault cells

  /*
  for (size_t i=0; i<world->GetCellCount(); i++) {
    Cell_Proto cellProto;
    world->GetCell(i).Serialize(cellProto);
    cellProto.SerializeToOstream(&out);
  }
  */

  /*
  Serializer ser("LEVL");
  ser << *world;
  ser << (uint32_t)this->player->GetId();
  ser << this->entities;

  FILE *f = createUserFile("level." + ToString(this->proto.level()));

  //std::thread([=](){
    if (f) {
  //    ser.WriteToFile(f);
      fclose(f);
    }
  //}).detach();
  */
}

void
RunningState::LoadLevel() {
  /*
  FILE *f = openUserFile("level." + ToString(this->proto.level()));

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
  }*/
}

void
RunningState::Load() {
  PROFILE();
  /*
  Log("Loading...\n");

  Deserializer deser;
  FILE *f = openUserFile("game");
  deser.LoadFromFile(f, "GAME");

  Log("game %08x\n", deser.GetPos());
  //deser >> GetGame();

  Log("runningstate %08x\n", deser.GetPos());
  //deser >> self;

  LoadLevel();
  */
}
