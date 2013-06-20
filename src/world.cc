#include "GLee.h"

#include "world.h"
#include "cell.h"
#include "util.h"
#include "mob.h"
#include "player.h"
#include "simplex.h"

#include "random.h"
#include "serializer.h"
#include "worldedit.h"

#include "shader.h"

World::World(const IVector3 &size, int level, Random &rnd) :random(rnd)
{  
  this->defaultShader = new Shader("default");
  this->ambientLight = IColor(16,16,32);
  this->checkOverwrite = false;
  this->size = size;
  this->dirty = true;
  this->firstDirty = true;
  this->level = level;

  this->cellCount = size.x * size.y * size.z;
  this->cells = std::vector<Cell>(this->cellCount, Cell("default"));
  this->defaultMask = std::vector<bool>(this->cellCount, true);
  
  std::cerr << this->cellCount << " cells, " << (sizeof(Cell)*this->cellCount) << std::endl;
    
  this->lastT = 0;
  this->tickInterval = 0.3;
  this->nextTickT = 0;

  // build features -------------------------------------------
  
  // some basic parameters for this world
  IVector3 r(random.Integer(), random.Integer(), random.Integer());
  size_t featureCount  = random.Integer(400)+100;             // 100 - 500
  float  useLastChance = 0.1 + random.Float01()*0.8;          // 0.1 - 0.9
  size_t caveCount     = random.Integer(30)+5;                //   5 -  35
  size_t caveLengthMin = random.Integer(100);                 //   0 - 100
  size_t caveLengthMax = caveLengthMin + random.Integer(100); //   0 - 200
  size_t caveRepeat    = 10+random.Integer(20);               //  10 -  30

  // try 10 times to build a world with at least 50 features
  for (size_t tries=0; tries < 10 && instances.size() < 50; tries++) {
    
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
  
    instances.clear();
    instances.push_back(getFeature("start")->BuildFeature(this, IVector3(32-4, 32-8,32-4), 0, 0, instances.size()));
    instances.back().prevID = ~0UL;
  
    int loop = 0;
    do {
      if (loop++ > 100000) break;

      // select a feature from which to go
      bool useLast = random.Chance(useLastChance);
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
      }
    } while(instances.size() < featureCount); 
  }

  std::cerr << "built world with " << instances.size() << " features. level " << (level) << std::endl;
 
  WorldEdit e(this);
  
  // create caves
  e.SetBrush(Cell("air"));

  for (size_t i = 0; i<caveCount; i++) {
    size_t featNum = random.Integer(instances.size());
    const FeatureInstance &instance = instances[featNum];
    IVector3 size = instance.feature->GetSize();
   
    for (size_t k=0; k<caveRepeat; k++) {
      IVector3 cavePos(instance.pos + IVector3(random.Integer(size.x), random.Integer(size.y), random.Integer(size.z)));
      size_t caveLength = caveLengthMin + random.Integer(caveLengthMax);
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

  for (auto instance : instances) {
    instance.feature->SpawnEntities(this, instance.pos);
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
  
  this->defaultShader->Bind();
  this->defaultShader->Uniform("u_texture", 0);
  this->defaultShader->Uniform("u_torch", player->GetTorchLight());

  // draw each vertex buffer 
  auto iter = vertices.begin();
  for (size_t i=0; i<this->vertices.size(); i++, iter++) {
    glBindBuffer       (GL_ARRAY_BUFFER, this->vbos[i]);
    //glBindBuffer       (GL_ARRAY_BUFFER, 0);
    glBindTexture      (GL_TEXTURE_2D,   iter->first);
    glInterleavedArrays(GL_T2F_C4F_N3F_V3F,  sizeof(Vertex), nullptr);
    //glInterleavedArrays(GL_T2F_C4F_N3F_V3F,  sizeof(Vertex), &iter->second[0]);
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
    glInterleavedArrays(GL_T2F_C4F_N3F_V3F, sizeof(Vertex), &iter->second[0]);
    glDrawArrays       (GL_TRIANGLES,   0, iter->second.size());
  }

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  Shader::Unbind();

  // draw all entities
  for (auto entity : this->entities) {
    entity->Draw();
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
  glInterleavedArrays(GL_T2F_C4F_N3F_V3F, sizeof(Vertex), &verts[0]);
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

  // handle collision between entities
  for (auto entity1 : this->entities) {
    for (auto entity2 : this->entities) {
      if (entity1 == entity2) continue;
      if (entity1->GetAABB().Overlap(entity2->GetAABB())) {
        entity1->OnCollide(entity2);
        entity2->OnCollide(entity1);
      }
    }
  }

  // update all entities
  for (auto entity : this->entities) {
    entity->Update(t);
  }

  // remove removable entities
  auto entityIter = this->entities.begin();
  while(entityIter != this->entities.end()) {
    if ((*entityIter)->IsRemovable()) {
      entityIter = this->entities.erase(entityIter);
    } else {
      entityIter++;
    }
  }
  
  // update all dynamic cells
  for (size_t i : this->dynamicCells) {
    this->cells[i].Update(t, random);
  }

  // tick world
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
  bool heightCheck = org.y < (y + cell.GetHeight( movingRight?0:0.999f, org.z))/* &&
                     org.y > (y + cell.GetHeightBottom( movingRight?0:0.999f, org.z)) */;
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
  bool heightCheck = org.y < (y+cell.GetHeight( org.x, movingForward?0:0.999f)) /*&&
                     org.y > (y+cell.GetHeightBottom( org.x, movingForward?0:0.999f)) */;
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
  return cell.IsSolid() && (org.y - y) >= cell.GetHeightBottom(org.x, org.z) && (org.y - y) <= cell.GetHeight(org.x, org.z);
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
 * @return The final center position of the AABB.
 * @note Might not work for too big AABBs.
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

  // TODO: automate/optimize generation for a given aabb size
  // TODO: also cache these
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y,  aabb.extents.z));

  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y/2, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y/2,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y/2, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y/2,  aabb.extents.z));

  verts.push_back(Vector3(-aabb.extents.x,               0, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,               0,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,               0, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,               0,  aabb.extents.z));

  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y/2, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y/2,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y/2, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y/2,  aabb.extents.z));

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
    
    // try to move along the x axis
    for (const Vector3 &v : verts) {
      if (IsPointSolid(center + v + Vector3(d.x + (d.x>0?0.01:-0.01), 0, 0))) {
        float newX = d.x;
        if (d.x > 0) {
          newX = ((int)center.x + 1) - aabb.extents.x - 0.01f;
        } else if (d.x < 0) {
          newX = ((int)center.x) + aabb.extents.x + 0.01f;
        }

        if (std::abs(newX - center.x) < std::abs(d.x))        
          d.x = newX - center.x;
          
        axis |= Axis::X; 
      }
    }

    // update center to new position
    center.x += d.x;
    
    // try to move along the z axis
    for (const Vector3 &v : verts) {
      if (IsPointSolid(center + v + Vector3(0, 0, d.z + (d.z>0?0.01:-0.01)))) {
        float newZ = d.z;
        if (d.z > 0) {
          newZ = ((int)center.z + 1) - aabb.extents.z - 0.01f;
        } else if (d.z < 0) {
          newZ = ((int)center.z) + aabb.extents.z + 0.01f;
        }

        if (std::abs(newZ - center.z) < std::abs(d.z))
          d.z = newZ - center.z;
          
        axis |= Axis::Z; 
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

/**
 * Add an entity to this world.
 * @param entity Entity to add
 */
void 
World::AddEntity(const std::shared_ptr<Entity> &entity) {
  this->entities.push_back(entity);
  entity->SetWorld(this);
}

/**
 * Add the player entity to this world. 
 * Also stores the player for future reference.
 * @param player Player to add
 */
void 
World::AddPlayer(const std::shared_ptr<Player> &player) {
  this->AddEntity(player);
  this->player = player;
}

/**
 * Remove an entity from this world.
 * If the entity is the player entity, the player reference will be unset.
 * @param entity Entity to remove
 */
void
World::RemoveEntity(const std::shared_ptr<Entity> &entity) {
  auto iter = std::find(this->entities.begin(), this->entities.end(), entity);
  if (iter == this->entities.end()) {
    return;
  }
  
  (*iter)->SetWorld(nullptr);
  this->entities.erase(iter);
  
  if (this->player == entity) {
    this->player = nullptr;
  }
}

/** 
 * Find entities within an AABB.
 * The entities' AABBs have to intersesct the given AABB to be returned.
 * @param aabb AABB within which to look for entities
 * @return A vector of entities.
 */
std::vector<std::shared_ptr<Entity>> 
World::FindEntities(const AABB &aabb) {
  std::vector<std::shared_ptr<Entity>> entities;
  
  for (auto entity : this->entities) {
    if (aabb.Overlap(entity->GetAABB())) {
      entities.push_back(entity);
    }
  } 
  
  return entities;
}

/**
 * Check if a cell is free of entities.
 * @param pos Cell position to check
 * @return true if no entities are inside the cell.
 */
bool 
World::CheckMob(const IVector3 &pos) {
  if (!IsValidCellPosition(pos)) return false;

  AABB aabb;
  aabb.extents = Vector3(0.5, 0.5, 0.5);
  aabb.center = Vector3(pos) + aabb.extents;
  
  for (auto entity : this->entities) {
    if (aabb.Overlap(entity->GetAABB())) return false;
  }
  
  return true;
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
 * @return The hit cell.
 * @note This will always return a cell. Outside the world or on the boundaries
 *       it will be the default cell, which has no world or position value.
 */
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

/**
 * Mark a feature as seen (e.g. for minimap).
 * @param f Feature id.
 */
void
World::AddFeatureSeen(size_t f) {
  if (f == ~0UL) return;
  
  // make sure vector is large enough
  if (seenFeatures.size() <= f) {
    seenFeatures.resize(f+1, false);
  }
  
  seenFeatures[f] = true;
 
  // mark neighbouring features as seen as well
  if (instances[f].prevID != ~0UL) {
    seenFeatures[instances[f].prevID] = true;
  }
  for (size_t i=0; i<instances.size(); i++) {
    if (instances[i].prevID == f) {
      if (seenFeatures.size() <= i) seenFeatures.resize(i+1, false);
      seenFeatures[i] = true;
    }
  }
}

