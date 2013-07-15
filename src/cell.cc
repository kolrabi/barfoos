#include "cell.h"
#include "util.h"
#include "world.h"
#include "gfx.h"
#include "game.h"
#include "mob.h"

#include "random.h"
#include "serializer.h"
#include "vertex.h"

#include "texture.h"
  
static std::map<std::string, CellInfo> cellInfos;

CellInfo::CellInfo(const std::string &name, FILE *f) : type(name) {
  char line[256];
  this->flags = 0;
  
  while(fgets(line, 256, f) && !feof(f)) {
    std::vector<std::string> tokens = Tokenize(line);
    if (tokens.size() == 0) continue;
    
    for (auto &c:tokens[0]) c = ::tolower(c);
    
    if (tokens[0] == "tex") {
      this->textures.push_back(loadTexture("cells/texture/"+tokens[1]));
    } else if (tokens[0] == "light") {
      this->light = IColor(std::atoi(tokens[1].c_str()), std::atoi(tokens[2].c_str()), std::atoi(tokens[3].c_str()));
    } else if (tokens[0] == "lightscale") this->light = this->light * (float)std::atof(tokens[1].c_str());
    else if (tokens[0] == "lightfactor")  this->lightFactor = std::atof(tokens[1].c_str());
    else if (tokens[0] == "lightfade")    this->lightFade   = std::atoi(tokens[1].c_str());
    
    else if (tokens[0] == "uvturb")       this->flags |= UVTurb | Dynamic;
    else if (tokens[0] == "wave")         this->flags |= Waving | Dynamic;
    else if (tokens[0] == "solid")      { this->flags |= Solid  | Pickable; this->clipSidesIn = ~0; }
    else if (tokens[0] == "dynamic")      this->flags |= Dynamic;
    else if (tokens[0] == "liquid")       this->flags |= Liquid;
    else if (tokens[0] == "norender")     this->flags |= DoNotRender;
    else if (tokens[0] == "transparent")  this->flags |= Transparent;
    else if (tokens[0] == "doublesided")  this->flags |= DoubleSided;
    else if (tokens[0] == "pickable")     this->flags |= Pickable; 
    else if (tokens[0] == "onusereplace") this->flags |= OnUseReplace;
    
    else if (tokens[0] == "speed")        this->speedModifier = std::atof(tokens[1].c_str());
    else if (tokens[0] == "friction")     this->friction      = std::atof(tokens[1].c_str());
    
    else if (tokens[0] == "showsides")    this->showSides     = ParseSidesMask(tokens[1]);
    else if (tokens[0] == "hidesides")    this->hideSides     = ParseSidesMask(tokens[1]);
    else if (tokens[0] == "clipsidesin")  this->clipSidesIn   = ParseSidesMask(tokens[1]);
    else if (tokens[0] == "clipsidesout") this->clipSidesOut  = ParseSidesMask(tokens[1]);
    
    else if (tokens[0] == "onusecascade") this->onUseCascade  = ParseSidesMask(tokens[1]);
    else if (tokens[0] == "usedelay")     this->useDelay      = std::atof(tokens[1].c_str());
    
    else if (tokens[0] == "replace")      this->replace = tokens[1];

    else if (tokens[0] == "strength")     this->breakStrength = std::atof(tokens[1].c_str());
    else if (tokens[0] == "lavadamage")   this->lavaDamage = std::atof(tokens[1].c_str());
    
    else if (tokens[0] == "detailbelowreplace") {
      this->detailBelowReplace = std::atoi(tokens[1].c_str());
      this->replaceChance = std::atof(tokens[2].c_str());
    } else if (tokens[0] == "scale") {
      this->scale = Vector3(std::atof(tokens[1].c_str()), std::atof(tokens[2].c_str()), std::atof(tokens[3].c_str()));
    } else if (tokens[0] == "multi") this->flags |= MultiSided;
    else {
      Log("Ignoring '%s'\n", tokens[0].c_str());
    }
  }
}

void LoadCells() {
  std::vector<std::string> assets = findAssets("cells");
  for (const std::string &name : assets) {
    FILE *f = openAsset("cells/"+name);
    if (f) {
      std::cerr << "loading cell info " << name << std::endl;
      cellInfos[name] = CellInfo(name, f);
      fclose(f);
    }
  }
}

bool IsCellTypeNameValid(const std::string &name) {
  return cellInfos.find(name) != cellInfos.end();
}

// -------------------------------------------------------------------------

Cell::Cell(const std::string &type) 
: info(&cellInfos[type])
{
  // shared information
  if (info->flags & CellFlags::Viscous) {
    this->shared.tickInterval = 32;
  } else {
    this->shared.tickInterval = 5;
  }
  
  this->shared.isLocked = false;
  this->shared.ignoreLock = false;
  this->shared.ignoreWrite = false;
  this->shared.featureID = ~0;

  this->shared.reversedTop = 
  this->shared.reversedBottom = false;
  this->shared.visibilityOverride = 0;
  
  this->SetYOffsets(1,1,1,1);
  this->SetYOffsetsBottom(0,0,0,0);

  this->shared.u[0] = 0;  this->shared.u[1] = 0;  this->shared.u[2] = 0;  this->shared.u[3] = 0;
  this->shared.v[0] = 0;  this->shared.v[1] = 0;  this->shared.v[2] = 0;  this->shared.v[3] = 0;

  if (this->IsLiquid()) {
    this->shared.smoothDetail = this->shared.detail = 15;
    this->shared.detail = 15;
  } else {
    this->shared.smoothDetail = this->shared.detail = 0;
    this->shared.detail = 0;
  }
  
  // unique information
  this->world = nullptr;
  this->tickPhase = 0;
  
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;

  this->visibility = 0;
  this->reversedSides = false;

  this->texture = nullptr;
  this->uscale = 1.0;
  
  this->lastUseT = 0;
  
  this->dirty = true;
}

Cell::Cell(const Cell &that)
: info(that.info), shared(that.shared)
{
  // unique information
  this->world = nullptr;
  this->tickPhase = 0;
  this->visibility = 0;
  this->reversedSides = false;
  
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;

  this->texture = that.texture;
  this->uscale = that.uscale;
  
  this->lastUseT = 0;

  this->dirty = true;
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
  Game &game
) {
  if (!world) return; 
 
  float deltaT = game.GetDeltaT();
  this->lastT = game.GetTime();

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

void Cell::OnUse(Game &game, Mob &user) {
  if (game.GetTime() - this->lastUseT < this->info->useDelay) return;

  this->lastUseT = game.GetTime();
  
  if (info->onUseCascade) {
    for (int i=0; i<6; i++) {
      if (this->info->onUseCascade & (1<<i)) this->neighbours[i]->OnUse(game, user);
    }
  }
  
  if (info->flags & CellFlags::OnUseReplace) {
    this->world->SetCell(GetPosition(), Cell(info->replace)).lastUseT = game.GetTime();
  }
}

void Cell::Tick(Game &game) {
  this->tickPhase = (this->tickPhase + 1) % this->shared.tickInterval;
  //if (this->tickPhase) return;

  if (this->neighbours[0] == nullptr) return;
  
  // TODO: make it pressure, flowrate based, maybe?
  if ((this->info->flags & CellFlags::Liquid) && this->shared.detail > 0) {
    if (self[Side::Up].info == self.info && self.shared.detail < 16) return;
    
    // if we can't flow down and have more than 1 unit of liquid
    if (!this->Flow(Side::Down) && this->shared.detail > 1) {
      // try flowing to one random side
      Side sides[4] = { Side::Left, Side::Right, Side::Forward, Side::Backward };
      int n = game.GetRandom().Integer(4);
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
    if (!liquidNeighbours && game.GetRandom().Chance(this->info->replaceChance)) {
      this->world->SetCell(GetPosition(), Cell(info->replace));
      // this is no longer valid
      return;
    }
  }
}

bool
Cell::Flow(Side side) {
  Cell *cell = this->neighbours[(int)side];
  
  if (cell->IsSolid()) return false;
  
  if (cell->info->flags & CellFlags::Liquid) {
    // combine lava and water to rock
    // TODO: make this data based
    if (cell->info->type == "water" && this->info->type == "lava") {
      this->world->SetCell(this->pos[side], Cell("rock"));
      this->world->SetCell(this->pos,       Cell("air"));
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
  this->visibility |= this->shared.visibilityOverride;
  
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

Serializer &operator << (Serializer &ser, const Cell &cell) {
  ser << cell.info->type;
  /*
  ser << cell.lightLevel.r;
  ser << cell.lightLevel.g;
  ser << cell.lightLevel.b;
  ser << cell.visibility << cell.visibilityOverride;
  ser << cell.topHeights[0] << cell.topHeights[1] << cell.topHeights[2] << cell.topHeights[3];
  ser << cell.bottomHeights[0] << cell.bottomHeights[1] << cell.bottomHeights[2] << cell.bottomHeights[3];
  */
  return ser;
}

void Cell::SetWorld(World *world, const IVector3 &pos) { 
  this->world = world; 
  
  if (info->textures.size() == 0) {
    this->SetTexture(0, info->flags & MultiSided);
  } else {
    this->SetTexture(info->textures[world->GetGame().GetRandom().Integer(info->textures.size())], info->flags & MultiSided);
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
