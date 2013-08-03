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
 
#include <unordered_map>

static std::unordered_map<std::string, CellProperties> cellProperties;

CellProperties::CellProperties() :
  type("default"),
  textures(0),
  light(0,0,0),
  flags(0),
  lightFactor(0.85),
  lightFade(0),
  replace(""),
  replaceChance(0.0),
  detailBelowReplace(0),
  scale(1.0, 1.0, 1.0),
  speedModifier(1.0),
  friction(1.0),
  showSides(0),
  hideSides(0),
  clipSidesIn(0),
  clipSidesOut(0),
  onUseCascade(0),
  useDelay(0.0),
  breakStrength(1.0),
  lavaDamage(0.0),
  useChance(1.0),
  breakParticle("particle.rock")
{}

void CellProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "tex")               Parse("cells/texture/", this->textures);
  
  else if (cmd == "light")        Parse(this->light);
  else if (cmd == "lightscale")   { float f; Parse(f); this->light = this->light * f; }
  else if (cmd == "lightfactor")  Parse(this->lightFactor);
  else if (cmd == "lightfade")    Parse(this->lightFade);
  
  else if (cmd == "uvturb")       this->flags |= UVTurb | Dynamic;
  else if (cmd == "wave")         this->flags |= Waving | Dynamic;
  else if (cmd == "solid")      { this->flags |= Solid  | Pickable; this->clipSidesIn = ~0; }
  else if (cmd == "dynamic")      this->flags |= Dynamic;
  else if (cmd == "liquid")       this->flags |= Liquid;
  else if (cmd == "norender")     this->flags |= DoNotRender;
  else if (cmd == "transparent")  this->flags |= Transparent;
  else if (cmd == "doublesided")  this->flags |= DoubleSided;
  else if (cmd == "pickable")     this->flags |= Pickable; 
  else if (cmd == "onusereplace") this->flags |= OnUseReplace;
  else if (cmd == "multi")        this->flags |= MultiSided;
  else if (cmd == "ladder")       this->flags |= Ladder;
  
  else if (cmd == "speed")        Parse(this->speedModifier);
  else if (cmd == "friction")     Parse(this->friction);
  
  else if (cmd == "showsides")    ParseSideMask(this->showSides);
  else if (cmd == "hidesides")    ParseSideMask(this->hideSides);
  else if (cmd == "clipsidesin")  ParseSideMask(this->clipSidesIn);
  else if (cmd == "clipsidesout") ParseSideMask(this->clipSidesOut);
  
  else if (cmd == "onusecascade") ParseSideMask(this->onUseCascade);
  else if (cmd == "usedelay")     Parse(this->useDelay);
  else if (cmd == "usechance")    Parse(this->useChance); 
  
  else if (cmd == "replace")      Parse(this->replace);

  else if (cmd == "strength")     Parse(this->breakStrength);
  else if (cmd == "lavadamage")   Parse(this->lavaDamage);
  
  else if (cmd == "scale")        Parse(this->scale);
  else if (cmd == "breakparticle")     Parse(this->breakParticle);
  
  else if (cmd == "detailbelowreplace") Parse(this->detailBelowReplace);
  else if (cmd == "replacechance") Parse(this->replaceChance); 
  else if (cmd == "onflowontoreplacetarget") {
    std::string from, to, own;
    Parse(from);
    Parse(to);
    Parse(own);
    this->onFlowOntoReplaceTarget[from] = to;
    this->onFlowOntoReplaceSelf[from] = own;
  }
  else SetError("Ignoring '" + cmd + "'\n");
}

void LoadCells() {
  std::vector<std::string> assets = findAssets("cells");
  for (const std::string &name : assets) {
    FILE *f = openAsset("cells/"+name);
    if (f) {
      Log("Loading cell properties for type '%s'\n", name.c_str());
      cellProperties[name].ParseFile(f);
      cellProperties[name].type = name;
      fclose(f);
    }
  }
}

// -------------------------------------------------------------------------

Cell::Cell(const std::string &type) : 
  info(&cellProperties[type]),
  world(nullptr),
  pos(0,0,0),
  dirty(true),
  
  tickPhase(0),
  lightLevel(0,0,0),
  lastT(0.0),
  visibility(0),
  reversedSides(false),
  verts(0),
  
  texture(nullptr),
  uscale(1.0),
  
  lastUseT(0.0),
  
  shared(info)
{
  // unique information
  this->SetYOffsets(1,1,1,1);
  this->SetYOffsetsBottom(0,0,0,0);

  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;
}

Cell::Cell(const Cell &that)
: info(that.info), 
  world(nullptr),
  pos(0,0,0),
  dirty(true),
  
  tickPhase(0),
  lightLevel(0,0,0),
  lastT(0.0),
  visibility(0),
  reversedSides(false),
  verts(0),
  
  texture(nullptr),
  uscale(1.0),
  
  lastUseT(0.0),
  
  shared(that.shared)
{}

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
Cell::Draw(std::vector<Vertex> &verts) const {
  if (info->flags & CellFlags::DoNotRender || visibility == 0) return;
  
  for (const Vertex &v : this->verts) {
    verts.push_back(v);
  }
}

void 
Cell::DrawHighlight(std::vector<Vertex> &verts) const {
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
Cell::SideCornerColor(Side side, size_t corner) const {
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

void 
Cell::SideColors(Side side, IColor *colors) const {
  colors[0] = this->SideCornerColor(side, 0);
  colors[1] = this->SideCornerColor(side, 1);
  colors[2] = this->SideCornerColor(side, 2);
  colors[3] = this->SideCornerColor(side, 3);
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

Cell &
Cell::SetTexture(const Texture *tex, bool multi) {
  if (multi) {
    this->uscale = 1.0/8.0;
  } else {
    this->uscale = 1.0;
  }
  this->texture = tex; 
  return *this; 
}

void
Cell::SideVerts(Side side, std::vector<Vertex> &verts, bool reverse) const {
  Vector3 uvec;
  Vector3 vvec(0,1,0);
  bool drawA = true, drawB = true;

  int tile = 0;
  int inv = 0;
  int r, s, t;
  
  switch(side) {
    case Side::Right:    r = Corner100; s = CornerY; t = CornerZ; uvec.z = uscale; break;
    case Side::Left:     r = Corner000; s = CornerY; t = CornerZ; uvec.z = uscale; inv = CornerZ; tile = 1; break;
    case Side::Forward:  r = Corner001; s = CornerY; t = CornerX; uvec.x = uscale; inv = CornerX; tile = 2; break;
    case Side::Backward: r = Corner000; s = CornerY; t = CornerX; uvec.x = uscale; tile = 3; break;
    case Side::Up:       r = Corner010; s = CornerZ; t = CornerX; uvec.x = uscale; vvec = Vector3(0,0,1); tile = 4; break;
    case Side::Down:     r = Corner000; s = CornerZ; t = CornerX; uvec.x = uscale; vvec = Vector3(0,0,1); inv = CornerZ; tile = 5; break;
    default: return;
  }
  
  int idx[4] = { (r)^inv, (r+s)^inv, (r+s+t)^inv, (r+t)^inv };

  Vector3 pos[4] = { 
    corners[idx[0]],
    corners[idx[1]],
    corners[idx[2]],
    corners[idx[3]]
  };

  // avoid coplanar faces
  if (side == Side::Up   &&  reverse && pos[0].y == 0.0 && pos[1].y == 0.0 && pos[3].y == 0.0) drawA = false;
  if (side == Side::Up   &&  reverse && pos[1].y == 0.0 && pos[2].y == 0.0 && pos[3].y == 0.0) drawB = false;
  if (side == Side::Up   && !reverse && pos[0].y == 0.0 && pos[1].y == 0.0 && pos[2].y == 0.0) drawA = false;
  if (side == Side::Up   && !reverse && pos[0].y == 0.0 && pos[2].y == 0.0 && pos[3].y == 0.0) drawB = false;

  if (side == Side::Down &&  reverse && pos[0].y == 1.0 && pos[1].y == 1.0 && pos[3].y == 1.0) drawA = false;
  if (side == Side::Down &&  reverse && pos[1].y == 1.0 && pos[2].y == 1.0 && pos[3].y == 1.0) drawB = false;
  if (side == Side::Down && !reverse && pos[0].y == 1.0 && pos[1].y == 1.0 && pos[2].y == 1.0) drawA = false;
  if (side == Side::Down && !reverse && pos[0].y == 1.0 && pos[2].y == 1.0 && pos[3].y == 1.0) drawB = false;

  if (!drawA && !drawB) return;

  float u[4] = { 
    pos[0].Dot(uvec) + this->shared.u[0] + tile * this->uscale, 
    pos[1].Dot(uvec) + this->shared.u[1] + tile * this->uscale, 
    pos[2].Dot(uvec) + this->shared.u[2] + tile * this->uscale, 
    pos[3].Dot(uvec) + this->shared.u[3] + tile * this->uscale 
  };
  float v[4] = { 
    pos[0].Dot(vvec) + this->shared.v[0], 
    pos[1].Dot(vvec) + this->shared.v[1],
    pos[2].Dot(vvec) + this->shared.v[2],
    pos[3].Dot(vvec) + this->shared.v[3] 
  };

  if (GetInfo().flags & CellFlags::UVTurb) {
    float t = this->lastT * 3;
    if (GetInfo().flags & CellFlags::Viscous) {
      t *= 0.5;
    }
    for (int i=0; i<4; i++) {
      Vector3 p(pos[i] + this->pos);
      u[i] = p.x + p.y + cos(p.z + t + p.y) * 0.3;
      v[i] = p.z - p.y + cos(p.x + t - p.y) * 0.3;
    }
  }
  
  IColor colors[4];
  SideColors(side, colors);
  bool doubleSided = info->flags & CellFlags::DoubleSided;

  if (reverse) {
    if (drawA) {
      Vector3 norm = Vector3::Normal(pos[0], pos[1], pos[3]);
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], norm));
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], norm));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], norm));
      
      if (doubleSided) {
        verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], -norm));
        verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], -norm));
        verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], -norm));
      }
    }

    if (drawB) {
      Vector3 norm = Vector3::Normal(pos[1], pos[2], pos[3]);
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], norm));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], norm));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], norm));
      
      if (doubleSided) {
        verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], -norm));
        verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], -norm));
        verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], -norm));
      }
    }
  } else {
    if (drawA) {
      Vector3 norm = Vector3::Normal(pos[0], pos[1], pos[2]);
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], norm));
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], norm));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], norm));
      
      if (doubleSided) {
        verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], -norm));
        verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], -norm));
        verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], -norm));
      }
    }

    if (drawB) {
      Vector3 norm = Vector3::Normal(pos[0], pos[2], pos[3]);
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], norm));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], norm));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], norm));
      
      if (doubleSided) {
        verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], -norm));
        verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], -norm));
        verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], -norm));
      }
    }
  }
}

void
Cell::UpdateVertices() {
  if (!this->dirty) return;
  if ((info->flags & CellFlags::Dynamic) == 0) this->dirty = false;

  float h[4];
  h[0] = YOfs(0);
  h[1] = YOfs(1);
  h[2] = YOfs(2);
  h[3] = YOfs(3);
  
  if (info->flags & CellFlags::Liquid && (neighbours[(int)Side::Up]->info != this->info)) {
    // snap vertices of neighbouring liquid cells together to make a nice connected surface
    Cell *l = neighbours[(int)Side::Left];
    Cell *r = neighbours[(int)Side::Right];
    Cell *f = neighbours[(int)Side::Forward];
    Cell *b = neighbours[(int)Side::Backward];
    Cell *rf = nullptr;
    Cell *lf = nullptr;
    Cell *rb = nullptr;
    Cell *lb = nullptr;
    
    if (l->info != this->info) l = nullptr;
    if (r->info != this->info) r = nullptr;
    if (f->info != this->info) f = nullptr;
    if (b->info != this->info) b = nullptr;
    
    if (r) rf = r->neighbours[(int)Side::Forward];
    if (f) rf = f->neighbours[(int)Side::Right];
    if (rf && rf->info != this->info) rf = nullptr;

    if (l) lf = l->neighbours[(int)Side::Forward];
    if (f) lf = f->neighbours[(int)Side::Left];
    if (lf && lf->info != this->info) lf = nullptr;

    if (l) lb = l->neighbours[(int)Side::Backward];
    if (b) lb = b->neighbours[(int)Side::Left];
    if (lb && lb->info != this->info) lb = nullptr;
    
    if (r) rb = r->neighbours[(int)Side::Backward];
    if (b) rb = b->neighbours[(int)Side::Right];
    if (rb && rb->info != this->info) rb = nullptr;


    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X

    float w[4] = {1,1,1,1};
    
    // 0
    if (lb) { h[0] += lb->YOfs(2); w[0]++; };
    if (b)  { h[0] += b ->YOfs(1); w[0]++; };
    if (l)  { h[0] += l ->YOfs(3); w[0]++; };
    
    // 1
    if (lf) { h[1] += lf->YOfs(3); w[1]++; };
    if (f)  { h[1] += f ->YOfs(0); w[1]++; };
    if (l)  { h[1] += l ->YOfs(2); w[1]++; };

    // 2
    if (rf) { h[2] += rf->YOfs(0); w[2]++; };
    if (f)  { h[2] += f ->YOfs(3); w[2]++; };
    if (r)  { h[2] += r ->YOfs(1); w[2]++; };
    
    // 3
    if (rb) { h[3] += rb->YOfs(1); w[3]++; };
    if (b)  { h[3] += b ->YOfs(2); w[3]++; };
    if (r)  { h[3] += r ->YOfs(0); w[3]++; };
        
    h[0] /= w[0]; h[1] /= w[1]; h[2] /= w[2]; h[3] /= w[3];
  }  
  
  if (GetInfo().flags & CellFlags::Waving) {
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X

    float t = this->lastT * 3;
    if (GetInfo().flags & CellFlags::Viscous) {
      t *= 0.5;
    }
    h[0] += Wave(this->pos.x,   this->pos.z,   t, 0.1);
    h[1] += Wave(this->pos.x,   this->pos.z+1, t, 0.1);
    h[2] += Wave(this->pos.x+1, this->pos.z+1, t, 0.1);
    h[3] += Wave(this->pos.x+1, this->pos.z,   t, 0.1);
  }
  
  const float &scaleX = info->scale.x;
  const float &scaleY = info->scale.y;
  const float &scaleZ = info->scale.z;
  
  corners[0] = Vector3(0.5-0.5*scaleX, 0.5+(YOfsb(0)-0.5)*scaleY, 0.5-0.5*scaleZ); 
  corners[1] = Vector3(0.5+0.5*scaleX, 0.5+(YOfsb(3)-0.5)*scaleY, 0.5-0.5*scaleZ); 
  corners[2] = Vector3(0.5-0.5*scaleX, 0.5+(h[0]    -0.5)*scaleY, 0.5-0.5*scaleZ); 
  corners[3] = Vector3(0.5+0.5*scaleX, 0.5+(h[3]    -0.5)*scaleY, 0.5-0.5*scaleZ); 

  corners[4] = Vector3(0.5-0.5*scaleX, 0.5+(YOfsb(1)-0.5)*scaleY, 0.5+0.5*scaleZ); 
  corners[5] = Vector3(0.5+0.5*scaleX, 0.5+(YOfsb(2)-0.5)*scaleY, 0.5+0.5*scaleZ); 
  corners[6] = Vector3(0.5-0.5*scaleX, 0.5+(h[1]    -0.5)*scaleY, 0.5+0.5*scaleZ);
  corners[7] = Vector3(0.5+0.5*scaleX, 0.5+(h[2]    -0.5)*scaleY, 0.5+0.5*scaleZ);

  verts.clear();
  
  if (visibility & (1<<Side::Right))    SideVerts(Side::Right,    verts, this->reversedSides);
  if (visibility & (1<<Side::Left))     SideVerts(Side::Left,     verts, this->reversedSides);
  if (visibility & (1<<Side::Up))       SideVerts(Side::Up,       verts, this->shared.reversedTop);
  if (visibility & (1<<Side::Down))     SideVerts(Side::Down,     verts, this->shared.reversedBottom);
  if (visibility & (1<<Side::Forward))  SideVerts(Side::Forward,  verts, this->reversedSides);
  if (visibility & (1<<Side::Backward)) SideVerts(Side::Backward, verts, this->reversedSides);
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
  
  if (info->textures.size() == 0) {
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
