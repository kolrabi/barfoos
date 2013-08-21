#include "GLee.h"

#include "world.h"
#include "cell.h"
#include "simplex.h"
#include "entity.h"
#include "itementity.h"
#include "item.h"

#include "random.h"
#include "worldedit.h"
#include "feature.h"

#include "runningstate.h"

#include "gfx.h"
#include "gfxview.h"

#include "vertex.h"
#include "texture.h"
#include "aabb.h"

#include "serializer.h"
#include "deserializer.h"

World::World(RunningState &state, const IVector3 &size) :
  state(state),
  size(size),
  dirty(true),
  firstDirty(true),
  minimap(*this),
  cellCount(size.x * size.y * size.z),
  cells(cellCount, Cell("default")),
  defaultCell("default"),
  defaultMask(cellCount, true),
  dynamicCells(0),
  nextTickT(0.0),
  tickInterval(0.1),
  ambientLight(32, 32, 32),
  allVerts(0),
  vertexStartsNormal(),
  vertexCountsNormal(),
  vertexStartsEmissive(),
  vertexCountsEmissive(),
  vbo(0),
  checkOverwrite(false),
  checkOverwriteOK(true)
{
  glGenBuffers(1, &this->vbo);
}

World::World(RunningState &state, Deserializer &deser) :
  state(state),
  dirty(true),
  firstDirty(true),
  minimap(*this, deser),
  defaultCell("default"),
  dynamicCells(0),
  allVerts(0),
  vertexStartsNormal(),
  vertexCountsNormal(),
  vertexStartsEmissive(),
  vertexCountsEmissive(),
  vbo(0),
  checkOverwrite(false),
  checkOverwriteOK(true)
{
  deser >> this->size;
  cellCount = size.x * size.y * size.z;

  this->cells.resize(cellCount);
  for (size_t i = 0; i<cellCount; i++) {
    deser >> this->cells[i];
  }

  deser >> this->defaultMask;
  deser >> this->nextTickT;
  deser >> this->tickInterval;
  deser >> this->ambientLight;

  for (size_t i = 0; i<cellCount; i++) {
    this->cells[i].SetWorld(this, GetCellPos(i));
    this->MarkForUpdateNeighbours(i);
  }
  glGenBuffers(1, &this->vbo);
}

World::~World() {
  if (this->vbo) glDeleteBuffersARB(1, &this->vbo);
}

void
World::Build() {

  // build features -------------------------------------------

  // some basic parameters for this world
  Log("%p %p %p\n", &state, &state.GetGame(), &state.GetRandom());
  Random &random = state.GetRandom();
  IVector3 r(random.Integer(), random.Integer(), random.Integer());
  size_t featureCount  = random.Integer(400)+400;             // 400 - 800
  float  useLastChance = 0.1 + random.Float01()*0.8;          // 0.1 - 0.9
  float  useLastDirChance = 0.6;
  size_t caveLengthMin = random.Integer(20);
  size_t caveLengthMax = caveLengthMin + random.Integer(100);
  size_t caveRepeat    = random.Integer(20)+10;

  size_t teleportCount = random.Integer(10)+2;
  size_t trapCount = random.Integer(10)+10;
  size_t decoCount = 500+random.Integer(200);
  size_t itemCount = 100+random.Integer(120);
  size_t monsterCount = 30+random.Integer(100);

  std::vector<FeatureInstance> instances;

  Log("Creating ground...\n");
  std::vector<Cell> defaultCells(this->cellCount);
  for (size_t i=0; i<this->cellCount; i++) {
    IVector3 pos = GetCellPos(i);
    if (pos.x < 2 || pos.y < 2 || pos.z < 2 || pos.x >= size.x-2 || pos.y >= size.y-2 || pos.z >= size.z-2) {
      defaultCells[i] = Cell("bedrock");
      defaultCells[i].SetLocked(true);
      defaultCells[i].SetIgnoreWrite(true);
    } else {
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
      }
    }
  }
  bool done = false;
  for (size_t tries=0; tries < 20 && !done; tries++) {

    Log("Filling ground...\n");
    this->cells = defaultCells;
    for (size_t i=0; i<this->cellCount; i++) {
      this->cells[i].SetWorld(this, GetCellPos(i));
    }

    this->defaultMask = std::vector<bool>(this->cellCount, true);

    instances.clear();
    instances.push_back(getFeature("start")->BuildFeature(state, *this, IVector3(32-4, 32-8,32-4), 0, 0, 0, nullptr, 0));
    instances.back().prevID = InvalidID;

    int loop = 0;
    int lastDir = 0;
    uint32_t minY = size.y;
    uint32_t maxY = 0;

    Log("Building features...\n");
    do {
      loop++;
      if (loop > 100000) break;

      // select a feature from which to go
      bool                      useLast     = random.Chance(useLastChance);
      bool                      useLastDir  = lastDir != 0 && useLast && random.Chance(useLastDirChance);
      ID                        featNum     = useLast ? instances.size()-1 : random.Integer(instances.size());
      const FeatureInstance &   instance    = instances[featNum];

      // select a random connection from the current feature
      const Feature *           feature     = instance.feature;
      const FeatureConnection * conn        = useLastDir ? feature->GetRandomConnection(lastDir, state) : feature->GetRandomConnection(state);
      if (!conn) continue;

      // select next feature
      const Feature *           nextFeature = conn->GetRandomFeature(state, instance.pos);
      if (!nextFeature) continue;

      // make sure both features can connect
      const FeatureConnection * revConn     = nextFeature->GetRandomConnection(-conn->dir, state);
      if (!revConn) continue;

      // snap both connection points together
      IVector3                  pos         = instance.pos + conn->pos - revConn->pos;

      // check if feature can be built
      this->BeginCheckOverwrite();
      nextFeature->BuildFeature(state, *this, pos, conn->dir, instance.dist, instances.size(), nullptr, featNum);
      if (!this->FinishCheckOverwrite()) continue;

      Log(".");

      // build it
      FeatureInstance           nextInstance = nextFeature->BuildFeature(state, *this, pos, conn->dir, instance.dist, instances.size(), revConn, featNum);
      minY = std::min(minY, pos.y);
      maxY = std::max(maxY, nextFeature->GetSize().y + pos.y);

      // replace some cells after connection if wanted
      feature->ReplaceChars(state, *this, instance.pos, conn->id, featNum);

      lastDir = conn->dir;
      loop    = 0;

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
            inst.feature->ReplaceChars(state, *this, inst.pos, c.id, inst.featureID);
            nextInstance.feature->ReplaceChars(state, *this, nextInstance.pos, nextConn.id, nextInstance.featureID);
          }
        }
      }

      // done :)
      instances.push_back(nextInstance);
      instances.back().prevID = featNum;
    } while(instances.size() < featureCount);
    Log("\n");

    int height = maxY - minY;

    // want at least 150 features and at least 16 cells high
    if (instances.size() < 150 || height < 16) {
      done = false;
      Log("Discarding boring world with %u features and height %d :( ...\n", instances.size(), height);
    } else {
      done = true;
      Log("Made a nice world with %u features and height %d :) ...\n", instances.size(), height);
    }
  }

  WorldEdit e(this);

  Log("Carving caves...\n");

  // create caves
  e.SetBrush(Cell("air"));
  size_t caveCount     = instances.size()>10 ? random.Integer(instances.size()/10) : 0;

  for (size_t i = 0; i<caveCount; i++) {
    size_t featNum = random.Integer(instances.size());
    const FeatureInstance &instance = instances[featNum];
    IVector3 size = instance.feature->GetSize();

    for (size_t k=0; k<caveRepeat; k++) {
      IVector3 cavePos(instance.pos + IVector3(random.Integer(size.x), random.Integer(size.y), random.Integer(size.z)));
      size_t caveLength = caveLengthMin + random.Integer(caveLengthMax-caveLengthMin);
      bool lastSolid = false;

      for (size_t j=0; j<caveLength; j++) {
        Side nextSide = (Side)(random.Integer(6));
        bool solid = this->GetCell(cavePos).GetInfo().flags & CellFlags::Solid;
        if (solid && !lastSolid) {
          e.ApplyBrush(cavePos);
        }
        lastSolid = solid;
        cavePos = cavePos[nextSide];
      }
    }
  }


  Log("Placing %u teleports...\n", teleportCount);
  for (size_t i=0; i<teleportCount; i++) {
    IVector3 a = GetRandomTeleportTarget(random);
    IVector3 b = GetRandomTeleportTarget(random);
    while (a == b) b = GetRandomTeleportTarget(random);

    SetCell(a, Cell("teleport")).SetTeleportTarget(b);
    SetCell(b, Cell("teleport")).SetTeleportTarget(a);
  }

  Log("Placing %u traps...\n", trapCount);
  for (size_t i=0; i<trapCount; i++) {

    IVector3 a = GetRandomTeleportTarget(random);

    Cell *aboveCell = &GetCell(a)[Side::Up][Side::Up];
    Side side = (Side)random.Integer(6);
    while (side == Side::Down) side = (Side)random.Integer(6);

    size_t distance = 0;
    while(!aboveCell->IsSolid()) {
      aboveCell = &((*aboveCell)[side]);
      distance ++;
    }

    if (distance > 5) { i--; continue; }

    Cell &trigger = GetCell(a);
    Cell &spawner = SetCell(aboveCell->GetPosition(), Cell("shooter"));
    // TODO: get entity from group "trap"
    const std::vector<std::string> &traps = GetEntitiesInGroup("trap");
    if (traps.empty()) {
      spawner.SetSpawnOnActive("projectile.bfw9k", -side, 0.0);
    } else {
      spawner.SetSpawnOnActive(traps[state.GetRandom().Integer(traps.size())], -side, 0.0);
    }

    ID id = state.GetNextTriggerId();
    spawner.SetTrigger(id, false);
    trigger.SetTriggerTarget(id);
  }

  Log("Spawning feature entities...\n");
  for (auto instance : instances) {
    instance.feature->SpawnEntities(state, instance.pos);
  }

  Log("Placing %u items...\n", itemCount);
  for (size_t i=0; i<itemCount; i++) {
    IVector3 a = GetRandomTeleportTarget(random);
    std::string itemName = getRandomItem("item", state.GetLevel(), state.GetRandom());
    ItemEntity *entity = new ItemEntity(itemName);
    entity->SetPosition(Vector3(a.x + 0.5, a.y + entity->GetAABB().extents.y+0.01, a.z + 0.5));
    state.AddEntity(entity);
  }

  Log("Placing some decoration (%u of them)...\n", decoCount);
  for (size_t i=0; i<decoCount; i++) {
    bool top = random.Coin();
    IVector3 a;
    if (top) {
      a = GetRandomTeleportTarget(random)[Side::Up];
    } else {
      a = GetRandomCeiling(random)[Side::Down];
    }
    Cell &cell = GetCell(a);
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
    if (top) {
      entity->SetPosition(Vector3(a.x + 0.5, a.y + entity->GetAABB().extents.y+0.01, a.z + 0.5));
    } else {
      entity->SetPosition(Vector3(a.x + 0.5, a.y - entity->GetAABB().extents.y+0.99, a.z + 0.5));
    }
  }

  Log("Placing %u enemies...\n", monsterCount);
  weighted_map<std::string> monsters;
  for (auto &m:GetEntitiesInGroup("monster")) {
    monsters[m] = GetEntityProbability(m, state.GetLevel());
  }
  for (size_t i=0; i<monsterCount; i++) {
    IVector3 a = GetRandomTeleportTarget(random);
    std::string monster = monsters.select(random.Float01());
    Entity *entity = Entity::Create(monster);
    if (!entity) continue;

    entity->SetPosition(Vector3(a.x + 0.5, a.y + entity->GetAABB().extents.y+0.01, a.z + 0.5));
    state.AddEntity(entity);
  }

  // remove spilt liquids
  Log("Wiping the floor...\n");
  bool foundLiquid = true;
  while(foundLiquid) {
    foundLiquid = false;
    for (size_t i=0; i<this->cellCount; i++) {
      if (!this->cells[i].IsLiquid()) continue;

      if (this->cells[i][Side::Down].GetType() == "air" ||
          this->cells[i][Side::Right].GetType() == "air" ||
          this->cells[i][Side::Left].GetType() == "air" ||
          this->cells[i][Side::Forward].GetType() == "air" ||
          this->cells[i][Side::Backward].GetType() == "air") {
        foundLiquid = true;
        this->SetCell(GetCellPos(i), Cell("air"));
      }
    }
  }

  // update all cells
  Log("Updating all cells...\n");
  for (size_t i=0; i<this->cellCount; i++) {
    this->MarkForUpdateNeighbours(i);
  }

  Update(state);
  Update(state);

  Log("Done!\n");
}

Cell &
World::SetCell(const IVector3 &pos, const Cell &cell, bool ignoreLock) {
  if (checkOverwrite) {
    if (!this->IsValidCellPosition(pos)) checkOverwriteOK = false;
    else if (!ignoreLock && this->cells[this->GetCellIndex(pos)].IsLocked())
      checkOverwriteOK = false;
    return this->defaultCell;
  }

  if (!this->IsValidCellPosition(pos)) return this->defaultCell;

  size_t i = this->GetCellIndex(pos);
  if (this->cells[i].GetIgnoreWrite()) return this->defaultCell;

  defaultMask[i] = false;

  size_t featId = this->cells[i].GetFeatureID();

  CellProperties info = this->cells[i].GetInfo();
  this->cells[i] = cell;
  this->cells[i].SetWorld(this, pos);
  this->cells[i].SetFeatureID(featId);
  this->UpdateCell(i);

  // ignore changes between invisible and dynamic cells, static mesh wont change
  /*
  this->dirty = !(((info.flags & CellFlags::DoNotRender) && (cell.GetInfo().flags & CellFlags::Dynamic)) ||
                  ((info.flags & CellFlags::Dynamic) && (cell.GetInfo().flags & CellFlags::DoNotRender)) );
                  */

  this->dirty = true;
  return this->cells[i];
}

/**
 * Update a cell after it was changed.
 * @param i Index of cell to update.
 */
void
World::UpdateCell(size_t i) {
  this->MarkForUpdateNeighbours(i);
  for (size_t n=0; n<6; n++) {
    this->MarkForUpdateNeighbours(&(this->cells[i][(Side)n]));
  }
}

/**
 * Update a cell after it was changed.
 * @param pos Position of cell to update.
 */
void
World::UpdateCell(const IVector3 &pos) {
  if (!IsValidCellPosition(pos)) return;
  UpdateCell(GetCellIndex(pos));
}

/**
 * Render the entire world.
 */
void
World::Draw(Gfx &gfx) {
  PROFILE();

  if (dirty) {
    PROFILE_NAMED("Vertex Update");
    // world has been changed, recreate vertex buffers

    this->defaultCell = Cell("default");

    this->allVerts.clear();
    this->dynamicCells.clear();

    if (firstDirty) {
      // fill up liquids with liquids above, so it won't trickle
      for (size_t i=0; i<this->cellCount; i++) {
        if (this->cells[i].IsLiquid() && this->cells[i][Side::Up].GetInfo() == this->cells[i].GetInfo()) {
          this->cells[i].SetDetail(16);
        }
      }

      firstDirty = false;
    }

    std::unordered_map<const Texture *, std::vector<Vertex>> verticesNormal;
    std::unordered_map<const Texture *, std::vector<Vertex>> verticesEmissive;

    size_t updateCount = 0;

    for (size_t i=0; i<this->cellCount; i++) {
      Cell &cell = this->cells[i];
      const CellProperties &info = cell.GetInfo();

      // don't bother with invisible cells
      if (info.flags & CellFlags::DoNotRender || !cell.GetVisibility()) continue;

      // don't add dynamic cells to static vertex buffer
      if (cell.IsDynamic()) {
        dynamicCells.push_back(i);
        continue;
      }

      if (cell.UpdateVertices()) {
        updateCount ++;
      }

      // group vertex buffers by texture

      const Texture *tex = cell.GetTexture();
      if (tex) cell.Draw(verticesNormal[tex]);

      const Texture *etex = cell.GetEmissiveTexture();
      if (etex) cell.DrawEmissive(verticesEmissive[etex]);
    }

    if (updateCount) Log("%u cell vertex updates\n", updateCount);

    size_t index = 0;
    this->allVerts.clear();

    for (auto &iter : verticesNormal) {
      this->vertexStartsNormal[iter.first] = index;
      this->vertexCountsNormal[iter.first] = iter.second.size();
      index += iter.second.size();

      for (auto &v : iter.second) {
        this->allVerts.push_back(v);
      }
    }

    for (auto &iter : verticesEmissive) {
      this->vertexStartsEmissive[iter.first] = index;
      this->vertexCountsEmissive[iter.first] = iter.second.size();
      index += iter.second.size();

      for (auto &v : iter.second) {
        this->allVerts.push_back(v);
      }
    }

    // set vertex buffer data
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    if (this->allVerts.size())
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*(this->allVerts.size()), &this->allVerts[0], GL_STATIC_DRAW);

    dirty = false;
  }

  gfx.SetShader("default");
  gfx.SetColor(IColor(255,255,255));
  gfx.SetLight(IColor(255,255,255));
  gfx.SetBlendNormal();

  {
    PROFILE_NAMED("Static Draw");

    for (auto &s : this->vertexStartsNormal) {
      gfx.SetTextureFrame(s.first);
      gfx.DrawTriangles(this->vbo, s.second, this->vertexCountsNormal[s.first]);
    }

    gfx.SetBlendAdd();
    for (auto &s : this->vertexStartsEmissive) {
      gfx.SetTextureFrame(s.first);
      gfx.DrawTriangles(this->vbo, s.second, this->vertexCountsEmissive[s.first]);
    }
  }

  {
    PROFILE_NAMED("Dynamic Draw");

    // get vertices for dynamic cells
    std::unordered_map<const Texture *, std::vector<Vertex>> dynVerticesNormal;
    std::unordered_map<const Texture *, std::vector<Vertex>> dynVerticesEmissive;

    for (size_t i : dynamicCells) {
      const Texture *tex = this->cells[i].GetTexture();
      if (dynVerticesNormal.find(tex) == dynVerticesNormal.end())
          dynVerticesNormal[tex] = std::vector<Vertex>();

      const Texture *etex = this->cells[i].GetEmissiveTexture();
      if (dynVerticesEmissive.find(etex) == dynVerticesEmissive.end())
          dynVerticesEmissive[etex] = std::vector<Vertex>();

      this->cells[i].UpdateVertices();
      this->cells[i].Draw(dynVerticesNormal[tex]);
      this->cells[i].DrawEmissive(dynVerticesEmissive[etex]);
    }

    // render vertices for dynamic cells
    gfx.SetBlendNormal();
    auto iter = dynVerticesNormal.begin();
    for (size_t i=0; i<dynVerticesNormal.size(); i++, iter++) {
      gfx.SetTextureFrame(iter->first);
      gfx.DrawTriangles(iter->second);
    }

    gfx.SetBlendAdd();
    iter = dynVerticesEmissive.begin();
    for (size_t i=0; i<dynVerticesEmissive.size(); i++, iter++) {
      gfx.SetTextureFrame(iter->first);
      gfx.DrawTriangles(iter->second);
    }
  }
}

void World::MarkForUpdateNeighbours(const Cell *cell) {
//  cell.UpdateNeighbours();
  size_t i = GetCellIndex(cell->GetPosition());
  this->neighbourUpdates.insert(i);
}

void World::MarkForUpdateNeighbours(size_t i) {
  this->neighbourUpdates.insert(i);
}

void
World::Update(
  RunningState &state
) {
  PROFILE();

  // update all dynamic cells
  //Log("updating %u dynamic cells\n", this->dynamicCells.size());
  for (size_t i : this->dynamicCells) {
    this->cells[i].Update(state);
  }

  //Log("updating %u neighbours\n", this->neighbourUpdates.size());
  size_t neighbourCount = 0;

  while(this->neighbourUpdates.size()) {
    std::unordered_set<size_t> tmp = this->neighbourUpdates;
    this->neighbourUpdates.clear();

    for (auto &i:tmp) {
      this->cells[i].UpdateNeighbours();
      neighbourCount++;
    }
  }

  if (neighbourCount > 0) {
    this->dirty = true;
    Log("updated %u neighbours\n", neighbourCount);
  }

  // tick world
  {
      PROFILE_NAMED("Tick");
    while (tickInterval != 0.0 && state.GetGame().GetTime() > nextTickT) {
      for (size_t i : this->dynamicCells) {
        this->cells[i].Tick(state);
      }
      nextTickT += tickInterval;
    }
  }
}

/**
 * Cast a ray from a given location along the x axis.
 * @param org Ray origin.
 * @param dir Ray direction.
 * @return true if the next adjacent cell in the given direction is solid and
 *         top of cell at that position is above the ray (hitting solid part
 *         of the cell).
 */
bool
World::CastRayX(const Vector3 &org, float dir) {
  bool movingRight = dir > 0;
  size_t x = org.x - (movingRight?0.01f:-0.01f); // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z; // start cell z

  // end cell x
  size_t x2 = (org.x+dir + 2*(movingRight?0.01f:-0.01f));
  if (x2 == x) return false; // not crossing cell borders

  return this->GetCell(IVector3(x,y,z)).CheckSideSolid(movingRight ? Side::Right : Side::Left, org);
}

/**
 * Cast a ray from a given location along the z axis.
 * @param org Ray origin.
 * @param dir Ray direction.
 * @return true if the next adjacent cell in the given direction is solid and
 *         top of cell at that position is above the ray (hitting solid part
 *         of the cell).
 */
bool
World::CastRayZ(const Vector3 &org, float dir) {
  bool movingForward = dir > 0;
  size_t x = org.x; // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z - (movingForward ? 0.01f : -0.01f); // start cell z

  size_t z2 = (org.z + dir + 2*(movingForward ? 0.01f : -0.01f));
  if (z2 == z) return false; // not crossing cell borders

  return this->GetCell(IVector3(x,y,z)).CheckSideSolid(movingForward?Side::Forward:Side::Backward, org);
}

/**
 * Cast a ray from a given location along the y axis up.
 * @param org Ray origin.
 * @return The end position of the ray where it hits a solid cell. Takes into
 *         account the bottom y-offsets of the cell.
 */
float
World::CastRayYUp(const Vector3 &org) {
  size_t x = org.x; // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z; // start cell z

  if (org.y < 0) return 0;

  if (GetCell(IVector3(x,y,z)).IsSolid() && y > GetCell(IVector3(x,y,z)).GetHeightBottom(org.x, org.z)) return y;

  while (y < this->size.y) {
    const Cell &cell = this->GetCell(IVector3(x,y,z));
    if (cell.GetInfo().flags & CellFlags::Solid) {
      return y + cell.GetHeightBottom(org.x, org.z);
    }
    y++;
  }
  return this->size.y;
}

/**
 * Cast a ray from a given location along the y axis down.
 * @param org Ray origin.
 * @return The end position of the ray where it hits a solid cell. Takes into
 *         account the top y-offsets of the cell.
 */
float
World::CastRayYDown(const Vector3 &org) {
  size_t x = org.x; // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z; // start cell z

  if (y >= this->size.y) return this->size.y;

  if (GetCell(IVector3(x,y,z)).IsSolid() && y < GetCell(IVector3(x,y,z)).GetHeight(org.x, org.z)) return y-1;

  float endY = 0;

  // iterate from the bottom of the world up to the start cell
  for (size_t yy = 0; yy <= y; yy++) {
    const Cell &cell = this->GetCell(IVector3(x,yy,z));
    if (cell.GetInfo().flags & CellFlags::Solid) {
      endY = yy + cell.GetHeight(org.x, org.z);
    }
  }
  return endY;
}

/**
 * Check if a point is within the solid of a cell.
 * @param org Point to check
 * @return true if point is inside a solid.
 */
bool
World::IsPointSolid(const Vector3 &org) {
  size_t x = org.x; // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z; // start cell z

  const Cell &cell = this->GetCell(IVector3(x,y,z));
  bool heightCheck = (org.y - y) >= cell.GetHeightBottom(org.x, org.z) && (org.y - y) <= cell.GetHeight(org.x, org.z);
  return cell.IsSolid() && heightCheck;
}

/**
 * Check if an AABB is intersection with solid geometry.
 * This checks each vertex position of the AABB, then check the
 * lines between the top and bottom vertices for solidity.
 * @note Experimental
 * @param aabb AABB to check
 * @return true if AABB intersects solid geometry.
 */
bool
World::IsAABBSolid(const AABB &aabb) {
  std::vector<Vector3> verts;
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y,  aabb.extents.z));

  for (Vector3 v : verts) {
    if (IsPointSolid(aabb.center + v)) return true;
  }

  float top = aabb.center.y + aabb.extents.y + 0.01;
  if (CastRayYUp(verts[0]) < top) return true;
  if (CastRayYUp(verts[1]) < top) return true;
  if (CastRayYUp(verts[2]) < top) return true;
  if (CastRayYUp(verts[3]) < top) return true;

  float bot = aabb.center.y - aabb.extents.y - 0.01;
  if (CastRayYDown(verts[4]) > bot) return true;
  if (CastRayYDown(verts[5]) > bot) return true;
  if (CastRayYDown(verts[6]) > bot) return true;
  if (CastRayYDown(verts[7]) > bot) return true;

  return false;
}

/**
 * Move a AABB through the world, doing collision detection and response.
 * @param aabb The AABB to move.
 * @param targ The target position where to move the AABB.
 * @param axis Output of the colliding axis flags.
 * @param[out] cell Returns one of the cell with which the aabb collided.
 * @param[out] side Returns the side of the cell that collided.
 * @return The final center position of the AABB.
 * @note Might not work for too big AABBs.
 */
Vector3 World::MoveAABB(
  const AABB &aabb,
  const Vector3 &targ,
  uint8_t &axis,
  Cell **cell,
  Side *side
) {
  // initialize axis to no collisions
  axis = 0;

  Vector3 center = aabb.center;
  Vector3 dist = targ-aabb.center;

  // are we moving at all?
  if (dist.GetSquareMag() == 0.0f) return center;

  bool movingUp      = dist.y > 0;
  bool storeCell = cell != nullptr;

  // create vertices that serve as origins for ray check. must be at most
  // one cell size apart for correct collision detection.
  std::vector<Vector3> verts;
  aabb.GetVertices(verts);

  Vector3 target = targ;
  while(true) {
    // CastRayX/Z only check nearest neighbouring cell, limit movement to 1.0
    // and reiterate if neccessary.
    bool keepGoing = false;
    Vector3 d = (target-center).Horiz();

    if (d.GetMag() > 1.0) {
      d = d.Normalize()*1.0;
      keepGoing = true;
    }

    // try to move along the x axis
    Vector3 ddx(d.x + (d.x>0?0.01:-0.01), 0, 0);

    for (const Vector3 &v : verts) {
      if (CastRayX(center + v, d.x) || IsPointSolid(center + v + ddx)) {
        float newX = d.x;
        if (d.x > 0) {
          newX = ((int)center.x + 1) - aabb.extents.x - 0.01f;
        } else if (d.x < 0) {
          newX = ((int)center.x) + aabb.extents.x + 0.01f;
        }

        if (std::abs(newX - center.x) < std::abs(d.x))
          d.x = newX - center.x;

        axis |= Axis::X;

        if (storeCell) {
          *cell = &GetCell(center+v+ddx);
          if (side) *side = d.x > 0 ? Side::Left : Side::Right;
          storeCell = false;
        }
      }
    }

    // update center to new position
    center.x += d.x;

    // try to move along the z axis
    Vector3 ddz(0, 0, d.z + (d.z>0?0.01:-0.01));

    for (const Vector3 &v : verts) {
      if (CastRayZ(center + v, d.z) || IsPointSolid(center + v + ddz)) {
        float newZ = d.z;
        if (d.z > 0) {
          newZ = ((int)center.z + 1) - aabb.extents.z - 0.01f;
        } else if (d.z < 0) {
          newZ = ((int)center.z) + aabb.extents.z + 0.01f;
        }

        if (std::abs(newZ - center.z) < std::abs(d.z))
          d.z = newZ - center.z;

        axis |= Axis::Z;

        if (storeCell) {
          *cell = &GetCell(center+v+ddz);
          if (side) *side = d.z > 0 ? Side::Backward : Side::Forward;
          storeCell = false;
        }
      }
    }

    // update center to new position
    center.z += d.z;

    if (axis & Axis::X) target.x = center.x;
    if (axis & Axis::Z) target.z = center.z;

    // check if nothing left to do for horizontal movement
    if (axis & Axis::X && axis & Axis::Z) break;

    // was this the final pass?
    if (!keepGoing) break;
  }

  // now try to move along y axis
  if (movingUp) {
    float endY = size.y;
    // get lowest collision point
    for (const Vector3 &v : verts) {
      if (v.y <= 0.0f) continue;
      float vY = CastRayYUp(center+v);
      if (vY < endY) {
        endY = vY;

        if (storeCell && endY - aabb.extents.y < target.y) {
          Vector3 end = center + v;
          end.y = endY + 0.01;
          *cell = &GetCell(end);
          if (side) *side = Side::Down;
          storeCell = false;
        }
      }
    }

    if (endY - aabb.extents.y < target.y) {
      // we are too high, move top of aabb to collision height
      axis |= Axis::Y;
      target.y = endY - aabb.extents.y - 0.001f;
    }
  } else {
    float endY = 0;
    // get highest collision point
    for (const Vector3 &v : verts) {
      if (v.y >= 0.0f) continue;
      float vY = CastRayYDown(center+v);
      if (vY > endY) {
        endY = vY;

        if (storeCell && endY + aabb.extents.y > target.y) {
          Vector3 end = center + v;
          end.y = endY + 0.01;
          *cell = &GetCell(end);
          if (side) *side = Side::Up;
          storeCell = false;
        }
      }
    }
    if (endY + aabb.extents.y > target.y) {
      // we are too low, move top of aabb to collision height
      axis |= Axis::Y;
      target.y = endY + aabb.extents.y + 0.001f;
    }
  }

  // update y position
  center.y = target.y;

  // done
  return center;
}

Vector3
World::MoveAABB(
  const AABB &aabb,
  const Vector3 &target
) {
  uint8_t axis;
  return this->MoveAABB(aabb, target, axis);
}

IColor
World::GetLight(const IVector3 &pos) const {
  return (GetCell(pos).GetLightLevel().Gamma(2.2)) + ambientLight;
}

/**
 * Write some ASCII art representation of this world to a file. (DEBUG)
 */
void
World::Dump() {
  FILE *f = fopen("world.txt", "w");
  for (size_t z = 0; z<size.z; z++) {
    for (size_t x = 0; x<size.x; x++) {
      int air = 0;
      for (size_t y = 0; y<size.y; y++) {
        if ((this->GetCell(IVector3(x,y,z)).GetInfo().flags & CellFlags::Solid) == 0) {
          air ++;
        }
      }
      if (air) {
        air /= 5;
        if (air > 4) air = 4;
        fprintf(f, "%c", "%+:. "[air]);
      } else {
        fprintf(f, "#");
      }
    }
    fprintf(f, "\n");
  }
  fclose(f);
}

/**
 * Find the final cell along a ray.
 * @param org Ray origin
 * @param dir Ray direction
 * @param[out] distance Distance along the ray to the surface of the cell
 * @param[out] side Side of the cell that was hit
 * @param[in] flags Flags a cell can have to be chosen for hit.
 * @return The hit cell.
 * @note This will always return a cell. Outside the world or on the boundaries
 *       it will be the default cell, which has no world or position value.
 */
Cell &
World::CastRayCell(const Vector3 &org, const Vector3 &dir, float &distance, Side &side, size_t flags) {
  int dx = dir.x == 0 ? 0 : (dir.x > 0 ? 1 : -1);
  int dy = dir.y == 0 ? 0 : (dir.y > 0 ? 1 : -1);
  int dz = dir.z == 0 ? 0 : (dir.z > 0 ? 1 : -1);
  Vector3 pos = org;

  size_t x = pos.x; // start cell x
  size_t y = pos.y; // start cell y
  size_t z = pos.z; // start cell z

  Cell *currentCell = nullptr;

  while (IsValidCellPosition(IVector3(x,y,z))) {
    currentCell = &this->GetCell(IVector3(x,y,z));

    float tt;
    Vector3 p;

    if ((currentCell->GetInfo().flags & flags) && currentCell->Ray(org, dir, tt, p) && tt > 0) {
      distance = tt;
      return *currentCell;
    }

    float u = INFINITY;
    float v = INFINITY;
    float w = INFINITY;

    if (dir.x>0) u = (x+1-pos.x)/dir.x;
    if (dir.x<0) u = (x  -pos.x)/dir.x;
    if (dir.y>0) v = (y+1-pos.y)/dir.y;
    if (dir.y<0) v = (y  -pos.y)/dir.y;
    if (dir.z>0) w = (z+1-pos.z)/dir.z;
    if (dir.z<0) w = (z  -pos.z)/dir.z;

    if (u < v && u < w) {
      pos = pos + dir * u;
      x += dx;
      side = dx > 0 ? Side::Left : Side::Right;
    } else if (v < u && v < w) {
      pos = pos + dir * v;
      y += dy;
      side = dy > 0 ? Side::Down : Side::Up;
    } else if (w < u && w < v) {
      pos = pos + dir * w;
      z += dz;
      side = dz > 0 ? Side::Backward : Side::Forward;
    } else {
      break;
    }
  }

  if (!currentCell) return defaultCell;
  return *currentCell;
}

/**
 * Check if a cell at a given position is still original or if it
 * is part of a feature.
 * @param pos Cell position.
 * @return true if cell wasn't modified.
 */
bool
World::IsDefault(const IVector3 &pos) {
  if (!IsValidCellPosition(pos)) return true;
  return defaultMask[GetCellIndex(pos)];
}

void
World::BreakBlock(const IVector3 &pos) {
  const CellProperties &info = this->GetCell(pos).GetInfo();

  if (info.type == "air") return;

  std::string particleType = info.breakParticle;
  AABB aabb = this->SetCell(pos, Cell("air")).GetAABB();

  if (particleType != "") {
    Random &random = state.GetRandom();
    for (size_t i=0; i<4; i++) {
      state.SpawnInAABB(particleType, aabb, random.Vector());
    }
  }

  for (int i=0; i<6; i++) {
    if (info.onUseCascade & (1<<i) && this->GetCell(pos[(Side)i]).GetInfo() == info)
      BreakBlock(pos[(Side)i]);
  }
}

bool
World::IsCellWalkable(const IVector3 &pos) const {
  const Cell &cell = GetCell(pos);
  const Cell &cell2 = cell[Side::Up];
  const Cell &cell3 = cell2[Side::Up];
  return cell.IsSolid() && !cell2.IsSolid() && !cell2.IsLiquid() && !cell3.IsSolid() && !cell3.IsLiquid();
}

bool
World::IsCellValidTeleportTarget(const IVector3 &pos) const {
  return IsCellWalkable(pos) && (
    IsCellWalkable(pos[Side::Right]) ||
    IsCellWalkable(pos[Side::Left]) ||
    IsCellWalkable(pos[Side::Forward]) ||
    IsCellWalkable(pos[Side::Backward])); // TODO: check triggers
}

bool
World::IsCellValidCeiling(const IVector3 &pos) const {
  const Cell &cell = GetCell(pos);
  const Cell &cell2 = cell[Side::Down];
  const Cell &cell3 = cell2[Side::Down];
  return cell.IsBottomFlat() && cell.IsSolid() && !cell2.IsSolid() && !cell2.IsLiquid() && !cell3.IsSolid() && !cell3.IsLiquid();
}

IVector3
World::FindSolidBelow(const IVector3 &pos) const {
  IVector3 p(pos);
  while(!GetCell(p).IsSolid())
    p = p[Side::Down];

  return p;
}

IVector3
World::FindSolidAbove(const IVector3 &pos) const {
  IVector3 p(pos);
  while(!GetCell(p).IsSolid())
    p = p[Side::Up];

  return p;
}

IVector3
World::GetRandomTeleportTarget(Random &random) const {
  IVector3 pos;
  do {
    pos.x = random.Integer(size.x);
    pos.y = random.Integer(size.y);
    pos.z = random.Integer(size.z);
    pos = FindSolidBelow(pos);
  } while(!IsCellValidTeleportTarget(pos));
  return pos;
}

IVector3
World::GetRandomCeiling(Random &random) const {
  IVector3 pos;
  do {
    pos.x = random.Integer(size.x);
    pos.y = random.Integer(size.y);
    pos.z = random.Integer(size.z);
    pos = FindSolidAbove(pos);
  } while(!IsCellValidCeiling(pos));
  return pos;
}

void World::TriggerOn(size_t id) {
  for (size_t i=0; i<this->cellCount; i++) {
    if (this->cells[i].GetTriggerId() == id) this->cells[i].TriggerOn();
  }
}

void World::TriggerOff(size_t id) {
  for (size_t i=0; i<this->cellCount; i++) {
    if (this->cells[i].GetTriggerId() == id) this->cells[i].TriggerOff();
  }
}



Serializer &operator << (Serializer &ser, const World &world) {
  ser << world.minimap;
  ser << world.size;

  for (auto &c:world.cells)
    ser << c;

  ser << world.defaultMask;
  ser << world.nextTickT;
  ser << world.tickInterval;
  ser << world.ambientLight;
  return ser;
}





// ----------------------------

MiniMap::MiniMap(const World &world) :
  world(world),
  seenFeatures(0),
  mapTexture(nullptr),
  viewY(0)
{
}

MiniMap::MiniMap(const World &world, Deserializer &deser) :
  world(world),
  mapTexture(nullptr),
  viewY(0)
{
  deser >> this->seenFeatures;
}

void
MiniMap::Draw(
  Gfx &gfx,
  const Vector3 &eyePos,
  float angle
) {
  PROFILE();

  size_t y = eyePos.y;
  if (y != this->viewY) {
    this->viewY = y;
    this->RepaintMap();
  }

  const IVector3 &size = world.GetSize();

  gfx.SetTextureFrame(this->mapTexture);
  gfx.SetColor(IColor(255,255,255));
  gfx.SetLight(IColor(255,255,255));
  gfx.GetView().Push();
  gfx.GetView().Translate(Vector3(-1 + 2*eyePos.x/size.x, -1 + 2*eyePos.z/size.z, 0));
  gfx.GetView().Scale(Vector3(-1, 1, 1));
  gfx.DrawUnitQuad();
  gfx.GetView().Pop();

  gfx.SetTextureFrame(loadTexture("gui/maparrow"));
  gfx.GetView().Push();
  gfx.GetView().Rotate(angle, Vector3(0,0,1));
  gfx.GetView().Scale(Vector3(-1.0/4.0, 1.0/4.0, 1));
  gfx.DrawUnitQuad();
  gfx.GetView().Pop();
}

/**
 * Mark a feature as seen.
 * @param f Feature id.
 */
void
MiniMap::AddFeatureSeen(ID f) {
  if (f == InvalidID) return;

  // make sure vector is large enough
  if (this->seenFeatures.size() <= f) {
    this->seenFeatures.resize(f+1, false);
  }

  if (this->seenFeatures[f]) return;

  this->seenFeatures[f] = true;

  RepaintMap();
}

bool
MiniMap::IsFeatureSeen(ID id) const {
  if (id == InvalidID || id >= this->seenFeatures.size()) return false;
  return this->seenFeatures[id];
}

void
MiniMap::RepaintMap() {
  const IVector3 &size = world.GetSize();
  uint8_t pixels[size.x*size.z*4];

  for (size_t x=0; x<size.x; x++) {
    for (size_t z=0; z<size.z; z++) {
      size_t index = (x+(size.z-1-z)*size.x)*4;
      pixels[index+3] = 0;

      for (size_t yy=viewY; yy>0; yy--) {
        IVector3 pos(x,yy,z);
        Cell &cell = this->world.GetCell(pos);
        if (!cell.IsSeen(2)) continue;

        bool solid = !cell.IsTransparent() && !cell[Side::Up].IsTransparent() && !cell[Side::Down].IsTransparent();
        uint8_t v = yy - viewY + 64;
        if (solid) {
          pixels[index+0] = v;
          pixels[index+1] = v;
          pixels[index+2] = v;
          pixels[index+3] = 255;
        } else {
          pixels[index+0] = v/2;
          pixels[index+1] = v/2;
          pixels[index+2] = v/2;
          pixels[index+3] = 255;
          break;
        }
      }
    }
  }

  this->mapTexture = updateTexture("*minimap", Point(size.x, size.z), pixels);
}

Serializer &operator << (Serializer &ser, const MiniMap &map) {
  ser << map.seenFeatures;
  return ser;
}
