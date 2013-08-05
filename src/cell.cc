#include "cell.h"
#include "util.h"
#include "world.h"
#include "gfx.h"
#include "runningstate.h"
#include "mob.h"

#include "random.h"
#include "vertex.h"

#include "texture.h"
 
#include "serializer.h"
#include "deserializer.h"
 
// -------------------------------------------------------------------------

CellBase::CellBase(const std::string &type) :
  info(&GetCellProperties(type)),
  world(nullptr),
  pos(0,0,0),
  dirty(true),
  lastT(0.0),
  shared(info)
{
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;
}

// -------------------------------------------------------------------------

CellRender::CellRender(const std::string &type) :
  CellBase(type),
  visibility(0),
  reversedSides(false),
  verts(0),
  texture(nullptr),
  uscale(1.0)
{
}

// -------------------------------------------------------------------------

Cell::Cell(const std::string &type) : 
  CellRender(type),
  tickPhase(0),
  lightLevel(0,0,0),
  lastUseT(0.0)
{
  // unique information
  this->SetYOffsets(1,1,1,1);
  this->SetYOffsetsBottom(0,0,0,0);
}

Cell::Cell(const Cell &that) : 
  CellRender(that.info->type),
  
  tickPhase(0),
  lightLevel(0,0,0),
  
  lastUseT(0.0)
{
  shared = that.shared;
}

Cell &
Cell::operator =(const Cell &that)
{
  this->shared = that.shared;
  
  // unique information
  this->info = that.info;
  this->world = nullptr;
  this->tickPhase = 0;
  this->visibility = 0;
  this->reversedSides = false;
  
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;

  this->texture = that.texture;
  this->uscale = that.uscale;
  
  this->lastUseT = 0.0;
  this->lastT = 0.0;

  this->dirty = true;
  
  return self;
}

Cell::~Cell() {
}

void 
CellRender::Draw(std::vector<Vertex> &verts) const {
  if (info->flags & CellFlags::DoNotRender || visibility == 0) return;
  
  for (const Vertex &v : this->verts) {
    verts.push_back(v);
  }
}

void 
CellRender::DrawHighlight(std::vector<Vertex> &verts) const {
  if (info->flags & CellFlags::DoNotRender || visibility == 0) return;
  
  for (const Vertex &v : this->verts) {
    Vertex vv(v);
    vv.xyz[0] = vv.xyz[0] + 0.01 * vv.n[0];
    vv.xyz[1] = vv.xyz[1] + 0.01 * vv.n[1];
    vv.xyz[2] = vv.xyz[2] + 0.01 * vv.n[2];
    vv.uv[0] /= uscale;
    vv.rgb[0] = 1.0;
    vv.rgb[1] = 1.0;
    vv.rgb[2] = 1.0;
    vv.rgb[3] = 1.0;
    verts.push_back(vv);
  }
}

void
Cell::Update(
  RunningState &state
) {
  if (!world) return; 
 
  float deltaT = state.GetGame().GetDeltaT();
  this->lastT = state.GetGame().GetTime();

  this->shared.smoothDetail = this->shared.smoothDetail + (this->shared.detail - this->shared.smoothDetail) * deltaT * 2;

  // if this cell lost all its liquid replace by air
  if (this->IsLiquid() && this->shared.smoothDetail < 0.1) {
    this->world->SetCell(this->pos, Cell("air"));
    // "this" is now the new air cell
  }

  if (this->IsLiquid()) {
    float h = this->shared.smoothDetail/16.0;

    if (self[Side::Up].info == info && self[Side::Down].info == info) {
      // liquid and top and bottom cells are the same as this one
      this->SetYOffsets(1,1,1,1); 
      this->SetYOffsetsBottom(0,0,0,0);
    } else if (self[Side::Up].info == info && self[Side::Down].info != info) {
      // liquid and liquid above and nothing liquid below
      this->SetYOffsets(1,1,1,1);
      this->SetYOffsetsBottom(1-h,1-h,1-h,1-h);
    } else {
      // no liquid above
      this->SetYOffsetsBottom(0,0,0,0);
      this->SetYOffsets(h,h,h,h);
    }
  }
  
  UpdateNeighbours();
}

void Cell::OnUse(RunningState &state, Mob &user, bool force) {
  if (state.GetGame().GetTime() - this->lastUseT < this->info->useDelay) return;
  if (!force && !state.GetRandom().Chance(this->info->useChance)) return;

  this->lastUseT = state.GetGame().GetTime();

  const CellProperties *info = this->info;
  if (info->flags & CellFlags::OnUseReplace) {
    this->world->SetCell(GetPosition(), Cell(info->replace)).lastUseT = state.GetGame().GetTime();
  }
  
  for (int i=0; i<6; i++) {
    if (info->onUseCascade & (1<<i) && this->neighbours[i]->info == info) 
      this->neighbours[i]->OnUse(state, user, true);
  }
}

void Cell::Tick(RunningState &state) {
  this->tickPhase = (this->tickPhase + 1) % this->shared.tickInterval;
  //if (this->tickPhase) return;

  if (this->neighbours[0] == nullptr) return;
  
  if ((this->info->flags & CellFlags::Liquid) && this->shared.detail > 0) {
    if (self[Side::Up].info == self.info && self.shared.detail < 16) return;
    
    // if we can't flow down and have more than 1 unit of liquid
    if (!this->Flow(Side::Down) && this->shared.detail > 1) {
      // try flowing to one random side
      Side sides[4] = { Side::Left, Side::Right, Side::Forward, Side::Backward };
      int n = state.GetRandom().Integer(4);
      for (int i=0; i<4; i++) {
        if (this->Flow(sides[(n+i)%4])) {
          break;
        }
      }
      
      // if we have too much liquid, even allow flowing up
      if (this->shared.detail > 16) this->Flow(Side::Up);
    }
  }
  
  // if this cell wants to be replaced if detail falls below a certain level
  if (info->detailBelowReplace && this->shared.detail < this->info->detailBelowReplace && this->info->replace != "") {
    // check if we are connected to liquid neighbours
    bool liquidNeighbours = false;
    liquidNeighbours |= this->neighbours[(int)Side::Left]->info == this->info     && this->neighbours[(int)Side::Left]->shared.detail > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Right]->info == this->info    && this->neighbours[(int)Side::Right]->shared.detail > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Forward]->info == this->info  && this->neighbours[(int)Side::Forward]->shared.detail > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Backward]->info == this->info && this->neighbours[(int)Side::Backward]->shared.detail > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Down]->info == this->info     && this->neighbours[(int)Side::Down]->shared.detail > info->detailBelowReplace;
    
    // if not connected, take a chance and replace
    if (!liquidNeighbours && state.GetRandom().Chance(this->info->replaceChance)) {
      this->world->SetCell(GetPosition(), Cell(info->replace));
      // this is no longer valid
      return;
    }
  }
}

bool
Cell::Flow(Side side) {
  if ((this->info->flags & CellFlags::Liquid) == 0) return false;

  Cell *cell = this->neighbours[(int)side];
  
  if (cell->IsSolid()) return false;
  
  if (cell->info->flags & CellFlags::Liquid) {
    // combine lava and water to rock
    if (this->info->onFlowOntoReplaceTarget.find(cell->info->type) != this->info->onFlowOntoReplaceTarget.end()) {
      this->world->SetCell(this->pos[side], this->info->onFlowOntoReplaceSelf.at(cell->info->type));
      this->world->SetCell(this->pos,       this->info->onFlowOntoReplaceTarget.at(cell->info->type));
      return true;
    }
    
    // check if cell is full
    if ((cell->shared.detail >= this->shared.detail && side != Side::Down) || cell->shared.detail >= 16) return false;
  }
  
  this->shared.detail -= 1;
  
  if (cell->info != this->info) {
    // replace target cell if not already of this type
    this->world->SetCell(this->pos[side], Cell(info->type));
    cell->shared.detail = 1;
    cell->shared.smoothDetail = 1;
    cell->UpdateVertices();
  } else {
    // otherwise just increase liquid
    cell->shared.detail++;
  }

  UpdateNeighbours();
  cell->UpdateNeighbours();
  return true;
}

float 
Cell::GetHeight(
  float x, 
  float z
) const {
  x -= (int)x;
  z -= (int)z;
  
  return GetHeightClamp(x, z);
}

float
Cell::GetHeightClamp(
  float x,
  float z
) const {
  if (x > 1.0) x = 1.0; 
  if (x < 0.0) x = 0.0;
  if (z > 1.0) z = 1.0; 
  if (z < 0.0) z = 0.0;
  
  float slopeX;
  float slopeZ;
 
  if (this->shared.reversedTop) {
    // Z ^
    //   | 1---2
    //   | | \ |
    //   | 0---3
    //   +------> X
   
    if (x < z) {
      // lower left triangle
      slopeX = YOfs(3) - YOfs(0);
      slopeZ = YOfs(1) - YOfs(0);
    } else {
      // upper right triangle
      slopeX = YOfs(2) - YOfs(1);
      slopeZ = YOfs(2) - YOfs(3);
    }
  } else { 
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X
  
    if (x < z) {
      // upper left triangle
      slopeX = YOfs(2) - YOfs(1);
      slopeZ = YOfs(1) - YOfs(0);
    } else {
      // lower right triangle
      slopeX = YOfs(3) - YOfs(0);
      slopeZ = YOfs(2) - YOfs(3);
    }
  }
  
  return YOfs(0) + slopeX * x + slopeZ * z;
}

float 
Cell::GetHeightBottom(
  float x, 
  float z
) const {
  x -= (int)x;
  z -= (int)z;
  
  return GetHeightBottomClamp(x,z);
}

float
Cell::GetHeightBottomClamp(
  float x,
  float z
) const {
  if (x > 1.0) x = 1.0; 
  if (x < 0.0) x = 0.0;
  if (z > 1.0) z = 1.0; 
  if (z < 0.0) z = 0.0;

  float slopeX;
  float slopeZ;
 
  if (this->shared.reversedBottom) {
    // Z ^
    //   | 1---2
    //   | | \ |
    //   | 0---3
    //   +------> X
   
    if (x < z) {
      // lower left triangle
      slopeX = YOfsb(3) - YOfsb(0);
      slopeZ = YOfsb(1) - YOfsb(0);
    } else {
      // upper right triangle
      slopeX = YOfsb(2) - YOfsb(1);
      slopeZ = YOfsb(2) - YOfsb(3);
    }
  } else { 
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X
  
    if (x < z) {
      // upper left triangle
      slopeX = YOfsb(2) - YOfsb(1);
      slopeZ = YOfsb(1) - YOfsb(0);
    } else {
      // lower right triangle
      slopeX = YOfsb(3) - YOfsb(0);
      slopeZ = YOfsb(2) - YOfsb(3);
    }
  }
  
  return YOfsb(0) + slopeX * x + slopeZ * z;
}

void
Cell::UpdateNeighbours(
) {
  if (this->world == nullptr) return;
  
  size_t oldvis = this->visibility;
  this->visibility = 0;

  // check if vertically out of box
  bool oversize = YOfs(0)  > 1.0 || YOfs(1)  > 1.0 || YOfs(2)  > 1.0 || YOfs(3)  > 1.0 ||
                  YOfsb(0) < 0.0 || YOfsb(1) < 0.0 || YOfsb(2) < 0.0 || YOfsb(3) < 0.0;

  // check for transparent neighbours
  for (size_t i =0; i<6; i++) {
    // all sides visible if oversize
    if (oversize) {
      this->visibility |= 1<<i;
      continue;
    }
    
    Cell &cell = *this->neighbours[i];
    
    // show sides to transparent cells
    if (cell.IsTransparent() || cell.info->scale != Vector3(1,1,1)) {
      this->visibility |= 1<<i;
    }
    
    // make interliquid sides transparent
    if (this->info == cell.info && this->info->flags & CellFlags::Liquid) {
      this->visibility &= ~(1<<i);
    }
    
    // TODO: instead hide sides with equals heights 
  }
  
  // if scaled, assume transparent
  if (info->scale.x != 1.0) this->visibility |= (1<<Side::Left)    | (1<<Side::Right);
  if (info->scale.y != 1.0) this->visibility |= (1<<Side::Up)      | (1<<Side::Down);
  if (info->scale.z != 1.0) this->visibility |= (1<<Side::Forward) | (1<<Side::Backward);

  // check nonflat top and bottom
  if (!this->IsTopFlat())    this->visibility |= 1<<Side::Up;
  if (!this->IsBottomFlat()) this->visibility |= 1<<Side::Down;

  // override
  this->visibility |= this->info->showSides;
  this->visibility &= ~(this->info->hideSides);

  // only transparent cells can be lit
  IColor color;
  if (this->IsTransparent()) { 
    // collect max light from neighbours
    for (size_t i=0; i<6; i++) {
      Cell &cell = *this->neighbours[i];
      color = color.Max(cell.lightLevel);
    }
    
    // propagate light
    color = (color * this->info->lightFactor) - this->info->lightFade;
    
    // emit light
    color = color.Max(this->info->light);
  }

  bool updated = false;
  
  // update this cell and neighbours recursively until nothing changes anymore
  // FIXME: change return type to void, we don't need to recurse
  if (this->SetLightLevel(color) || this->visibility != oldvis) {
    updated = true;
    for (size_t i=0; i<6; i++) {
      Cell &cell = *this->neighbours[i];
      cell.UpdateNeighbours();
    }
  }
  
  this->SetDirty();
  
  if (updated) {
    // set corner cells dirty as well
    this->world->GetCell(this->pos + IVector3( 1,  1,  1)).SetDirty();
    this->world->GetCell(this->pos + IVector3(-1,  1,  1)).SetDirty();
    this->world->GetCell(this->pos + IVector3( 1, -1,  1)).SetDirty();
    this->world->GetCell(this->pos + IVector3(-1, -1,  1)).SetDirty();
    this->world->GetCell(this->pos + IVector3( 1,  1, -1)).SetDirty();
    this->world->GetCell(this->pos + IVector3(-1,  1, -1)).SetDirty();
    this->world->GetCell(this->pos + IVector3( 1, -1, -1)).SetDirty();
    this->world->GetCell(this->pos + IVector3(-1, -1, -1)).SetDirty();
  }
}
  
IColor 
CellRender::SideCornerColor(Side side, size_t corner) const {
  if (!world) return IColor();
  
  if (!GetInfo().light.IsBlack()) {
    return GetInfo().light;
  }

  IVector3 va(1,0,0), vb(0,1,0);
  IVector3 p0 = pos[side]; 

  if (side == Side::Right || side == Side::Left) va = IVector3(0,0,1);
  if (side == Side::Up    || side == Side::Down) vb = IVector3(0,0,1);

  // 0 == -1, -1, 1 == -1, 1, 2 == 1,1, 3 == 1,-1

  if (corner == 0 || corner == 1) va = -va;
  if (corner == 0 || corner == 3) vb = -vb;

  if (side == Side::Left || side == Side::Forward) va = -va;
  if (side == Side::Down) vb = -vb;

  IColor l0, l1, l2, l3;
  l0 = this->world->GetLight(p0);
  l1 = this->world->GetLight(p0+va);
  l2 = this->world->GetLight(p0+vb);
  l3 = this->world->GetLight(p0+va+vb);

  return (l0+l1+l2+l3)/4;
}

Cell &
Cell::SetOrder(bool topReversed, bool bottomReversed) {
  this->shared.reversedTop = topReversed;
  this->shared.reversedBottom = bottomReversed;
  this->SetDirty();
  return *this;
}

Cell &
Cell::SetYOffsets(float a, float b, float c, float d) {
  this->shared.topHeights[0] = a * OffsetScale;
  this->shared.topHeights[1] = b * OffsetScale;
  this->shared.topHeights[2] = c * OffsetScale;
  this->shared.topHeights[3] = d * OffsetScale;
  this->SetDirty();
  return *this;
}

Cell &
Cell::SetYOffsetsBottom(float a, float b, float c, float d) {
  this->shared.bottomHeights[0] = a * OffsetScale;
  this->shared.bottomHeights[1] = b * OffsetScale;
  this->shared.bottomHeights[2] = c * OffsetScale;
  this->shared.bottomHeights[3] = d * OffsetScale;
  this->SetDirty();
  return *this;
}

bool Cell::HasSolidSides() const {
  if (this->neighbours[0] == nullptr) return true;
  return this->neighbours[(int)Side::Left]->IsSolid() &&
         this->neighbours[(int)Side::Right]->IsSolid() &&
         this->neighbours[(int)Side::Forward]->IsSolid() &&
         this->neighbours[(int)Side::Backward]->IsSolid();
}

bool Cell::CheckSideSolid(Side side, const Vector3 &org) const {
  if (!this->world) return true;

  Cell *cell = this->neighbours[(int)side];

  // check for clipping movement into cell from opposite side
  bool clipIn  = (cell->info->clipSidesIn  & (int)(1<<(-side)));
  
  // check for clipping movement out the cell
  bool clipOut = (this->info->clipSidesOut & (1<<side));
  
  if (side == Side::Up || side == Side::Down) {
    return (clipIn || clipOut);
  }

  bool heightCheck = org.y < (this->pos.y+cell->GetHeightClamp( org.x-(int)org.x + (side==Side::Left?1:0), org.z-(int)org.z) + (side==Side::Backward?1:0));
  return (clipIn && heightCheck) || clipOut;
}

void Cell::SetWorld(World *world, const IVector3 &pos) { 
  this->world = world; 
  
  if (info->textures.empty()) {
    this->SetTexture(0, info->flags & MultiSided);
  } else {
    this->SetTexture(info->textures[world->GetState().GetRandom().Integer(info->textures.size())], info->flags & MultiSided);
  }

  
  //this->tickPhase = pos.y % this->shared.tickInterval;
  // this->tickPhase = this->world->GetRandom().Integer(this->shared.tickInterval);
  // this->reversedSides = this->world->GetRandom().Integer(2);
  this->dirty = true;
  
  this->pos = pos; 
  for (size_t i=0; i<6; i++) {
    this->neighbours[i] = &this->world->GetCell(pos[(Side)i]);
  }
  
  for (size_t i=0; i<6; i++) {
    this->neighbours[i]->UpdateNeighbours();
  }
  
  this->UpdateNeighbours();
}

AABB Cell::GetAABB() const {
  AABB aabb;
  aabb.center = Vector3(this->pos) + Vector3(0.5,0.5,0.5);
  aabb.extents = Vector3(0.5,0.5,0.5);
  return aabb;
}
  
bool Cell::IsSeen() const {
  if (!this->world) return false;
  if (this->GetFeatureID() == ~0UL) return false;
  return this->world->IsFeatureSeen(this->GetFeatureID());
}

bool 
Cell::Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const {
  bool hit = false;
  float tt = INFINITY;
  
  for (size_t i=0; i<verts.size(); i+=3) {
    Vector3 tri[3] = {
      Vector3(this->verts[i+0].xyz[0], this->verts[i+0].xyz[1], this->verts[i+0].xyz[2]),
      Vector3(this->verts[i+1].xyz[0], this->verts[i+1].xyz[1], this->verts[i+1].xyz[2]),
      Vector3(this->verts[i+2].xyz[0], this->verts[i+2].xyz[1], this->verts[i+2].xyz[2])
    };
    
    float ttt;
    Vector3 pp;
    
    if (TriangleRay(tri, start, dir, ttt, pp)) {
      hit = true;
      if (ttt < tt) tt = ttt;
    }
  }
  
  if (hit) {
    t = tt;
    p = start + dir * t;
  }
  
  return hit;
}

void Cell::OnStepOn(RunningState &, Mob &) {
  // TODO: traps, teleports
}

void Cell::OnStepOff(RunningState &, Mob &) {
  // TODO: traps, teleports
}
  


Serializer &operator << (Serializer &ser, const Cell &cell) {
  ser << cell.info->type;
  ser << (cell.texture?cell.texture->name:"") << cell.uscale;
  ser << cell.lastT << cell.tickPhase << cell.lastUseT;
  ser << cell.lightLevel;
  
  ser << cell.shared.tickInterval;
  ser << cell.shared.featureID;
  ser << uint8_t((cell.shared.reversedTop ? 1 : 0) | (cell.shared.reversedBottom ? 2 : 0));
  
  for (int i=0; i<4; i++) {
    ser << cell.shared.topHeights[i];
    ser << cell.shared.bottomHeights[i];
    ser << cell.shared.u[i];
    ser << cell.shared.v[i];
  }
  
  ser << cell.shared.detail << cell.shared.smoothDetail;
  return ser;
}

Deserializer &operator >> (Deserializer &deser, Cell &cell) {
  std::string cellType;
  deser >> cellType;
  
  cell = Cell(cellType);
  
  std::string textureName;
  deser >> textureName;
  if (textureName != "") cell.texture = loadTexture(textureName);
  
  deser >> cell.uscale;
  deser >> cell.lastT >> cell.tickPhase >> cell.lastUseT;
  deser >> cell.lightLevel;
  
  deser >> cell.shared.tickInterval;
  deser >> cell.shared.featureID;
  
  uint8_t flags;
  deser >> flags;
  cell.shared.reversedTop = flags & 1;
  cell.shared.reversedBottom = flags & 2;
  
  for (int i=0; i<4; i++) {
    deser >> cell.shared.topHeights[i];
    deser >> cell.shared.bottomHeights[i];
    deser >> cell.shared.u[i];
    deser >> cell.shared.v[i];
  }
  
  deser >> cell.shared.detail >> cell.shared.smoothDetail;
  return deser;
}
