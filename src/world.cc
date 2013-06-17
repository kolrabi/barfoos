#include "GLee.h"

#include "world.h"
#include "cell.h"
#include "util.h"
#include "mob.h"
#include "simplex.h"

#include "random.h"
#include "serializer.h"
#include "worldedit.h"

World::World(const IVector3 &size, int level, Random &rnd) :random(rnd)
{  
  this->ambientLight = IColor(32, 32, 64);
  this->checkOverwrite = false;
  this->size = size;
  this->dirty = true;
  this->firstDirty = true;
  this->level = level;

  this->cellCount = size.x * size.y * size.z;
  this->cells = std::vector<Cell>(this->cellCount, Cell("default"));
  this->defaultMask = std::vector<bool>(this->cellCount, true);
  
  IVector3 r(random.Integer(), random.Integer(), random.Integer());
  std::cerr << r << std::endl;
  std::cerr << this->cellCount << " cells, " << (sizeof(Cell)*this->cellCount) << std::endl;
  
  for (size_t i=0; i<this->cellCount; i++) {
    IVector3 pos = GetCellPos(i);
    if (pos.x < 2 || pos.y < 2 || pos.z < 2 || pos.x >= size.x-2 || pos.y >= size.y-2 || pos.z >= size.z-2) {
      this->cells[i] = Cell("bedrock");
      this->cells[i].SetLocked(true);
      this->cells[i].SetIgnoreWrite(true);
    } else {
      IVector3 ppp(pos+r);
      ppp.x %= 256;
      ppp.y %= 256;
      ppp.z %= 256;
      Vector3 vpos(ppp);
      float f = (simplexNoise(vpos*0.03) * simplexNoise(vpos*0.06));
      float g = (simplexNoise(vpos*(-0.05)));
      if (g < -0.5) {
        SetCell(pos, Cell("rock"));
      } else if (f > 0) {
        this->cells[i] = Cell("rock");
      } else if (f < -0.5) {
        SetCell(pos, Cell("brick"));
      }
    }
    this->cells[i].SetWorld(this);
    this->cells[i].SetPosition(pos);
  }

  // reinitialize, as it is modified by above SetCell  
  this->defaultMask = std::vector<bool>(this->cellCount, true);
  
  this->lastT = 0;
  this->tickInterval = 1;
  this->nextTickT = 0;

  // build features -------------------------------------------

  std::map<const Feature *, int> featureCounts;
  
  instances.push_back(getFeature("start")->BuildFeature(this, IVector3(32-4, 32-8,32-4), 0, 0, instances.size()));
  instances.back().prevID = ~0UL;
  
  int loop = 0;
  do {
    if (loop++ > 10000) break;

    // select a feature from which to go
    bool useLast = random.Chance(0.90);
    size_t featNum = useLast ? instances.size()-1 : (random.Integer(instances.size()));
    const FeatureInstance &instance = instances[featNum];
    
    // select a random connection from the current feature
    const Feature *feature = instance.feature;
    const FeatureConnection *conn = feature->GetRandomConnection(random);
    if (!conn) continue;

    // select next feature
    const Feature *nextFeature = conn->GetRandomFeature(this, instance.pos, random);
    if (!nextFeature) continue;
    
    // make sure both features can connect
    const FeatureConnection *revConn = nextFeature->GetRandomConnection(-conn->dir, random);
    if (!revConn) continue;

    // snap both connection points together
    IVector3 pos = instance.pos + conn->pos - revConn->pos;
    
    // build the next feature if possible
    this->BeginCheckOverwrite();
    nextFeature->BuildFeature(this, pos, conn->dir, instance.dist, instances.size());
    if (this->FinishCheckOverwrite()) {
      instances.push_back(nextFeature->BuildFeature(this, pos, conn->dir, instance.dist, instances.size()));
      instances.back().prevID = featNum;

      /*
      auto m = std::make_shared<Mob>(Mob());
      m->SetPosition(instances.back().pos + (instances.back().feature->GetSize())/2);
      m->SetSpawnPos(instances.back().pos + (instances.back().feature->GetSize())/2);
      this->AddMob(m);*/
    }
  } while(instances.size() < 500); 

  std::cerr << "built world with " << instances.size() << " features. level " << (level) << std::endl;
 
  WorldEdit e(this);
  
  // create caves
  size_t caveCount = random.Integer(30)+5;
  
  e.SetBrush(Cell("air"));

  for (size_t i = 0; i<caveCount; i++) {
    size_t featNum = random.Integer(instances.size());
    const FeatureInstance &instance = instances[featNum];
    IVector3 size = instance.feature->GetSize();
   
    for (size_t k=0; k<10; k++) {
    IVector3 cavePos(instance.pos + IVector3(random.Integer(size.x), random.Integer(size.y), random.Integer(size.z)));
    size_t caveLength = random.Integer(300);
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
  this->Dump();
}

World::~World() {
  if (this->vbos.size()) glDeleteBuffersARB(this->vbos.size(), &this->vbos[0]);
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
  this->cells[i] = cell;
  this->cells[i].SetWorld(this);
  this->cells[i].SetPosition(pos);
  this->cells[i].SetFeatureID(featId);
  this->cells[i].UpdateNeighbours();

  //this->lightDirty = true;
  this->dirty = true;
  return this->cells[i];
}

/**
 * Update a cell after it was changed.
 * @param i Index of cell to update.
 */
void 
World::UpdateCell(size_t i) {
  this->cells[i].UpdateNeighbours();
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
World::Draw() {
  if (dirty) {
    // world has been changed, recreate vertex buffers
    if (this->vbos.size()) {
      glDeleteBuffers(this->vbos.size(), &this->vbos[0]);
      this->vbos.clear();
    }

    this->defaultCell = Cell("default");

    this->vertices = std::map<GLuint, std::vector<Vertex>>();
    this->dynamicCells.clear();

    if (firstDirty) {
      for (size_t i=0; i<this->cellCount; i++) {
        this->cells[i].UpdateNeighbours();
      }
      firstDirty = false;
    }

    for (size_t i=0; i<this->cellCount; i++) {
      Cell &cell = this->cells[i];
      const CellInfo &info = cell.GetInfo();

      // don't add dynamic cells to static vertex buffer
      if (info.flags & CellFlags::Dynamic) {
        dynamicCells.push_back(i);
        continue;
      }
     
      // don't bother with invisible cells 
      if (info.flags & CellFlags::DoNotRender || !cell.GetVisibility()) continue;

      cell.UpdateVertices();

      // group vertex buffers by texture
      unsigned int tex = cell.GetTexture();
      if (this->vertices.find(tex) == this->vertices.end()) 
        this->vertices[tex] = std::vector<Vertex>();
 
      cell.Draw(this->vertices[tex]);
    }

    // one vertex buffer for each texture
    this->vbos.resize(vertices.size());
    glGenBuffers(this->vbos.size(), &this->vbos[0]);

    // set vertex buffer data
    auto iter = vertices.begin();
    for (size_t i=0; i<this->vertices.size(); i++, iter++) {
      glBindBuffer(GL_ARRAY_BUFFER, this->vbos[i]);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*(iter->second.size()), &iter->second[0], GL_STATIC_DRAW);
    }

    dirty = false;
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // draw each vertex buffer 
  auto iter = vertices.begin();
  for (size_t i=0; i<this->vertices.size(); i++, iter++) {
    glBindBuffer       (GL_ARRAY_BUFFER, this->vbos[i]);
    //glBindBuffer       (GL_ARRAY_BUFFER, 0);
    glBindTexture      (GL_TEXTURE_2D,   iter->first);
    glInterleavedArrays(GL_T2F_C3F_V3F,  sizeof(Vertex), nullptr);
    //glInterleavedArrays(GL_T2F_C3F_V3F,  sizeof(Vertex), &iter->second[0]);
    glDrawArrays       (GL_TRIANGLES,    0, iter->second.size());
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // get vertices for dynamic cells
  std::map<GLuint, std::vector<Vertex>> dynvertices;
  for (size_t i : dynamicCells) {
    unsigned int tex = this->cells[i].GetTexture();
    if (dynvertices.find(tex) == dynvertices.end()) 
        dynvertices[tex] = std::vector<Vertex>();
    this->cells[i].UpdateVertices();
    this->cells[i].Draw(dynvertices[tex]);
  }

  // render vertices for dynamic cells
  iter = dynvertices.begin();
  for (size_t i=0; i<dynvertices.size(); i++, iter++) {
    glBindTexture      (GL_TEXTURE_2D,  iter->first);
    glInterleavedArrays(GL_T2F_C3F_V3F, sizeof(Vertex), &iter->second[0]);
    glDrawArrays       (GL_TRIANGLES,   0, iter->second.size());
  }

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  // draw all mobs
  for (auto mob : this->mobs) {
    mob->Draw();
  }
}

void
World::DrawMap(
) {
  std::vector<Vertex> verts;

  for (size_t i=0; i<this->instances.size(); i++) {
    if (i >= this->seenFeatures.size() || !this->seenFeatures[i]) continue;

    const FeatureInstance &inst = this->instances[i];

    Vector3 pos(inst.pos);
    Vector3 size(inst.feature->GetSize());
    pos.x += 0.5;
    pos.z += 0.5;
    pos.y += size.y*0.5;
    size.x -= 1.0;
    size.z -= 1.0;

    verts.push_back(Vertex(pos,                          IColor(255,255,255), 0, 0));
    verts.push_back(Vertex(pos+Vector3(     0,0,size.z), IColor(255,255,255), 0, 1));
    verts.push_back(Vertex(pos+Vector3(size.x,0,size.z), IColor(255,255,255), 1, 1));
    verts.push_back(Vertex(pos+Vector3(size.x,0,     0), IColor(255,255,255), 1, 0));
  }
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  
  glBindTexture      (GL_TEXTURE_2D,  0);
  glInterleavedArrays(GL_T2F_C3F_V3F, sizeof(Vertex), &verts[0]);
  glDrawArrays       (GL_QUADS,       0, verts.size());

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
}

void 
World::Update(
  float t
) {
  if (lastT == 0) {
    lastT = t;
    nextTickT = t;
  }
  deltaT = t - lastT;

  // handle collision between mobs
  for (auto mob1 : this->mobs) {
    for (auto mob2 : this->mobs) {
      if (mob1 == mob2) continue;
      if (mob1->GetAABB().Overlap(mob2->GetAABB())) {
        Vector3 d = mob2->GetAABB().center - mob1->GetAABB().center;
        Vector3 f = d * (100 / (1+d.GetSquareMag()));
        mob2->ApplyForce(f);
        mob1->ApplyForce(f * (-1));
      }
    }
  }

  // update all mobs
  for (auto mob : this->mobs) {
    mob->Update(t);
  }

  // update all dynamic cells
  for (size_t i : this->dynamicCells) {
    this->cells[i].Update(t, random);
  }

  while (tickInterval != 0.0 && t > nextTickT) {
    nextTickT += tickInterval;
    for (size_t i : this->dynamicCells) {
      this->cells[i].Tick(random);
    }
  }

  lastT = t;
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
  
  const Cell &cell = this->GetCell(IVector3(x2,y,z));
  bool solid = cell.GetInfo().flags & CellFlags::Solid;
  bool heightCheck = org.y < (y + cell.GetHeight( movingRight?0:0.999f, org.z)) &&
                     org.y > (y + cell.GetHeightBottom( movingRight?0:0.999f, org.z));
//  if (solid && heightCheck) std::cerr << "X" << IVector3(x2,y,z);
  return solid && heightCheck;
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
  size_t z = org.z - (movingForward>0?0.01f:-0.01f); // start cell z

  size_t z2 = (org.z + dir + 2*(movingForward>0?0.01f:-0.01f));
  if (z2 == z) return false; // not crossing cell borders

  const Cell &cell = this->GetCell(IVector3(x, y, z2));
  bool solid = cell.GetInfo().flags & CellFlags::Solid;
  bool heightCheck = org.y < (y+cell.GetHeight( org.x, movingForward?0:0.999f)) &&
                     org.y > (y+cell.GetHeightBottom( org.x, movingForward?0:0.999f));
//  if (solid && heightCheck) std::cerr << "Z" << IVector3(x,y,z2);
  return solid && heightCheck;
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
 * Move a AABB through the world, doing collision detection and response.
 * @param aabb The AABB to move.
 * @param targ The target position where to move the AABB.
 * @param axis Output of the colliding axis flags.
 * @return The final center position of the AABB.
 */
Vector3 World::MoveAABB(
  const AABB &aabb, 
  const Vector3 &targ,
  uint8_t &axis
) {
  // initialize axis to no collisions
  axis = 0;

  Vector3 center = aabb.center;
  Vector3 dist = targ-aabb.center;

  // are we moving at all?
  if (dist.GetSquareMag() == 0.0f) return center;

  bool movingUp      = dist.y > 0;

  // create vertices that serve as origins for ray check. must be at most
  // one cell size apart for correct collision detection.
  std::vector<Vector3> verts;

  // TODO: optimize generation
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,               0, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,               0,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,               0, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,               0,  aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y,  aabb.extents.z));

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

    for (const Vector3 &v : verts) {
      if (d.x && std::signbit(v.x) == std::signbit(d.x) && CastRayX(center + v, d.x)) {
        // collided along x axis, move aabb to collider cell side
        if (d.x > 0) {
          center.x = ((int)center.x + 1) - aabb.extents.x - 0.01f;
        } else if (d.x < 0) {
          center.x = ((int)center.x) + aabb.extents.x + 0.01f;
        }

        axis |= Axis::X; target.x = center.x; d.x = 0;
      }

      if (d.z && std::signbit(v.z) == std::signbit(d.z) && CastRayZ(center + v, d.z)) {
        // collided along z axis, move aabb to collider cell side
        if (d.z > 0) {
          center.z = ((int)center.z + 1) - aabb.extents.z - 0.01f;
        } else if (d.z < 0) {
          center.z = ((int)center.z) + aabb.extents.z + 0.01f;
        }

        axis |= Axis::Z; target.z = center.z; d.z = 0;
      }
      
      // check if nothing left to do for horizontal movement
      if (axis & Axis::X  &&  axis & Axis::Z) break;
    }

    // update center to new position
    center.x += d.x;
    center.z += d.z;

    // check if nothing left to do for horizontal movement
    if (axis & Axis::X  && axis & Axis::Z) break;

    // was this the final pass?
    if (!keepGoing) break;
  }
  
  //if (axis) std::cerr << std::endl;

  if (movingUp) {
    float endY = size.y;
    // get lowest collision point
    for (const Vector3 &v : verts) {
      if (v.y <= 0.0f) continue;
      float vY = CastRayYUp(aabb.center+v);
      if (vY < endY) endY = vY;
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
      float vY = CastRayYDown(aabb.center+v);
      if (vY > endY) endY = vY;
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

void 
World::AddMob(const std::shared_ptr<Mob> &mob) {
  mobs.push_back(mob);
  mob->SetWorld(this);
}

void
World::RemoveMob(const std::shared_ptr<Mob> &mob) {
  auto iter = std::find(mobs.begin(), mobs.end(), mob);
  if (iter == mobs.end()) return;
  (*iter)->SetWorld(nullptr);
  mobs.erase(iter);
}

std::vector<std::shared_ptr<Mob>> 
World::FindMobs(const AABB &aabb) {
  std::vector<std::shared_ptr<Mob>> mobs;
  for (auto mob : this->mobs) {
    if (aabb.Overlap(mob->GetAABB())) mobs.push_back(mob);
  } 
  return mobs;
}

bool 
World::CheckMob(const IVector3 &pos) {
  if (!IsValidCellPosition(pos)) return false;

  AABB aabb;
  aabb.extents = Vector3(0.5, 0.5, 0.5);
  aabb.center = Vector3(pos) + aabb.extents;
  
  for (auto mob : this->mobs) {
    if (aabb.Overlap(mob->GetAABB())) return false;
  } 
  return true;
}

IColor 
World::GetLight(const IVector3 &pos) const {
  return (GetCell(pos).GetLightLevel().Gamma(2.2)) + ambientLight;
}

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
  

Cell &
World::CastRayCell(const Vector3 &org, const Vector3 &dir, float &distance, Side &side) {
  int dx = dir.x == 0 ? 0 : (dir.x > 0 ? 1 : -1);
  int dy = dir.y == 0 ? 0 : (dir.y > 0 ? 1 : -1);
  int dz = dir.z == 0 ? 0 : (dir.z > 0 ? 1 : -1);
  Vector3 pos = org;
  float t = 0.0;

  size_t x = pos.x; // start cell x
  size_t y = pos.y; // start cell y
  size_t z = pos.z; // start cell z
  
  Cell *currentCell = nullptr;

  while (IsValidCellPosition(IVector3(x,y,z))) {
    currentCell = &this->GetCell(IVector3(x,y,z));
    if (!(currentCell->GetInfo().flags & CellFlags::DoNotRender)) {
      distance = t;
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
      t += u;
      side = dx > 0 ? Side::Left : Side::Right;
    } else if (v < u && v < w) {
      pos = pos + dir * v;
      y += dy;
      t += v;
      side = dy > 0 ? Side::Down : Side::Up;
    } else if (w < u && w < v) {
      pos = pos + dir * w;
      z += dz;
      t += w;
      side = dz > 0 ? Side::Backward : Side::Forward;
    } else {
      break;
    }
  }

  if (!currentCell) return defaultCell;
  return *currentCell;
}

bool 
World::IsDefault(const IVector3 &pos) {
  if (!IsValidCellPosition(pos)) return true;
  return defaultMask[GetCellIndex(pos)];
}

void
World::AddFeatureSeen(size_t f) {
  if (f == ~0UL) return;
  if (seenFeatures.size() <= f) seenFeatures.resize(f+1, false);
  seenFeatures[f] = true;
 
  if (instances[f].prevID != ~0UL) seenFeatures[instances[f].prevID] = true;
  for (size_t i=0; i<instances.size(); i++) {
    if (instances[i].prevID == f) {
      if (seenFeatures.size() <= i) seenFeatures.resize(i+1, false);
      seenFeatures[i] = true;
    }
  }
}

