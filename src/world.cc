#include "common.h"

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
#include "image.h"
#include "texture.h"
#include "aabb.h"

const IColor World::ambientLight = IColor(32,32,32);

World::World(RunningState &state, const IVector3 &size) :
  state(state),
  minimap(*this),

  dirty(true),
  firstDirty(true),
  cells(size.x * size.y * size.z, Cell("default")),
  defaultMask(cells.size(), true),
  defaultCell("default"),
  dynamicCells(0),
  allVerts(),
  vertexStartsNormal(),
  vertexCountsNormal(),
  vertexStartsEmissive(),
  vertexCountsEmissive(),
  checkOverwrite(false),
  checkOverwriteOK(true)
{
  this->proto.set_size_x(size.x);
  this->proto.set_size_y(size.y);
  this->proto.set_size_z(size.z);

  // TODO: load default mask
}

World::World(RunningState &state, const World_Proto &proto) :
  state(state),
  proto(proto),
  minimap(*this, proto.mini_map()),

  dirty(true),
  firstDirty(true),
  defaultCell("default"),
  dynamicCells(0),
  allVerts(),
  vertexStartsNormal(),
  vertexCountsNormal(),
  vertexStartsEmissive(),
  vertexCountsEmissive(),
  checkOverwrite(false),
  checkOverwriteOK(true)
{
  this->cells.clear();
  this->defaultMask.clear();
}

World::~World() {
}

Cell &
World::SetCell(const IVector3 &pos, const Cell &cell, bool ignoreLock) {
  if (checkOverwrite) {
    if (!this->IsValidCellPosition(pos)) 
      checkOverwriteOK = false;
    else if (!ignoreLock && this->cells[this->GetCellIndex(pos)].IsProtected())
      checkOverwriteOK = false;
    return this->defaultCell;
  }

  if (!this->IsValidCellPosition(pos)) return this->defaultCell;

  size_t i = this->GetCellIndex(pos);
  if (this->cells[i].IsIgnoringWrite()) return this->defaultCell;

  defaultMask[i] = false;

  size_t featId = this->cells[i].GetFeatureID();

  CellProperties info = this->cells[i].GetInfo();
  this->cells[i] = cell;
  this->cells[i].SetWorld(this, GetCellPos(i));
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

    this->dynamicCells.clear();

    if (firstDirty) {
      // fill up liquids with liquids above, so it won't trickle
      for (size_t i=0; i<this->GetCellCount(); i++) {
        if (this->cells[i].IsLiquid() && this->cells[i][Side::Up].GetInfo() == this->cells[i].GetInfo()) {
          this->cells[i].SetLiquidAmount(16);
        }
      }

      firstDirty = false;
    }

    std::unordered_map<const Texture *, std::vector<Vertex>> verticesNormal;
    std::unordered_map<const Texture *, std::vector<Vertex>> verticesEmissive;

    size_t updateCount = 0;

    for (size_t i=0; i<this->GetCellCount(); i++) {
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
      if (tex) for (auto &v:cell.GetVertices()) verticesNormal[tex].push_back(v);

      const Texture *etex = cell.GetEmissiveTexture();
      if (etex) for (auto &v:cell.GetVertices()) verticesEmissive[etex].push_back(v);
    }

    //if (updateCount) Log("%u cell vertex updates\n", updateCount);

    size_t index = 0;
    this->allVerts.Clear();

    for (auto &iter : verticesNormal) {
      this->vertexStartsNormal[iter.first] = index;
      this->vertexCountsNormal[iter.first] = iter.second.size();
      index += iter.second.size();

      for (auto &v : iter.second) {
        this->allVerts.Add(v);
      }
    }

    for (auto &iter : verticesEmissive) {
      this->vertexStartsEmissive[iter.first] = index;
      this->vertexCountsEmissive[iter.first] = iter.second.size();
      index += iter.second.size();

      for (auto &v : iter.second) {
        this->allVerts.Add(v);
      }
    }

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
      gfx.DrawTriangles(this->allVerts, s.second, this->vertexCountsNormal[s.first]);
    }

    gfx.SetBlendAdd();
    for (auto &s : this->vertexStartsEmissive) {
      gfx.SetTextureFrame(s.first);
      gfx.DrawTriangles(this->allVerts, s.second, this->vertexCountsEmissive[s.first]);
    }
  }

  {
    PROFILE_NAMED("Dynamic Draw");

    // get vertices for dynamic cells
    std::unordered_map<const Texture *, VertexBuffer> dynVerticesNormal;
    std::unordered_map<const Texture *, VertexBuffer> dynVerticesEmissive;

    for (size_t i : dynamicCells) {
      Cell &cell = this->cells[i];
      cell.UpdateVertices();

      const Texture *tex = cell.GetTexture();
      const Texture *etex = cell.GetEmissiveTexture();
      for (auto &v:cell.GetVertices()) {
        if (tex)  dynVerticesNormal[tex].Add(v);
        if (etex) dynVerticesEmissive[etex].Add(v);
      }
    }

    // render vertices for dynamic cells
    gfx.SetBlendNormal();
    auto iter = dynVerticesNormal.begin();
    for (size_t i=0; i<dynVerticesNormal.size(); i++, iter++) {
      gfx.SetTextureFrame(iter->first);
      gfx.DrawTriangles(iter->second);
    }

    gfx.SetBlendAdd();
    gfx.SetLight(IColor(255,255,255));
    iter = dynVerticesEmissive.begin();
    for (size_t i=0; i<dynVerticesEmissive.size(); i++, iter++) {
      gfx.SetTextureFrame(iter->first);
      gfx.DrawTriangles(iter->second);
    }

    //lastDynVertexCount = dynVerticesNormal.size();
    //lastDynVertexEmissiveCount = dynVerticesEmissive.size();
  }
}

void
World::MarkForUpdateNeighbours(const CellBase *cell) {
  this->neighbourUpdates.insert(GetCellIndex(cell->GetPosition()));
}

void
World::MarkForUpdateNeighbours(size_t i) {
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
    while (tickInterval != 0.0 && state.GetGame().GetTime() > this->GetNextTickTime()) {
      for (size_t i : this->dynamicCells) {
        this->cells[i].Tick(state);
      }
      this->SetNextTickTime(this->GetNextTickTime() + tickInterval);
    }
  }
}

/**
 * Cast a ray from a given location along the x axis.
 * @param org Ray origin.
 * @param dir Ray direction.
 * @param sneak If true assume sneaking mob and prevent falling from ledges.
 * @return true if the next adjacent cell in the given direction is solid and
 *         top of cell at that position is above the ray (hitting solid part
 *         of the cell).
 */
bool
World::CastRayX(const Vector3 &org, float dir, bool sneak) const {
  bool movingRight = dir > 0;
  size_t x = org.x - (movingRight?0.01f:-0.01f); // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z; // start cell z

  // end cell x
  size_t x2 = (org.x+dir + 2*(movingRight?0.01f:-0.01f));
  if (x2 == x) return false; // not crossing cell borders

  return this->GetCell(IVector3(x,y,z)).CheckSideSolid(movingRight ? Side::Right : Side::Left, org, sneak);
}

/**
 * Cast a ray from a given location along the z axis.
 * @param org Ray origin.
 * @param dir Ray direction.
 * @param sneak If true assume sneaking mob and prevent falling from ledges.
 * @return true if the next adjacent cell in the given direction is solid and
 *         top of cell at that position is above the ray (hitting solid part
 *         of the cell).
 */
bool
World::CastRayZ(const Vector3 &org, float dir, bool sneak) const {
  bool movingForward = dir > 0;
  size_t x = org.x; // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z - (movingForward ? 0.01f : -0.01f); // start cell z

  size_t z2 = (org.z + dir + 2*(movingForward ? 0.01f : -0.01f));
  if (z2 == z) return false; // not crossing cell borders

  return this->GetCell(IVector3(x,y,z)).CheckSideSolid(movingForward?Side::Forward:Side::Backward, org, sneak);
}

/**
 * Cast a ray from a given location along the y axis up.
 * @param org Ray origin.
 * @return The end position of the ray where it hits a solid cell. Takes into
 *         account the bottom y-offsets of the cell.
 */
float
World::CastRayYUp(const Vector3 &org) const {
  //Log("CastRayYUP: %f %f %f\n", org.x, org.y, org.z);

  if (org.y < 0) return 0;

  size_t x = org.x; // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z; // start cell z

  //if (GetCell(IVector3(x,y,z)).IsSolid() && y > GetCell(IVector3(x,y,z)).GetHeightBottom(org.x, org.z)) return y;

  while (y < this->proto.size_y()) {
    const Cell &cell = this->GetCell(IVector3(x,y,z));
    if (cell.IsSolid()) {
      return y + cell.GetHeightBottom(org.x, org.z);
    }
    y++;
  }
  return this->proto.size_y();
}

/**
 * Cast a ray from a given location along the y axis down.
 * @param org Ray origin.
 * @return The end position of the ray where it hits a solid cell. Takes into
 *         account the top y-offsets of the cell.
 */
float
World::CastRayYDown(const Vector3 &org) const {
  size_t x = org.x; // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z; // start cell z

  if (y >= this->proto.size_y()) y = this->proto.size_y() - 1;

  Cell *cell = &GetCell(IVector3(x,y,z));
  while(!cell->IsSolid()) {
    cell = &((*cell)[Side::Down]);
    y--;
  }

  return cell->GetHeight(org.x, org.z) + y;
}

/**
 * Check if a point is within the solid of a cell.
 * @param org Point to check
 * @return true if point is inside a solid.
 */
bool
World::IsPointSolid(const Vector3 &org) const {
  size_t x = org.x; // start cell x
  size_t y = org.y; // start cell y
  size_t z = org.z; // start cell z

  const Cell &cell = this->GetCell(IVector3(x,y,z));
  if (!cell.IsSolid()) return false;

  float cellY = org.y - y;
  //Log("%f %f %f %s\n", cellY, cell.GetHeight(org.x, org.z), cell.GetHeightBottom(org.x, org.z), cell.GetType().c_str());
  if (cellY > cell.GetHeight(org.x, org.z)) return false;
  if (cellY < cell.GetHeightBottom(org.x, org.z)) return false;
  return true;
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
World::IsAABBSolid(const AABB &aabb) const {
  std::vector<Vector3> verts;
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y,  aabb.extents.z));

  for (auto &v : verts) {
    if (IsPointSolid(aabb.center + v)) return true;
  }

  float top = aabb.center.y + aabb.extents.y + 0.01;
  //Log("%f < %f? (from %f+%f)\n", CastRayYUp(verts[0]), top, verts[0].y, aabb.center.y);
  if (CastRayYUp(verts[0]+aabb.center) < top) return true;
  if (CastRayYUp(verts[1]+aabb.center) < top) return true;
  if (CastRayYUp(verts[2]+aabb.center) < top) return true;
  if (CastRayYUp(verts[3]+aabb.center) < top) return true;
  //Log("meh..\n");

  float bot = aabb.center.y - aabb.extents.y - 0.01;
  if (CastRayYDown(verts[4]+aabb.center) > bot) return true;
  if (CastRayYDown(verts[5]+aabb.center) > bot) return true;
  if (CastRayYDown(verts[6]+aabb.center) > bot) return true;
  if (CastRayYDown(verts[7]+aabb.center) > bot) return true;
  //Log("muh..\n");

  return false;
}

/**
 * Move a AABB through the world, doing collision detection and response.
 * @param aabb The AABB to move.
 * @param targ The target position where to move the AABB.
 * @param axis Output of the colliding axis flags.
 * @param[out] cell Returns one of the cell with which the aabb collided.
 * @param[out] side Returns the side of the cell that collided.
 * @param sneak If true, assume sneaking mob and prevent falling from ledges.
 * @return The final center position of the AABB.
 * @note Might not work for too big AABBs.
 */
Vector3 World::MoveAABB(
  const AABB &aabb,
  const Vector3 &targ,
  uint8_t &axis,
  Cell **cell,
  Side *side,
  bool sneak
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
      d = d.Normalize();
      keepGoing = true;
    }

    // try to move along the x axis
    Vector3 ddx(d.x + (d.x>0?0.01:-0.01), 0, 0);

    for (const Vector3 &v : verts) {
      if (CastRayX(center + v, d.x, sneak)/* || IsPointSolid(center + v + ddx)*/) {
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
        break;
      }
    }

    // update center to new position
    center.x += d.x;

    // try to move along the z axis
    Vector3 ddz(0, 0, d.z + (d.z>0?0.01:-0.01));

    for (const Vector3 &v : verts) {
      if (CastRayZ(center + v, d.z, sneak) || IsPointSolid(center + v + ddz)) {
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
        break;
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
    float endY = this->proto.size_y();
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
  const Vector3 &target,
  bool sneak
) {
  uint8_t axis;
  return this->MoveAABB(aabb, target, axis, nullptr, nullptr, sneak);
}

IColor
World::GetLight(const IVector3 &pos) const {
  return (GetCell(pos).GetLightLevel()) + ambientLight;
}

IColor
World::GetLight(const Vector3 &pos) const {
  IVector3 ipos(pos.x - 1.0, pos.y - 1.0, pos.z - 1.0);

  IColor c_000 = GetLight(IVector3(ipos.x,   ipos.y,   ipos.z));
  IColor c_100 = GetLight(IVector3(ipos.x+1, ipos.y,   ipos.z));
  IColor c_010 = GetLight(IVector3(ipos.x,   ipos.y,   ipos.z));
  IColor c_110 = GetLight(IVector3(ipos.x+1, ipos.y+1, ipos.z));
  IColor c_001 = GetLight(IVector3(ipos.x,   ipos.y+1, ipos.z+1));
  IColor c_101 = GetLight(IVector3(ipos.x+1, ipos.y,   ipos.z+1));
  IColor c_011 = GetLight(IVector3(ipos.x,   ipos.y+1, ipos.z+1));
  IColor c_111 = GetLight(IVector3(ipos.x+1, ipos.y+1, ipos.z+1));

  float dx = pos.x - 0.5 - ipos.x;

  IColor c_00  = IColor::Lerp(c_000, c_100, dx);
  IColor c_10  = IColor::Lerp(c_010, c_110, dx);
  IColor c_01  = IColor::Lerp(c_001, c_101, dx);
  IColor c_11  = IColor::Lerp(c_011, c_111, dx);

  float dy = pos.y - 0.5 - ipos.y;

  IColor c_0   = IColor::Lerp(c_00,  c_10,  dy);
  IColor c_1   = IColor::Lerp(c_01,  c_11,  dy);

  float dz = pos.z - 0.5 - ipos.z;
  return         IColor::Lerp(c_0,   c_1,   dz);
}

/**
 * Write some ASCII art representation of this world to a file. (DEBUG)
 */
void
World::Dump() {
  FILE *f = fopen("world.txt", "w");
  for (size_t z = 0; z<this->proto.size_z(); z++) {
    for (size_t x = 0; x<this->proto.size_x(); x++) {
      int air = 0;
      for (size_t y = 0; y<this->proto.size_y(); y++) {
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
World::CastRayCell(const Vector3 &org, const Vector3 &dir, float &distance, Side &side, size_t flags) const {
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
World::IsDefault(const IVector3 &pos) const {
  if (!IsValidCellPosition(pos)) return true;
  return defaultMask[GetCellIndex(pos)];
}

void
World::ClearDefaults() {
  this->defaultMask = std::vector<bool>(this->GetCellCount(), true);
}

void
World::CopyCellsFrom(const std::vector<Cell> &cells) {
  this->cells = cells;
  for (size_t i=0; i<this->GetCellCount(); i++) {
    this->cells[i].SetWorld(this, GetCellPos(i));
  }
  this->ClearDefaults();
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
World::IsCellValidTeleportTarget(const IVector3 &pos, const Vector3 &extents) const {
  AABB aabb;
  aabb.center = Vector3(pos.x+0.5, pos.y + 1.01 + extents.y, pos.z + 0.5);
  aabb.extents = extents;

  Cell &cell = GetCell(pos);

  return
    !cell.IsTrigger() &&
    !cell.IsTeleport() &&
    IsCellWalkable(pos) &&
    IsCellWalkable(pos[Side::Right]) &&
    IsCellWalkable(pos[Side::Left]) &&
    IsCellWalkable(pos[Side::Forward]) &&
    IsCellWalkable(pos[Side::Backward]) &&
    !IsAABBSolid(aabb)
  ;
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
World::GetRandomTeleportTarget(Random &random, const Vector3 &extents) const {
  IVector3 pos;
  do {
    pos.x = random.Integer(this->proto.size_x());
    pos.y = random.Integer(this->proto.size_y());
    pos.z = random.Integer(this->proto.size_z());
    pos = FindSolidBelow(pos);
  } while(!IsCellValidTeleportTarget(pos, extents));
  return pos;
}

IVector3
World::GetRandomCeiling(Random &random) const {
  IVector3 pos;
  do {
    pos.x = random.Integer(this->proto.size_x());
    pos.y = random.Integer(this->proto.size_y());
    pos.z = random.Integer(this->proto.size_z());
    pos = FindSolidAbove(pos);
  } while(!IsCellValidCeiling(pos));
  return pos;
}

void World::TriggerOn(size_t id) {
  for (size_t i=0; i<this->GetCellCount(); i++) {
    if (this->cells[i].GetTriggerId() == id) this->cells[i].TriggerOn();
  }
}

void World::TriggerOff(size_t id) {
  for (size_t i=0; i<this->GetCellCount(); i++) {
    if (this->cells[i].GetTriggerId() == id) this->cells[i].TriggerOff();
  }
}

const World_Proto &
World::GetProto() {
  *this->proto.mutable_mini_map() = this->minimap.GetProto();

  // TODO: save default mask

  return this->proto;
}

/*

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


*/


// ----------------------------

MiniMap::MiniMap(const World &world) :
  world(world),
  seenFeatures(0),
  mapTexture(nullptr),
  viewY(0)
{
}

MiniMap::MiniMap(const World &world, const MiniMap_Proto &proto) :
  world(world),
  proto(proto),
  mapTexture(nullptr),
  viewY(0)
{
  // TODO: load seen features
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

  gfx.SetTextureFrame(Texture::Get("gui/maparrow"));
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
  uint8_t *pixels = new uint8_t[size.x*size.z*4];

  for (size_t x=0; x<size.x; x++) {
    for (size_t z=0; z<size.z; z++) {
      size_t index = (x+(size.z-1-z)*size.x)*4;
      pixels[index+3] = 0;

      for (size_t yy=viewY; yy>0; yy--) {
        IVector3 pos(x,yy,z);
        Cell &cell = this->world.GetCell(pos);
        if (!cell.IsSeen(2)) continue;

        bool solid = !cell.IsTransparent() && !cell[Side::Up].IsTransparent() && !cell[Side::Down].IsTransparent();
        uint8_t v = yy - viewY + 192;
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

  this->mapTexture = Texture::Create("*minimap", Image(Point(size.x, size.z), pixels, true));
}


const MiniMap_Proto &
MiniMap::GetProto() {
  // TODO: save seen features
  return this->proto;
}
