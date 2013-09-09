#include "cell.h"

#include "vertex.h"
#include "util.h"

#include "world.h"

static bool sidesDatasInited = false;

struct SideData {
  int idx[4];
  int tile;
  Vector3 uvec;
  Vector3 vvec;
};

static SideData sideData[6];

static void InitSideData(Side side) {
  Vector3 uvec;
  Vector3 vvec(0,1,0);
  
  int tile = 0;
  int inv = 0;
  int r, s, t;
  
  switch(side) {
    case Side::Right:    r = Corner100; s = CornerY; t = CornerZ; uvec.z = 1.0; break;
    case Side::Left:     r = Corner000; s = CornerY; t = CornerZ; uvec.z = 1.0; inv = CornerZ; tile = 1; break;
    case Side::Forward:  r = Corner001; s = CornerY; t = CornerX; uvec.x = 1.0; inv = CornerX; tile = 2; break;
    case Side::Backward: r = Corner000; s = CornerY; t = CornerX; uvec.x = 1.0; tile = 3; break;
    case Side::Up:       r = Corner010; s = CornerZ; t = CornerX; uvec.x = 1.0; vvec = Vector3(0,0,1); tile = 4; break;
    case Side::Down:     r = Corner000; s = CornerZ; t = CornerX; uvec.x = 1.0; vvec = Vector3(0,0,1); inv = CornerZ; tile = 5; break;
    default: return;
  }
  
  sideData[(int)side].tile = tile;
  
  sideData[(int)side].idx[0] = (r    )^inv;
  sideData[(int)side].idx[1] = (r+s  )^inv;
  sideData[(int)side].idx[2] = (r+s+t)^inv;
  sideData[(int)side].idx[3] = (r+  t)^inv;
  
  sideData[(int)side].uvec = uvec;
  sideData[(int)side].vvec = vvec;
}

static void InitSideData() {
  for (size_t s = 0; s<6; s++) {
    InitSideData((Side)s);
  }
  sidesDatasInited = true;
}

static const SideData &GetSideData(Side s) {
  if (!sidesDatasInited) InitSideData();
  return sideData[(int)s];
}

/** Get all 4 corner colors for a given side.
  * @param side Side to use.
  * @param[out] colors Where to store the colors.
  */
void 
CellRender::SideColors(Side side, IColor *colors) const {
  colors[0] = this->SideCornerColor(side, 0);
  colors[1] = this->SideCornerColor(side, 1);
  colors[2] = this->SideCornerColor(side, 2);
  colors[3] = this->SideCornerColor(side, 3);
}

/** Set normal texture.
  * @param tex Texture to use.
  * @param multi If true, texture contains multiple sides.
  */
void
CellRender::SetTexture(const Texture *tex, bool multi) {
  if (multi) {
    this->uscale = 1.0/8.0;
  } else {
    this->uscale = 1.0;
  }
  this->texture = tex; 
}

/** Set emissive normal texture.
  * @param tex Texture to use.
  */
void
CellRender::SetEmissiveTexture(const Texture *tex) {
  this->emissiveTexture = tex; 
}

void
CellRender::SideVerts(Side side, std::vector<Vertex> &verts, bool reverse) const {
  bool drawA = true, drawB = true;

  const SideData &data = GetSideData(side);

  Vector3 pos[4] = { corners[data.idx[0]],    corners[data.idx[1]],    corners[data.idx[2]],    corners[data.idx[3]] };

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
    this->shared.u[0] + (pos[0].Dot(data.uvec) + data.tile) * this->uscale, 
    this->shared.u[1] + (pos[1].Dot(data.uvec) + data.tile) * this->uscale, 
    this->shared.u[2] + (pos[2].Dot(data.uvec) + data.tile) * this->uscale, 
    this->shared.u[3] + (pos[3].Dot(data.uvec) + data.tile) * this->uscale 
  };
  
  float v[4] = { 
    pos[0].Dot(data.vvec) + this->shared.v[0], 
    pos[1].Dot(data.vvec) + this->shared.v[1],
    pos[2].Dot(data.vvec) + this->shared.v[2],
    pos[3].Dot(data.vvec) + this->shared.v[3] 
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
  
  IColor colors[4];/*
  = {
    this->world->GetLight(pos[0]+this->pos),
    this->world->GetLight(pos[1]+this->pos),
    this->world->GetLight(pos[2]+this->pos),
    this->world->GetLight(pos[3]+this->pos)
  };*/
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
CellRender::SideColors(Side side, std::vector<IColor> &outcolors, bool reverse) const {
  bool drawA = true, drawB = true;

  const SideData &data = GetSideData(side);

  Vector3 pos[4] = { corners[data.idx[0]],    corners[data.idx[1]],    corners[data.idx[2]],    corners[data.idx[3]] };

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

  IColor colors[4];
  SideColors(side, colors);
  bool doubleSided = info->flags & CellFlags::DoubleSided;

  if (reverse) {
    if (drawA) {
      outcolors.push_back(colors[0]);
      outcolors.push_back(colors[1]);
      outcolors.push_back(colors[3]);
      
      if (doubleSided) {
        outcolors.push_back(colors[3]);
        outcolors.push_back(colors[1]);
        outcolors.push_back(colors[0]);
      }
    }

    if (drawB) {
      outcolors.push_back(colors[1]);
      outcolors.push_back(colors[2]);
      outcolors.push_back(colors[3]);
      
      if (doubleSided) {
        outcolors.push_back(colors[3]);
        outcolors.push_back(colors[2]);
        outcolors.push_back(colors[1]);
      }
    }
  } else {
    if (drawA) {
      outcolors.push_back(colors[0]);
      outcolors.push_back(colors[1]);
      outcolors.push_back(colors[2]);
      
      if (doubleSided) {
        outcolors.push_back(colors[2]);
        outcolors.push_back(colors[1]);
        outcolors.push_back(colors[0]);
      }
    }

    if (drawB) {
      outcolors.push_back(colors[0]);
      outcolors.push_back(colors[2]);
      outcolors.push_back(colors[3]);
      
      if (doubleSided) {
        outcolors.push_back(colors[3]);
        outcolors.push_back(colors[2]);
        outcolors.push_back(colors[0]);
      }
    }
  }
}

bool
CellRender::UpdateVertices() {
  if (!visibility || (this->info->flags & CellFlags::DoNotRender)) return false;
  
  if (!this->vertsDirty) {
    this->UpdateColors();
    return false;
  }
  if ((this->info->flags & CellFlags::Dynamic) == 0) this->vertsDirty = false;

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
  
  const float &scaleX = shared.scale.x;
  const float &scaleY = shared.scale.y;
  const float &scaleZ = shared.scale.z;
  
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
  
  return true;
}

void
CellRender::UpdateColors() {
  if (!visibility || (this->info->flags & CellFlags::DoNotRender)) return;
  if (!this->colorDirty) return;
  if ((this->info->flags & CellFlags::Dynamic) == 0) this->colorDirty = false;
  
  std::vector<IColor> colors;
  
  if (visibility & (1<<Side::Right))    SideColors(Side::Right,    colors, this->reversedSides);
  if (visibility & (1<<Side::Left))     SideColors(Side::Left,     colors, this->reversedSides);
  if (visibility & (1<<Side::Up))       SideColors(Side::Up,       colors, this->shared.reversedTop);
  if (visibility & (1<<Side::Down))     SideColors(Side::Down,     colors, this->shared.reversedBottom);
  if (visibility & (1<<Side::Forward))  SideColors(Side::Forward,  colors, this->reversedSides);
  if (visibility & (1<<Side::Backward)) SideColors(Side::Backward, colors, this->reversedSides);
  
  if (colors.size() != verts.size()) {
    Log("color count %u doesn't match vertex count %u! %u\n", colors.size(), verts.size(), info->flags & CellFlags::DoubleSided);
    return;
  }
  
  for (size_t i=0; i<verts.size(); i++) {
    verts[i].SetColor(colors[i]);
  }
}
