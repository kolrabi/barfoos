#include "cell.h"
#include "util.h"
#include "world.h"
#include "gfx.h"
#include "game.h"

#include "random.h"
#include "serializer.h"
#include "vertex.h"
  
static std::map<std::string, CellInfo> cellInfos;

CellInfo::CellInfo(FILE *f) {
  char line[256];
  this->flags = 0;
  
  while(fgets(line, 256, f) && !feof(f)) {
    std::vector<std::string> tokens;
    char *p = line;
    char *q;
    do {
      q = strchr(p, ' ');
      if (!q) q = strchr(p, '\r');
      if (!q) q = strchr(p, '\n');
      if (q) *q = 0;
      tokens.push_back(p);
      if (q) { p = q+1; }
    } while(q);
    if (tokens.size() == 0) continue;

    if (tokens[0] == "tex") {
      // TODO: check
      this->textures.push_back(loadTexture("cells/texture/"+tokens[1]));
    } else if (tokens[0] == "light") {
      // TODO: check
      this->light = IColor(std::atoi(tokens[1].c_str()), std::atoi(tokens[2].c_str()), std::atoi(tokens[3].c_str()));
    } else if (tokens[0] == "lightfactor") {
      this->lightFactor = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "lightfade") {
      this->lightFade = std::atoi(tokens[1].c_str());
    } 
    else if (tokens[0] == "uvturb") this->flags |= UVTurb | Dynamic;
    else if (tokens[0] == "wave") this->flags |= Waving | Dynamic;
    else if (tokens[0] == "solid") this->flags |= Solid;
    else if (tokens[0] == "dynamic") this->flags |= Dynamic;
    else if (tokens[0] == "liquid") this->flags |= Liquid;
    else if (tokens[0] == "norender") this->flags |= DoNotRender;
    else if (tokens[0] == "transparent") this->flags |= Transparent;
    else if (tokens[0] == "detailbelowreplace") {
      this->detailBelowReplace = std::atoi(tokens[1].c_str());
      this->replaceChance = std::atof(tokens[2].c_str());
      this->replace = tokens[3];
    }
    else if (tokens[0] == "scale") {
      this->scale = Vector3(std::atof(tokens[1].c_str()), std::atof(tokens[2].c_str()), std::atof(tokens[3].c_str()));
    }
    else if (tokens[0] == "multi") this->flags |= MultiSided;
    else {
      std::cerr << "ignoring '" << tokens[0] << "'" << std::endl;
    }
  }
}

void LoadCells() {
  std::vector<std::string> assets = findAssets("cells");
  for (const std::string &name : assets) {
    FILE *f = openAsset("cells/"+name);
    if (f) {
      std::cerr << "loading cell info " << name << std::endl;
      cellInfos[name] = CellInfo(f);
      fclose(f);
    }
  }
}

bool IsCellTypeNameValid(const std::string &name) {
  return cellInfos.find(name) != cellInfos.end();
}

// -------------------------------------------------------------------------

Cell::Cell(const std::string &type) : info(&cellInfos[type])
{
  this->world = nullptr;
  this->isLocked = false;
  this->ignoreLock = false;
  this->ignoreWrite = false;
  this->type = type;
  this->featureID = ~0;
  this->SetDirty();
  this->tickPhase = 0;
  this->tickInterval = 16;

  if (info->flags & CellFlags::Viscous) {
    this->tickInterval = 16;
  } else {
    this->tickInterval = 5;
  }
  
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;

  if (info->textures.size() == 0) {
    this->SetTexture(0, info->flags & MultiSided);
  } else {
    this->SetTexture(info->textures[rand()%info->textures.size()], info->flags & MultiSided);
  }

  if (info->flags & CellFlags::Liquid) {
    this->smoothDetail = this->detail = 15;
  } else {
    this->smoothDetail = this->detail = 0;
  }

  reversedTop = reversedBottom = reversedSides = false;
  visibility = 0;
  visibilityOverride = 0;

  SetYOffsets(1,1,1,1);
  SetYOffsetsBottom(0,0,0,0);

  u[0] = u[1] = u[2] = u[3] = 0;
  v[0] = v[1] = v[2] = v[3] = 0;
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
  for (const Vertex &v : this->verts) {
    Vertex vv(v);
    vv.xyz[0] = (vv.xyz[0] - this->pos.x - 0.5) * 1.1 + this->pos.x + 0.5;
    vv.xyz[1] = (vv.xyz[1] - this->pos.y - 0.5) * 1.1 + this->pos.y + 0.5; 
    vv.xyz[2] = (vv.xyz[2] - this->pos.z - 0.5) * 1.1 + this->pos.z + 0.5;
    verts.push_back(vv);
  }
}

void
Cell::Update(
  Game &game
) {
  if (!world) return;
  
  float t = game.GetTime();
  float deltaT = game.GetDeltaT();

  smoothDetail = smoothDetail + (detail - smoothDetail) * deltaT*10;
  
  if (GetInfo().flags & CellFlags::Waving) {
    float h = 0.8;
    if (info->flags & CellFlags::Liquid) h = 0.8*smoothDetail/16.0;
    if (h>1) h = 1;
    if (GetInfo().flags & CellFlags::Viscous) {
      SetYOffsets(
        Wave(this->pos.x,   this->pos.z,   t, 0.1, h),
        Wave(this->pos.x,   this->pos.z+1, t, 0.1, h),
        Wave(this->pos.x+1, this->pos.z+1, t, 0.1, h),
        Wave(this->pos.x+1, this->pos.z,   t, 0.1, h)
      );
    } else {
      SetYOffsets(
        Wave((this->pos.x)*2,   this->pos.z,   t*5, 0.1, h),
        Wave((this->pos.x)*2,   this->pos.z+1, t*5, 0.1, h),
        Wave((this->pos.x+1)*2, this->pos.z+1, t*5, 0.1, h),
        Wave((this->pos.x+1)*2, this->pos.z,   t*5, 0.1, h)
      );
    }
  }

  if (info->flags & CellFlags::Liquid) {
    float h = smoothDetail/16.0;
    if (neighbours[(int)Side::Up]->info == info && neighbours[(int)Side::Down]->info == info) {
      // liquid and top and bottom cells are the same as this one
      SetYOffsets(1,1,1,1); 
      SetYOffsetsBottom(0,0,0,0);
    } else if (neighbours[(int)Side::Up]->info == info && neighbours[(int)Side::Down]->info != info) {
      // liquid and liquid above and nothing solid below
      SetYOffsetsBottom(1-h,1-h,1-h,1-h);
      SetYOffsets(1,1,1,1);
    } else {
      // no liquid above
      SetYOffsetsBottom(0,0,0,0);
      SetYOffsets(h,h,h,h);
    }
  }
  
  if (GetInfo().flags & CellFlags::UVTurb) {
  
#define U(xx,zz) (sin((2*zz*3.14159)+t*0.5)*0.25)
#define V(xx,zz) (cos((2*xx*3.14159)+t*0.5)*0.25)

    u[0] = U(0,0); v[0] = V(0,0);
    u[1] = U(0,1); v[1] = V(0,1);
    u[2] = U(1,1); v[2] = V(1,1);
    u[3] = U(1,0); v[3] = V(1,0);
  }
  
  UpdateNeighbours();
}

void Cell::Tick(Game &game) {
  this->tickPhase = (this->tickPhase + 1) % this->tickInterval;
  if (this->tickPhase) return;

  if (this->neighbours[0] == nullptr) return;
  if (this->info->flags & CellFlags::Liquid) {
    if (this->neighbours[(int)Side::Up]->info == this->info && detail < 16) return;
    
    if ((!Flow(Side::Down) && detail > 1)) {
      Side sides[4] = { Side::Left, Side::Right, Side::Forward, Side::Backward };
      int n = game.GetRandom().Integer(4);
      for (int i=0; i<4; i++) {
        if (Flow(sides[(n+i)%4])) {
          break;
        }
      }
      if (detail > 16) Flow(Side::Up);
    }
  }
  if (info->detailBelowReplace && detail < info->detailBelowReplace && info->replace != "") {
    bool liquidNeighbours = false;
    liquidNeighbours |= this->neighbours[(int)Side::Left]->info == this->info     && this->neighbours[(int)Side::Left]->detail > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Right]->info == this->info    && this->neighbours[(int)Side::Right]->detail > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Forward]->info == this->info  && this->neighbours[(int)Side::Forward]->detail > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Backward]->info == this->info && this->neighbours[(int)Side::Backward]->detail > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Down]->info == this->info     && this->neighbours[(int)Side::Down]->detail > info->detailBelowReplace;
    if (!liquidNeighbours && world->GetRandom().Chance(info->replaceChance)) {
      Cell c(info->replace);
      c.SetYOffsets(topHeights[0]/32.0, topHeights[1]/32.0, topHeights[2]/32.0, topHeights[3]/32.0);
      world->SetCell(GetPosition(), c);
    }
  }
}

bool
Cell::Flow(Side side) {
  Cell *cell = this->neighbours[(int)side];
  if (cell->IsSolid()) return false;
  if (cell->info->flags & CellFlags::Liquid) {
    if (cell->type == "water" && this->type == "lava") {
      this->world->SetCell(GetPosition()[side], Cell("rock"));
      this->world->SetCell(GetPosition(), Cell("air"));
      return true;
    }
    if ((cell->detail >= detail && side != Side::Down) || cell->detail >= 16) return false;
  }
  
  detail -= 1;
  if (cell->info != this->info) {
    this->world->SetCell(GetPosition()[side], Cell(type));
    cell->detail = 1;
    cell->smoothDetail = 0;
    cell->UpdateVertices();
  } else {
    cell->detail++;
  }
  
  if (!detail) {
    this->world->SetCell(GetPosition(), Cell("air"));
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
 
  if (reversedTop) {
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
 
  if (reversedBottom) {
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

bool
Cell::UpdateNeighbours(
) {
  if (neighbours[0] == nullptr) return false;
  
  size_t oldvis = this->visibility;

  this->visibility = 0;
  
  bool oversize = YOfs(0)  > 1.0 || YOfs(1)  > 1.0 || YOfs(2)  > 1.0 || YOfs(3)  > 1.0 ||
                  YOfsb(0) < 0.0 || YOfsb(1) < 0.0 || YOfsb(2) < 0.0 || YOfsb(3) < 0.0;

  // check for transparent neighbours
  for (size_t i =0; i<6; i++) {
    if (neighbours[i]->IsTransparent() || oversize) this->visibility |= 1<<i;
    if (this->type == neighbours[i]->type && this->info->flags & CellFlags::Liquid) {
      this->visibility &= ~(1<<i);
    }
  }
  
  if (info->scale.x != 1.0) this->visibility |= (1<<Side::Left)    | (1<<Side::Right);
  if (info->scale.y != 1.0) this->visibility |= (1<<Side::Up)      | (1<<Side::Down);
  if (info->scale.z != 1.0) this->visibility |= (1<<Side::Forward) | (1<<Side::Backward);

  if (!this->IsTopFlat())    this->visibility |= 1<<Side::Up;
  if (!this->IsBottomFlat()) this->visibility |= 1<<Side::Down;

  this->visibility |= this->visibilityOverride;

  // only transparent cells can be lit
  IColor c;
  if (this->IsTransparent()) { 
    for (size_t i=0; i<6; i++) {
      c = c.Max(neighbours[i]->lightLevel);
    }
    c = (c * this->info->lightFactor) - this->info->lightFade;
    c = c.Max(info->light);
    c = c.Max(this->torchLight);
  }

  bool updated = false;
  if (SetLightLevel(c) || this->visibility != oldvis) {
    updated = true;
    for (size_t i=0; i<6; i++) {
      while (neighbours[i]->UpdateNeighbours());
    }
  }
  SetDirty();
  if (updated) {
    // set corner cells dirty as well
    this->world->GetCell(this->GetPosition() + IVector3( 1,  1,  1)).SetDirty();
    this->world->GetCell(this->GetPosition() + IVector3(-1,  1,  1)).SetDirty();
    this->world->GetCell(this->GetPosition() + IVector3( 1, -1,  1)).SetDirty();
    this->world->GetCell(this->GetPosition() + IVector3(-1, -1,  1)).SetDirty();
    this->world->GetCell(this->GetPosition() + IVector3( 1,  1, -1)).SetDirty();
    this->world->GetCell(this->GetPosition() + IVector3(-1,  1, -1)).SetDirty();
    this->world->GetCell(this->GetPosition() + IVector3( 1, -1, -1)).SetDirty();
    this->world->GetCell(this->GetPosition() + IVector3(-1, -1, -1)).SetDirty();
  }
  return false; //updated; 
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
  l0 = world->GetLight(p0);
  l1 = world->GetLight(p0+va);
  l2 = world->GetLight(p0+vb);
  l3 = world->GetLight(p0+va+vb);

  return (l0+l1+l2+l3)/4;
}

void Cell::SideColors(Side side, IColor *colors) const {
  colors[0] = SideCornerColor(side, 0);
  colors[1] = SideCornerColor(side, 1);
  colors[2] = SideCornerColor(side, 2);
  colors[3] = SideCornerColor(side, 3);
}

Cell &
Cell::SetOrder(bool topReversed, bool bottomReversed) {
  reversedTop = topReversed;
  reversedBottom = bottomReversed;
  SetDirty();
  return *this;
}

Cell &
Cell::SetYOffsets(float a, float b, float c, float d) {
  topHeights[0] = a*32;
  topHeights[1] = b*32;
  topHeights[2] = c*32;
  topHeights[3] = d*32;
  SetDirty();
  return *this;
}

Cell &
Cell::SetYOffsetsBottom(float a, float b, float c, float d) {
  bottomHeights[0] = a*32;
  bottomHeights[1] = b*32;
  bottomHeights[2] = c*32;
  bottomHeights[3] = d*32;
  SetDirty();
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
  Vector3 norm;
  
  switch(side) {
    case Side::Right:    norm = Vector3( 1,0,0); r = Corner100; s = CornerY; t = CornerZ; uvec.z = uscale; break;
    case Side::Left:     norm = Vector3(-1,0,0); r = Corner000; s = CornerY; t = CornerZ; uvec.z = uscale; inv = CornerZ; tile = 1; break;
    case Side::Forward:  norm = Vector3(0,0, 1); r = Corner001; s = CornerY; t = CornerX; uvec.x = uscale; inv = CornerX; tile = 2; break;
    case Side::Backward: norm = Vector3(0,0,-1); r = Corner000; s = CornerY; t = CornerX; uvec.x = uscale; tile = 3; break;
    case Side::Up:       norm = Vector3(0, 1,0); r = Corner010; s = CornerZ; t = CornerX; uvec.x = uscale; vvec = Vector3(0,0,1); tile = 4; break;
    case Side::Down:     norm = Vector3(0,-1,0); r = Corner000; s = CornerZ; t = CornerX; uvec.x = uscale; vvec = Vector3(0,0,1); inv = CornerZ; tile = 5; break;
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
    pos[0].Dot(uvec)+this->u[0]+tile*uscale, 
    pos[1].Dot(uvec)+this->u[1]+tile*uscale, 
    pos[2].Dot(uvec)+this->u[2]+tile*uscale, 
    pos[3].Dot(uvec)+this->u[3]+tile*uscale };
  float v[4] = { 
    pos[0].Dot(vvec)+this->v[0], 
    pos[1].Dot(vvec)+this->v[1],
    pos[2].Dot(vvec)+this->v[2],
    pos[3].Dot(vvec)+this->v[3] };

  IColor colors[4];
  SideColors(side, colors);

  if (reverse) {
    if (drawA) {
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], norm));
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], norm));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], norm));
    }

    if (drawB) {
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], norm));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], norm));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], norm));
    }
  } else {
    if (drawA) {
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], norm));
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1], norm));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], norm));
    }

    if (drawB) {
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0], norm));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2], norm));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3], norm));
    }
  }
}

void
Cell::UpdateVertices() {
  if (!dirty) return;
  dirty--;
  if (info->flags & CellFlags::Dynamic) SetDirty();

  float h[4];
  h[0] = YOfs(0);
  h[1] = YOfs(1);
  h[2] = YOfs(2);
  h[3] = YOfs(3);
  
  float w[4] = {1,1,1,1};
  
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
  
  if (visibility & (1<<Side::Right))    SideVerts(Side::Right,    verts, reversedSides);
  if (visibility & (1<<Side::Left))     SideVerts(Side::Left,     verts, reversedSides);
  if (visibility & (1<<Side::Up))       SideVerts(Side::Up,       verts, reversedTop);
  if (visibility & (1<<Side::Down))     SideVerts(Side::Down,     verts, reversedBottom);
  if (visibility & (1<<Side::Forward))  SideVerts(Side::Forward,  verts, reversedSides);
  if (visibility & (1<<Side::Backward)) SideVerts(Side::Backward, verts, reversedSides);
}

void Cell::SetPosition(const IVector3 &pos) { 
  this->pos = pos; 
  for (size_t i=0; i<6; i++) {
    this->neighbours[i] = &this->world->GetCell(pos[(Side)i]);
  }
  
  for (size_t i=0; i<6; i++) {
    this->neighbours[i]->UpdateNeighbours();
  }
  UpdateNeighbours();
}

bool Cell::HasSolidSides() const {
  if (this->neighbours[0] == nullptr) return true;
  return this->neighbours[(int)Side::Left]->IsSolid() &&
         this->neighbours[(int)Side::Right]->IsSolid() &&
         this->neighbours[(int)Side::Forward]->IsSolid() &&
         this->neighbours[(int)Side::Backward]->IsSolid();
}

bool Cell::CheckSideSolid(Side side, const Vector3 &org) const {
  Cell *cell = this->neighbours[(int)side];
  
  if (side == Side::Up || side == Side::Down) {
    return cell->IsSolid();
  }

  bool solid = cell->IsSolid();
  bool heightCheck = org.y < (this->pos.y+cell->GetHeightClamp( org.x-(int)org.x + (side==Side::Left?1:0), org.z-(int)org.z) + (side==Side::Backward?1:0));
  return solid && heightCheck;
}

bool Cell::IsFeatureBorder() const {
  for (size_t i=0; i<6; i++) {
    if (this->neighbours[i]->featureID == ~0UL) return true;
  }
  return false; 
}

Serializer &operator << (Serializer &ser, const Cell &cell) {
  ser << cell.type;
  ser << cell.lightLevel.r;
  ser << cell.lightLevel.g;
  ser << cell.lightLevel.b;
  ser << cell.visibility << cell.visibilityOverride;
  ser << cell.topHeights[0] << cell.topHeights[1] << cell.topHeights[2] << cell.topHeights[3];
  ser << cell.bottomHeights[0] << cell.bottomHeights[1] << cell.bottomHeights[2] << cell.bottomHeights[3];
  return ser;
}

void Cell::SetWorld(World *world) { 
  this->world = world; 
  this->tickPhase = this->world->GetRandom().Integer(this->tickInterval);
  this->reversedSides = world->GetRandom().Integer(2);
  this->SetDirty();
}

AABB Cell::GetAABB() const {
  AABB aabb;
  aabb.center = Vector3(pos) + Vector3(0.5,0.5,0.5);
  aabb.extents = Vector3(0.5,0.5,0.5);
  return aabb;
}

bool 
Cell::Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const {
  bool hit = false;
  float tt = INFINITY;
  
  for (size_t i=0; i<verts.size(); i+=3) {
    Vector3 tri[3] = {
      Vector3(verts[i+0].xyz[0], verts[i+0].xyz[1], verts[i+0].xyz[2]),
      Vector3(verts[i+1].xyz[0], verts[i+1].xyz[1], verts[i+1].xyz[2]),
      Vector3(verts[i+2].xyz[0], verts[i+2].xyz[1], verts[i+2].xyz[2])
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
