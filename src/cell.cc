#include "cell.h"
#include "util.h"
#include "world.h"

#include <GL/glfw.h>
#include <cstring>
  
static std::map<std::string, CellInfo> cellInfos;

CellInfo::CellInfo(const std::string &texture, const IColor &light, uint32_t flags) {
  this->textures.push_back(loadTexture("cells/"+texture));
  this->light = light;
  this->flags = flags;
}

CellInfo::CellInfo(const std::string &texture, uint32_t flags) {
  this->textures.push_back(loadTexture("cells/texture/"+texture));
  this->light = IColor();
  this->flags = flags;
}

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
    } 
    else if (tokens[0] == "uvturb") this->flags |= UVTurb | Dynamic;
    else if (tokens[0] == "wave") this->flags |= Waving | Dynamic;
    else if (tokens[0] == "solid") this->flags |= Solid;
    else if (tokens[0] == "dynamic") this->flags |= Dynamic;
    else if (tokens[0] == "liquid") this->flags |= Liquid;
    else if (tokens[0] == "norender") this->flags |= DoNotRender;
    else if (tokens[0] == "transparent") this->flags |= Transparent;
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
  this->dirty = true;
  
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;

  if (info->textures.size() == 0) {
    this->SetTexture(0, info->flags & MultiSided);
  } else {
    this->SetTexture(info->textures[rand()%info->textures.size()], info->flags & MultiSided);
  }

  if (info->flags & CellFlags::Liquid) {
    this->tickInterval = (info->flags & CellFlags::Viscous)?0.75:0.5;
    this->nextTickT = 0;
    this->smoothDetail = this->detail = 16;
  } else {
    this->tickInterval = 0;
    this->nextTickT = 0;
    this->smoothDetail = this->detail = 0;
  }

  reversedTop = reversedBottom = false;
  visibility = 0;
  visibilityOverride = 0;

  yofs[0] = yofs[1] = yofs[2] = yofs[3] = 1.0f;
  yofsb[0] = yofsb[1] = yofsb[2] = yofsb[3] = 0.0f;
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
Cell::Update(
  float t,
  Random &random
) {
  if (!world) return;
    
  smoothDetail = smoothDetail + (detail - smoothDetail) * world->GetDeltaT() * 0.01;
  
  if (GetInfo().flags & CellFlags::Waving) {
    float h = 0.8;
    if (info->flags & CellFlags::Liquid) h = 0.8*detail/16.0;
    if (h>1) h = 1;
    if (GetInfo().flags & CellFlags::Viscous) {
      yofs[0] = Wave(this->pos.x,   this->pos.z,   t, 0.1, h);
      yofs[1] = Wave(this->pos.x,   this->pos.z+1, t, 0.1, h);
      yofs[2] = Wave(this->pos.x+1, this->pos.z+1, t, 0.1, h);
      yofs[3] = Wave(this->pos.x+1, this->pos.z,   t, 0.1, h);
    } else {
      yofs[0] = Wave((this->pos.x)*2,   this->pos.z,   t*5, 0.1, h);
      yofs[1] = Wave((this->pos.x)*2,   this->pos.z+1, t*5, 0.1, h);
      yofs[2] = Wave((this->pos.x+1)*2, this->pos.z+1, t*5, 0.1, h);
      yofs[3] = Wave((this->pos.x+1)*2, this->pos.z,   t*5, 0.1, h);
    }
    if (info->flags & CellFlags::Liquid && neighbours[(int)Side::Up]->info == info && neighbours[(int)Side::Down]->info == info) {
      yofs[0] = neighbours[(int)Side::Up]->yofsb[0]+1;
      yofs[1] = neighbours[(int)Side::Up]->yofsb[1]+1;
      yofs[2] = neighbours[(int)Side::Up]->yofsb[2]+1;
      yofs[3] = neighbours[(int)Side::Up]->yofsb[3]+1;
      yofsb[0] = neighbours[(int)Side::Down]->yofs[0]-2;
      yofsb[1] = neighbours[(int)Side::Down]->yofs[1]-2;
      yofsb[2] = neighbours[(int)Side::Down]->yofs[2]-2;
      yofsb[3] = neighbours[(int)Side::Down]->yofs[3]-2;
    } else if (info->flags & CellFlags::Liquid && neighbours[(int)Side::Up]->info == info && neighbours[(int)Side::Down]->info != info) {
      if (neighbours[(int)Side::Down]->IsSolid()) {
        yofsb[0] = 0;
        yofsb[1] = 0;
        yofsb[2] = 0;
        yofsb[3] = 0;
        yofs[0] = 1;
        yofs[1] = 1;
        yofs[2] = 1;
        yofs[3] = 1;
      } else {
        yofsb[0] = 1.0-yofs[0];
        yofsb[1] = 1.0-yofs[1];
        yofsb[2] = 1.0-yofs[2];
        yofsb[3] = 1.0-yofs[3];
        yofs[0] = neighbours[(int)Side::Up]->yofsb[0]+1;
        yofs[1] = neighbours[(int)Side::Up]->yofsb[1]+1;
        yofs[2] = neighbours[(int)Side::Up]->yofsb[2]+1;
        yofs[3] = neighbours[(int)Side::Up]->yofsb[3]+1;
      }
    }
  }
  
  if (GetInfo().flags & CellFlags::UVTurb) {
  
#define U(xx,zz) (sin((zz)+t*0.7)*0.5)
#define V(xx,zz) (sin((xx)+t*0.5)*0.5)

    u[0] = U(this->pos.x,this->pos.z);     
    v[0] = V(this->pos.x,this->pos.z);

    u[1] = U(this->pos.x,this->pos.z+1);
    v[1] = V(this->pos.x,this->pos.z+1);

    u[2] = U(this->pos.x+1,this->pos.z+1);
    v[2] = V(this->pos.x+1,this->pos.z+1);

    u[3] = U(this->pos.x+1,this->pos.z);
    v[3] = V(this->pos.x+1,this->pos.z);
  }
  
  if (nextTickT == 0.0) nextTickT = t;
  while (tickInterval != 0.0 && t > nextTickT) {
    nextTickT += tickInterval;
    Tick(random);
  }
  
  UpdateNeighbours();
}

void Cell::Tick(Random &random) {
  if (this->neighbours[0] == nullptr) return;
  if (this->info->flags & CellFlags::Liquid) {
    if (this->neighbours[(int)Side::Up]->info == this->info && detail < 16) return;
    
    if ((!Flow(Side::Down) && detail > 1)) {
      Side sides[4] = { Side::Left, Side::Right, Side::Forward, Side::Backward };
      int n = random.Integer(4);
      for (int i=0; i<4; i++) {
        if (Flow(sides[(n+i)%4])) {
          break;
        }
      }
      if (detail > 16) Flow(Side::Up);
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
  
  if (cell->info != this->info) {
    this->world->SetCell(GetPosition()[side], Cell(type));
    cell->detail = 1;
    cell->nextTickT = nextTickT;
  } else {
    cell->detail++;
  }
  
  detail -= 1;
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
      slopeX = yofs[3] - yofs[0];
      slopeZ = yofs[1] - yofs[0];
    } else {
      // upper right triangle
      slopeX = yofs[2] - yofs[1];
      slopeZ = yofs[2] - yofs[3];
    }
  } else { 
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X
  
    if (x < z) {
      // upper left triangle
      slopeX = yofs[2] - yofs[1];
      slopeZ = yofs[1] - yofs[0];
    } else {
      // lower right triangle
      slopeX = yofs[3] - yofs[0];
      slopeZ = yofs[2] - yofs[3];
    }
  }
  
  return yofs[0] + slopeX * x + slopeZ * z;
}

float 
Cell::GetHeightBottom(
  float x, 
  float z
) const {
  x -= (int)x;
  z -= (int)z;

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
      slopeX = yofsb[3] - yofsb[0];
      slopeZ = yofsb[1] - yofsb[0];
    } else {
      // upper right triangle
      slopeX = yofsb[2] - yofsb[1];
      slopeZ = yofsb[2] - yofsb[3];
    }
  } else { 
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X
  
    if (x < z) {
      // upper left triangle
      slopeX = yofsb[2] - yofsb[1];
      slopeZ = yofsb[1] - yofsb[0];
    } else {
      // lower right triangle
      slopeX = yofsb[3] - yofsb[0];
      slopeZ = yofsb[2] - yofsb[3];
    }
  }
  
  return yofsb[0] + slopeX * x + slopeZ * z;
}

void
Cell::UpdateNeighbours(
) {
  if (neighbours[0] == nullptr) return;
  
  size_t oldvis = this->visibility;

  this->visibility = 0;

  // check for transparent neighbours
  for (size_t i =0; i<6; i++) {
    if (neighbours[i]->IsTransparent()) this->visibility |= 1<<i;
    if (this->type == neighbours[i]->type && this->info->flags & CellFlags::Liquid) {
      this->visibility &= ~(1<<i);
    }
  }

  if (!this->IsTopFlat())    this->visibility |= 1<<Side::Up;
  if (!this->IsBottomFlat()) this->visibility |= 1<<Side::Down;

  this->visibility |= this->visibilityOverride;

  // only transparent cells can be lit
  IColor c;
  if (this->IsTransparent()) { 
    for (size_t i=0; i<6; i++) {
      c = c.Max(neighbours[i]->lightLevel);
    }
    c = c - 32;
    c = c.Max(info->light);
  }

  if (SetLightLevel(c) || this->visibility != oldvis) {
    for (size_t i=0; i<6; i++) {
      neighbours[i]->UpdateNeighbours();
    }
  }
  SetDirty();
  return; 
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
  yofs[0] = a;
  yofs[1] = b;
  yofs[2] = c;
  yofs[3] = d;
  SetDirty();
  return *this;
}

Cell &
Cell::SetYOffsetsBottom(float a, float b, float c, float d) {
  yofsb[0] = a;
  yofsb[1] = b;
  yofsb[2] = c;
  yofsb[3] = d;
  SetDirty();
  return *this;
}

Cell &
Cell::SetTexture(unsigned int tex, bool multi) {
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
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0]));
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1]));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3]));
    }

    if (drawB) {
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1]));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2]));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3]));
    }
  } else {
    if (drawA) {
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0]));
      verts.push_back(Vertex(pos[1]+this->pos, colors[1], u[1], v[1]));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2]));
    }

    if (drawB) {
      verts.push_back(Vertex(pos[0]+this->pos, colors[0], u[0], v[0]));
      verts.push_back(Vertex(pos[2]+this->pos, colors[2], u[2], v[2]));
      verts.push_back(Vertex(pos[3]+this->pos, colors[3], u[3], v[3]));
    }
  }
}

void
Cell::UpdateVertices() {
  if (!dirty) return;
  dirty = info->flags & CellFlags::Dynamic;

  float h[4];
  h[0] = yofs[0];
  h[1] = yofs[1];
  h[2] = yofs[2];
  h[3] = yofs[3];
  
  float w[4] = {1,1,1,1};
  
  if (info->flags & CellFlags::Liquid && (neighbours[(int)Side::Down]->IsSolid() || neighbours[(int)Side::Down]->detail == 16)) {
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
    if (lb) { h[0] += lb->yofs[2]; w[0]++; };
    if (b)  { h[0] += b ->yofs[1]; w[0]++; };
    if (l)  { h[0] += l ->yofs[3]; w[0]++; };
    
    // 1
    if (lf) { h[1] += lf->yofs[3]; w[1]++; };
    if (f)  { h[1] += f ->yofs[0]; w[1]++; };
    if (l)  { h[1] += l ->yofs[2]; w[1]++; };

    // 2
    if (rf) { h[2] += rf->yofs[0]; w[2]++; };
    if (f)  { h[2] += f ->yofs[3]; w[2]++; };
    if (r)  { h[2] += r ->yofs[1]; w[2]++; };
    
    // 3
    if (rb) { h[3] += rb->yofs[1]; w[3]++; };
    if (b)  { h[3] += b ->yofs[2]; w[3]++; };
    if (r)  { h[3] += r ->yofs[0]; w[3]++; };
        
    h[0] /= w[0]; h[1] /= w[1]; h[2] /= w[2]; h[3] /= w[3];
  }  
  
  corners[0] = Vector3(0, yofsb[0], 0); 
  corners[1] = Vector3(1, yofsb[3], 0); 
  corners[2] = Vector3(0, h[0],  0); 
  corners[3] = Vector3(1, h[3],  0); 

  corners[4] = Vector3(0, yofsb[1], 1); 
  corners[5] = Vector3(1, yofsb[2], 1); 
  corners[6] = Vector3(0, h[1],  1);
  corners[7] = Vector3(1, h[2],  1);

  verts.clear();
  
  if (visibility & (1<<Side::Right))    SideVerts(Side::Right,    verts);
  if (visibility & (1<<Side::Left))     SideVerts(Side::Left,     verts);
  if (visibility & (1<<Side::Up))       SideVerts(Side::Up,       verts, reversedTop);
  if (visibility & (1<<Side::Down))     SideVerts(Side::Down,     verts, reversedBottom);
  if (visibility & (1<<Side::Forward))  SideVerts(Side::Forward,  verts);
  if (visibility & (1<<Side::Backward)) SideVerts(Side::Backward, verts);
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
