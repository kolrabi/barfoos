#include "common.h"

#include "worldbuilder.h"
#include "world.h"
#include "worldedit.h"
#include "cell.h"
#include "simplex.h"

#include "random.h"
#include "feature.h"
#include "item.h"
#include "itementity.h"

#include "runningstate.h"

#include <algorithm>

WorldBuilder::WorldBuilder(World &world) :
  world(world),
  minY(0), maxY(0),
  lastDir(0),
  loop(0)
{
}

WorldBuilder::~WorldBuilder() {
}

void
WorldBuilder::Build(RunningState &state, const Theme &theme) {

  // build features -------------------------------------------

  Random &random = state.GetRandom();

  this->MakeGround(random);

  bool done = false;
  for (size_t tries=0; tries < 20 && !done; tries++) {
    this->FillGround();

    this->instances.clear();
    this->instances.push_back(getFeature(theme.firstFeature)->BuildFeature(state, this->world, theme.firstFeaturePos, 0, 0, 0, nullptr, 0));
    this->instances.back().prevID = InvalidID;

    Log("Building features...\n");
    this->lastDir = 0;
    this->minY = this->world.GetSize().y;
    this->maxY = 0;

    this->loop = 0;
    do {
      this->BuildFeature(state, random, theme);
    } while(instances.size() < theme.featureCount && this->loop++ < 10000);

    int height = maxY - minY;

    // want at least 150 features and at least 16 cells high
    if (instances.size() < theme.minFeatures || height < 16) {
      done = false;
      Log("Discarding boring world with %u features and height %d :( ...\n", instances.size(), height);
    } else {
      done = true;
      Log("Made a nice world with %u features and height %d :) ...\n", instances.size(), height);
    }
  }

  this->MakeCaves(random, theme);
  this->PlaceTeleports(random, theme);
  this->PlaceTraps(state, random, theme);

  Log("Spawning feature entities...\n");
  for (auto instance : instances) {
    instance.feature->SpawnEntities(state, instance.pos);
  }

  Log("Placing %u items...\n", theme.itemCount);
  for (size_t i=0; i<theme.itemCount; i++) {
    std::string itemName = getRandomItem("item", state.GetLevel(), state.GetRandom());
    ItemEntity *entity = new ItemEntity(itemName);
    if (!entity) continue;

    IVector3 a = this->world.GetRandomTeleportTarget(random, entity->GetAABB().extents);
    entity->SetPosition(Vector3(a.x + 0.5, a.y + entity->GetAABB().extents.y+0.01, a.z + 0.5));
    state.AddEntity(entity);
  }

  Log("Placing some decoration (%u of them)...\n", theme.decoCount);
  for (size_t i=0; i<theme.decoCount; i++) {
    bool top = random.Coin();
    IVector3 a;
    if (top) {
      a = this->world.GetRandomTeleportTarget(random)[Side::Up];
    } else {
      a = this->world.GetRandomCeiling(random)[Side::Down];
    }
    Cell &cell = this->world.GetCell(a);
    ID decoID = cell.GetFeatureID();
    if (decoID == InvalidID) continue;

    std::string decoGroup = "deco." + instances[decoID].feature->GetDecoGroup() + ".";
    decoGroup += top?"top":"bottom";

    const std::vector<std::string> &decoEnts = GetEntitiesInGroup(decoGroup);
    if (decoEnts.empty()) continue;

    std::string decoName = decoEnts[random.Integer(decoEnts.size())];

    Entity *entity = Entity::Create(decoName);
    if (!entity) continue;

    state.AddEntity(entity);

    Vector3 pos(Vector3(a.x + 0.5, a.y, a.z + 0.5));
    if (top) {
      pos.y = this->world.CastRayYDown(pos) + entity->GetAABB().extents.y + 0.01;
    } else {
      pos.y = this->world.CastRayYUp(pos) - entity->GetAABB().extents.y - 0.01;
    }
    entity->SetPosition(pos);
  }

  Log("Placing %u enemies...\n", theme.monsterCount);
  weighted_map<std::string> monsters;
  for (auto &m:GetEntitiesInGroup("monster")) {
    monsters[m] = GetEntityProbability(m, state.GetLevel());
  }
  for (size_t i=0; i<theme.monsterCount; i++) {
    std::string monster = monsters.select(random.Float01());
    Entity *entity = Entity::Create(monster);
    if (!entity) continue;

    IVector3 a = this->world.GetRandomTeleportTarget(random, entity->GetAABB().extents);

    state.AddEntity(entity);
    entity->SetPosition(Vector3(a.x + 0.5, a.y + entity->GetAABB().extents.y+1.01, a.z + 0.5));
  }

  // remove spilt liquids
  Log("Wiping the floor...\n");
  bool foundLiquid = true;
  while(foundLiquid) {
    foundLiquid = false;
    for (size_t i=0; i<this->world.GetCellCount(); i++) {
      Cell &cell = this->world.GetCell(this->world.GetCellPos(i));
      if (!cell.IsLiquid()) continue;

      if (cell[Side::Down].GetType()     == "air" ||
          cell[Side::Right].GetType()    == "air" ||
          cell[Side::Left].GetType()     == "air" ||
          cell[Side::Forward].GetType()  == "air" ||
          cell[Side::Backward].GetType() == "air") {
        foundLiquid = true;
        this->world.SetCell(this->world.GetCellPos(i), Cell("air"));
      }
    }
  }

  // update all cells
  Log("Updating all cells...\n");
  for (size_t i=0; i<this->world.GetCellCount(); i++) {
    this->world.UpdateCell(this->world.GetCellPos(i));
  }

  this->world.Update(state);
  this->world.Update(state);

  Log("Done!\n");
}

void
WorldBuilder::MakeGround(Random &random) {
  Log("Creating ground...\n");

  IVector3 r(random.Integer(), random.Integer(), random.Integer());

  size_t cellCount = this->world.GetCellCount();
  defaultCells = std::vector<Cell>(cellCount);

  const IVector3 &size = this->world.GetSize();
  for (size_t i=0; i<cellCount; i++) {
    IVector3 pos = this->world.GetCellPos(i);
    if (pos.x < 2 || pos.y < 2 || pos.z < 2 || pos.x >= size.x-2 || pos.y >= size.y-2 || pos.z >= size.z-2) {
      defaultCells[i] = Cell("bedrock");
      defaultCells[i].SetProtected(true);
      defaultCells[i].SetIgnoringWrite(true);
    } else {
      /*
      defaultCells[i] = Cell("rock");

      IVector3 ppp(pos+r);
      ppp.x %= 256;
      ppp.y %= 256;
      ppp.z %= 256;
      Vector3 vpos(ppp);
      float f = (simplexNoise(vpos*0.07) * simplexNoise(vpos*(-0.06)) * simplexNoise(vpos*(-0.13)));
      if (f > 0.75) {
        defaultCells[i] = Cell("brick");
      } else if (f > 0.5) {
        defaultCells[i] = Cell("rock");
      } else {
        defaultCells[i] = Cell("dirt");
      }*/
    }
  }
}

void
WorldBuilder::FillGround() {
  Log("Filling ground...\n");
  this->world.CopyCellsFrom(this->defaultCells);
}

void
WorldBuilder::BuildFeature(RunningState &state, Random &random, const Theme &theme) {
  // select a feature from which to go
  bool                      useLast     = random.Chance(theme.useLastChance);
  bool                      useLastDir  = lastDir != 0 && useLast && random.Chance(theme.useLastDirChance);
  ID                        featNum     = useLast ? instances.size()-1 : random.Integer(instances.size());
  const FeatureInstance &   instance    = instances[featNum];

  // select a random connection from the current feature
  const Feature *           feature     = instance.feature;
  const FeatureConnection * conn        = useLastDir ? feature->GetRandomConnection(lastDir, state) : feature->GetRandomConnection(state);
  if (!conn) return;

  // select next feature
  const Feature *           nextFeature = conn->GetRandomFeature(state, instance.pos);
  if (!nextFeature) return;

  // make sure both features can connect
  const FeatureConnection * revConn     = nextFeature->GetRandomConnection(-conn->dir, state);
  if (!revConn) return;

  // snap both connection points together
  IVector3                  pos         = instance.pos + conn->pos - revConn->pos + IVector3(SideFromDir(conn->dir));

  // check if feature can be built
  this->world.BeginCheckOverwrite();
  nextFeature->BuildFeature(state, this->world, pos, conn->dir, instance.dist, instances.size(), nullptr, featNum);
  if (!this->world.FinishCheckOverwrite()) return;

  // build it
  FeatureInstance           nextInstance = nextFeature->BuildFeature(state, this->world, pos, conn->dir, instance.dist, instances.size(), revConn, featNum);
  this->minY = std::min(this->minY, pos.y);
  this->maxY = std::max(this->maxY, nextFeature->GetSize().y + pos.y);

  // replace some cells after connection if wanted
  feature->ReplaceChars(this->world, instance.pos, conn->id, featNum);

  lastDir = conn->dir;
  this->loop    = 0;

  // check if we accidentally connected properly to anything else
  for (auto &nextConn : nextInstance.feature->GetConnections()) {
    IVector3 nextConnPos = nextInstance.pos + nextConn.pos;

    for (auto &inst : instances) {
      for (const FeatureConnection &c : inst.feature->GetConnections()) {
        // match direction
        if (c.dir != -nextConn.dir) continue;

        // match position
        IVector3 connPos = inst.pos + c.pos;
        if (connPos != nextConnPos) continue;

        // replace some cells after connection if wanted
        inst.feature->ReplaceChars(this->world, inst.pos, c.id, inst.featureID);
        nextInstance.feature->ReplaceChars(this->world, nextInstance.pos, nextConn.id, nextInstance.featureID);
      }
    }
  }

  // done :)
  instances.push_back(nextInstance);
  instances.back().prevID = featNum;
}

void
WorldBuilder::MakeCaves(Random &random, const Theme &theme) {
  WorldEdit e(&this->world);

  Log("Carving caves...\n");

  // create caves
  e.SetBrush(Cell("air"));
  size_t caveCount     = instances.size()>10 ? random.Integer(instances.size()/10) : 0;

  for (size_t i = 0; i<caveCount; i++) {
    size_t featNum = random.Integer(instances.size());
    const FeatureInstance &instance = instances[featNum];
    IVector3 size = instance.feature->GetSize();

    for (size_t k=0; k<theme.caveRepeat; k++) {
      IVector3 cavePos(instance.pos + IVector3(random.Integer(size.x), random.Integer(size.y), random.Integer(size.z)));
      size_t caveLength = theme.caveLengthMin + random.Integer(theme.caveLengthMax-theme.caveLengthMin);
      bool lastSolid = false;

      for (size_t j=0; j<caveLength; j++) {
        Side nextSide = (Side)(random.Integer(6));
        bool solid = this->world.GetCell(cavePos).GetInfo().flags & CellFlags::Solid;
        if (solid && !lastSolid) {
          e.ApplyBrush(cavePos);
        }
        lastSolid = solid;
        cavePos = cavePos[nextSide];
      }
    }
  }
}

void
WorldBuilder::PlaceTeleports(Random &random, const Theme &theme) {
  Log("Placing %u teleports...\n", theme.teleportCount);
  for (size_t i=0; i<theme.teleportCount; i++) {
    size_t a = world.GetCellIndex(world.GetRandomTeleportTarget(random));
    size_t b = world.GetCellIndex(world.GetRandomTeleportTarget(random));
    while (a == b) b = world.GetCellIndex(world.GetRandomTeleportTarget(random));

    world.SetCell(world.GetCellPos(a), Cell("teleport")).SetTeleportTarget(b);
    world.SetCell(world.GetCellPos(b), Cell("teleport")).SetTeleportTarget(a);
  }
}

void
WorldBuilder::PlaceTraps(RunningState &state, Random &random, const Theme &theme) {
  Log("Placing %u traps...\n", theme.trapCount);
  for (size_t i=0; i<theme.trapCount; i++) {

    size_t a = world.GetCellIndex(world.GetRandomTeleportTarget(random));

    Cell *aboveCell = &world.GetCell(a)[Side::Up][Side::Up];
    Side side = (Side)random.Integer(6);
    while (side == Side::Down) side = (Side)random.Integer(6);

    size_t distance = 0;
    while(!aboveCell->IsSolid()) {
      aboveCell = &((*aboveCell)[side]);
      distance ++;
    }

    if (distance > 5) { i--; continue; }

    Cell &trigger = world.GetCell(a);
    Cell &spawner = world.SetCell(aboveCell->GetPosition(), Cell("shooter"));

    const std::vector<std::string> &traps = GetEntitiesInGroup("trap");
    if (traps.empty()) {
      spawner.SetSpawnOnActive("projectile.bfw9k", -side, 0.0);
    } else {
      spawner.SetSpawnOnActive(traps[random.Integer(traps.size())], -side, 0.0);
    }

    ID id = state.GetNextTriggerId();
    spawner.SetTrigger(id, false);
    trigger.SetTriggerTarget(id);
  }
}
