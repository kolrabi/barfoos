#include "GLee.h"

#include "world.h"
#include "cell.h"
#include "mob.h"
#include "simplex.h"

#include "random.h"
#include "worldedit.h"

#include "runningstate.h"
#include "feature.h"
#include "gfx.h"

#include "vertex.h"
#include "texture.h"

#include "serializer.h"
#include "deserializer.h"

World::World(RunningState &state, const IVector3 &size) : 
  state(state),
  size(size),
  dirty(true),
  firstDirty(true),
  cellCount(size.x * size.y * size.z),
  cells(cellCount, Cell("default")),
  defaultCell("default"),
  defaultMask(cellCount, true),
  dynamicCells(0),
  nextTickT(0.0),
  tickInterval(0.01),
  ambientLight(32, 32, 64),
  allVerts(0),
  vertexStarts(),
  vertexCounts(),
  vbo(0),
  seenFeatures(0),
  checkOverwrite(false),
  checkOverwriteOK(true)
{  
  glGenBuffers(1, &this->vbo);
}

World::World(RunningState &state, Deserializer &deser) : 
  state(state),
  dirty(true),
  firstDirty(true),
  defaultCell("default"),
  dynamicCells(0),
  allVerts(0),
  vertexStarts(),
  vertexCounts(),
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
  deser >> this->seenFeatures;
  
  for (size_t i = 0; i<cellCount; i++) {
    this->cells[i].SetWorld(this, GetCellPos(i));
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
  size_t caveLengthMin = random.Integer(20);                  //   0 -  20
  size_t caveLengthMax = caveLengthMin + random.Integer(100); //   0 - 200
  size_t caveRepeat    = random.Integer(20)+1;                //   1 -  21

  std::vector<FeatureInstance> instances;
  
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
        float f = (simplexNoise(vpos*0.07) * simplexNoise(vpos*(-0.06)) * simplexNoise(vpos*(-0.13)));
        if (f > 0.75) {
          this->SetCell(pos, Cell("brick"));
        } else if (f > 0.5) {
          this->SetCell(pos, Cell("rock"));
        } else {
          this->SetCell(pos, Cell("dirt"));
        }
      }
      this->cells[i].SetWorld(this, pos);
    }

    // reinitialize, as it is modified by above SetCell  
    this->defaultMask = std::vector<bool>(this->cellCount, true);
  
    instances.clear();
    instances.push_back(getFeature("start")->BuildFeature(state, *this, IVector3(32-4, 32-8,32-4), 0, 0, instances.size(), nullptr));
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
      const FeatureConnection *conn = feature->GetRandomConnection(state);
      if (!conn) continue;

      // select next feature
      const Feature *nextFeature = conn->GetRandomFeature(state, instance.pos);
      if (!nextFeature) continue;
    
      // make sure both features can connect
      const FeatureConnection *revConn = nextFeature->GetRandomConnection(-conn->dir, state);
      if (!revConn) continue;

      // snap both connection points together
      IVector3 pos = instance.pos + conn->pos - revConn->pos;
    
      // build the next feature if possible
      this->BeginCheckOverwrite();
      nextFeature->BuildFeature(state, *this, pos, conn->dir, instance.dist, instances.size(), nullptr);
      if (this->FinishCheckOverwrite()) {
        FeatureInstance nextInstance = nextFeature->BuildFeature(state, *this, pos, conn->dir, instance.dist, instances.size(), revConn);
        feature->ReplaceChars(state, *this, instance.pos, conn->id, featNum);

        // check if we accidentally connected properly to anything else
        for (const FeatureConnection &nextConn : nextInstance.feature->GetConnections()) {
          IVector3 nextConnPos = nextInstance.pos + nextConn.pos;
          int connDir = nextConn.dir;
          
          for (auto &inst : instances) {
            for (const FeatureConnection &c : inst.feature->GetConnections()) {
              // match direction
              if (c.dir != -connDir) continue;
              
              // match position
              IVector3 connPos = inst.pos + c.pos;
              if (connPos != nextConnPos) continue;
              
              // do connect replacement of cells
              inst.feature->ReplaceChars(state, *this, inst.pos, c.id, inst.featureID);
              nextInstance.feature->ReplaceChars(state, *this, nextInstance.pos, nextConn.id, nextInstance.featureID);
            }
          }
        }

        instances.push_back(nextInstance);
        instances.back().prevID = featNum;
      }
    } while(instances.size() < featureCount); 
  }

  Log("Built world with %lu features. Level %lu.\n", instances.size(), state.GetLevel());
  /*
  for (size_t i=0; i<this->cellCount; i++) {
    IVector3 pos = GetCellPos(i);

    IVector3 ppp(pos+r);
    ppp.x %= 256;
    ppp.y %= 256;
    ppp.z %= 256;
    Vector3 vpos(ppp);
    vpos.y *= 2;
    float f = (simplexNoise(vpos*0.01) * simplexNoise(vpos*0.02) * simplexNoise(vpos*0.04));
    if (f < -0.2 && cells[i].GetFeatureID() > 10) {
      SetCell(pos, Cell("air"));
    }
  } */ 
 
  WorldEdit e(this);
  
  // create caves
  e.SetBrush(Cell("air"));
  size_t caveCount     = 1; //instances.size()>10 ? random.Integer(instances.size()/10) : 0;

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
    instance.feature->SpawnEntities(state, instance.pos);
  }

  this->Dump();
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
  this->cells[i].UpdateNeighbours();

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
World::Draw(Gfx &gfx) {
  PROFILE();
  
  if (dirty) {
    PROFILE();
    // world has been changed, recreate vertex buffers

    this->defaultCell = Cell("default");

    this->allVerts.clear();
    this->dynamicCells.clear();

    if (firstDirty) {
      for (size_t i=0; i<this->cellCount; i++) {
        this->cells[i].UpdateNeighbours();
      }

      for (size_t i=0; i<this->cellCount; i++) {
        if (this->cells[i].IsLiquid() && this->cells[i][Side::Up].GetInfo() == this->cells[i].GetInfo()) {
          this->cells[i].SetDetail(16);
        } 
      }
      firstDirty = false;
    }

    std::unordered_map<const Texture *, std::vector<Vertex>> vertices;

    for (size_t i=0; i<this->cellCount; i++) {
      Cell &cell = this->cells[i];
      const CellProperties &info = cell.GetInfo();

      // don't bother with invisible cells 
      if (info.flags & CellFlags::DoNotRender || !cell.GetVisibility()) continue;

      // don't add dynamic cells to static vertex buffer
      if (info.flags & CellFlags::Dynamic) {
        dynamicCells.push_back(i);
        continue;
      }

      cell.UpdateVertices();

      // group vertex buffers by texture
      
      const Texture *tex = cell.GetTexture();
      cell.Draw(vertices[tex]);
    }
    
    size_t index = 0;
    this->allVerts.clear();
    for (auto &iter : vertices) {
      this->vertexStarts[iter.first] = index;
      this->vertexCounts[iter.first] = iter.second.size();
      index += iter.second.size();

      for (auto &v : iter.second) {
        this->allVerts.push_back(v);
      }
    }

    // set vertex buffer data
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*(this->allVerts.size()), &this->allVerts[0], GL_STATIC_DRAW);

    dirty = false;
  }

  gfx.SetShader("default");
  gfx.SetColor(IColor(255,255,255));
  
  for (auto &s : this->vertexStarts) {
    gfx.SetTextureFrame(s.first);
    gfx.DrawTriangles(this->vbo, s.second, this->vertexCounts[s.first]);
  }

  // get vertices for dynamic cells
  std::unordered_map<const Texture *, std::vector<Vertex>> dynvertices;
  for (size_t i : dynamicCells) {
    const Texture *tex = this->cells[i].GetTexture();
    if (dynvertices.find(tex) == dynvertices.end()) 
        dynvertices[tex] = std::vector<Vertex>();
    this->cells[i].UpdateVertices();
    this->cells[i].Draw(dynvertices[tex]);
  }

  // render vertices for dynamic cells
  auto iter = dynvertices.begin();
  for (size_t i=0; i<dynvertices.size(); i++, iter++) {
    gfx.SetTextureFrame(reinterpret_cast<const Texture *>(iter->first));
    gfx.DrawTriangles(iter->second);
  }
}

void
World::DrawMap(
  Gfx &gfx
) {
  PROFILE();
/*  std::vector<Vertex> verts;

  uint8_t types[size.x*size.z];

  for (size_t x=0; x<size.x; x++) {
    for (size_t z=0; z<size.z; z++) {
      bool unSolid = false;
      size_t lastId = ~0UL;
      types[x+z*size.x] = 0;
      for (size_t y=0; y<size.y; y++) {
        IVector3 pos(x,y,z);
        Cell &cell = this->GetCell(pos);

        bool seen = cell.IsSeen() || 
                    cell[Side::Right].IsSeen() ||
                    cell[Side::Left].IsSeen() ||
                    cell[Side::Forward].IsSeen() ||
                    cell[Side::Backward].IsSeen();
        if (!seen) continue;

        if (cell.GetFeatureID() != lastId && cell.GetFeatureID() != ~0UL) {
          lastId = cell.GetFeatureID();
          unSolid = false;
        }

        bool solid = !cell.IsTransparent();

        if (!solid) {
          unSolid = true;
        }
 
        if (unSolid) {
          types[x+z*size.x] = 2;
          continue;
        }
        types[x+z*size.x] = 1;
      }
    }
  }
        
  Vector3 vsize(1,0,1);
  for (size_t x=0; x<size.x; x++) {
    for (size_t z=0; z<size.z; z++) {
      IColor color;
      switch(types[x+z*size.x]) {
        case 0: continue;
        case 1: color = IColor(192,192,192); break;
        case 2: color = IColor(128,128,128); break;
      }

      Vector3 vpos(x,0,z);
    
      verts.push_back(Vertex(vpos,                            color, 0, 0));
      verts.push_back(Vertex(vpos+Vector3(      0,0,vsize.z), color, 0, 1));
      verts.push_back(Vertex(vpos+Vector3(vsize.x,0,vsize.z), color, 1, 1));
      verts.push_back(Vertex(vpos+Vector3(vsize.x,0,      0), color, 1, 0));
    }
  }
*/
  //gfx.SetTextureFrame(loadTexture("gui/white"));
  //gfx.DrawQuads(verts);
  (void)gfx;
}

void 
World::Update(
  RunningState &state
) {
  PROFILE();
  
  // update all dynamic cells
  for (size_t i : this->dynamicCells) {
    this->cells[i].Update(state);
  }

  // tick world
  while (tickInterval != 0.0 && state.GetGame().GetTime() > nextTickT) {
    for (size_t i : this->dynamicCells) {
      this->cells[i].Tick(state);
    }
    nextTickT += tickInterval;
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

/*  
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
  */
}
  
bool 
World::IsFeatureSeen(size_t id) const {
  if (id == ~0UL || id >= seenFeatures.size()) return false;
  return seenFeatures[id];
}

void
World::BreakBlock(const IVector3 &pos) {
  if (this->GetCell(pos).GetInfo().type == "air") return;
  
  std::string particleType = this->GetCell(pos).GetInfo().breakParticle;
  AABB aabb = this->SetCell(pos, Cell("air")).GetAABB();
  
  if (particleType != "") {
    Random &random = state.GetRandom();
    for (size_t i=0; i<4; i++) {
      state.SpawnMobInAABB(particleType, aabb, random.Vector()*10);
    }
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
    IsCellWalkable(pos[Side::Backward]));
}

IVector3 
World::FindSolidBelow(const IVector3 &pos) const {
  IVector3 p(pos);
  while(!GetCell(p).IsSolid()) 
    p = p[Side::Down];

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

Serializer &operator << (Serializer &ser, const World &world) {
  ser << world.size;
  
  for (auto &c:world.cells)
    ser << c;
    
  ser << world.defaultMask;
  ser << world.nextTickT;
  ser << world.tickInterval;
  ser << world.ambientLight;
  ser << world.seenFeatures;
  return ser;
}
