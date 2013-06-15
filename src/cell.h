#ifndef BARFOOS_CELL_H
#define BARFOOS_CELL_H

#include "common.h"
#include "util.h"

class World;
class Random;
class Serializer;

enum CellFlags {
  Solid       = (1<<0),
  Transparent = (1<<1),
  Dynamic     = (1<<2),
  DoNotRender = (1<<3),
  Liquid      = (1<<4),
  Viscous     = (1<<5),
  MultiSided  = (1<<6),
  UVTurb      = (1<<7),
  Waving      = (1<<8),
};

struct CellInfo {
  std::vector<unsigned int> textures;
  IColor light;
  uint32_t flags;

  CellInfo():flags(0) {}
  CellInfo(const std::string &texture, const IColor &light, uint32_t flags = Solid);
  CellInfo(const std::string &texture, uint32_t flags = Solid);

  CellInfo(FILE *f);
};

class Cell {
public:

  Cell(const std::string &type = "default");
  virtual ~Cell();

  virtual void Update(float t, Random &random);
  virtual void Draw(std::vector<Vertex> &vertices) const;
  virtual bool UpdateNeighbours();
  
  const std::string &GetType() const;
  const CellInfo &GetInfo() const;

  uint8_t GetVisibility() const;
  void SetVisibility(uint8_t visibility);

  Cell &SetOrder(bool topReversed, bool bottomReversed);

  Cell &SetYOffsets(float a, float b, float c, float d);
  Cell &SetYOffsetsBottom(float a, float b, float c, float d);

  float GetHeight(float x, float z) const;
  float GetHeightBottom(float x, float z) const;

  Cell &SetTexture(unsigned int tex, bool multi = false);
  unsigned int GetTexture() const;

  bool IsTopFlat() const;
  bool IsBottomFlat() const;
  bool IsTransparent() const;
  bool IsSolid() const;

  void SetWorld(World *world);
  World *GetWorld() const;
  
  void SetPosition(const IVector3 &pos);
  IVector3 GetPosition() const;

  const IColor &GetLightLevel() const;
  bool SetLightLevel(const IColor &level, bool force=false);
  
  bool IsLocked() const;
  void SetLocked(bool locked);
  
  bool GetIgnoreLock() const;
  void SetIgnoreLock(bool ignore);
  
  bool GetIgnoreWrite() const;
  void SetIgnoreWrite(bool ignore);
  
  uint32_t GetDetail() const;
  void SetDetail(uint32_t detail);
  
  bool HasSolidSides() const;
  
  void SetDirty() { dirty = 1; }
  bool IsDirty() const { return dirty; }

  void UpdateVertices();
  void Tick(Random &random);
  
protected:

  // generic information
  World *world;
  IVector3 pos;
  float smoothDetail;
  const CellInfo *info;
  size_t dirty;
  
  std::string type;
  IColor lightLevel, torchLight;
  uint32_t detail;

  // map generation
  bool isLocked;    // disallow modification via World::SetCell...
  bool ignoreLock;  // ..unless this is true
  bool ignoreWrite; // World::SetCell will only pretend to succeed

  // rendering  
  bool reversedTop;
  bool reversedBottom;
  
  int8_t topHeights[4], bottomHeights[4];
  float u[4], v[4];
  uint8_t visibility;
  uint8_t visibilityOverride;
  unsigned int texture;
  float uscale;
  Vector3 corners[8];
  std::vector<Vertex> verts;
  
  Cell *neighbours[6];
  
  float YOfs(size_t n)  const { return topHeights[n]/32.0; }
  float YOfsb(size_t n) const { return bottomHeights[n]/32.0; }
  
  IColor SideCornerColor(Side side, size_t corner) const;
  void SideColors(Side side, IColor *colors) const;
  void SideVerts(Side side, std::vector<Vertex> &verts, bool reverse = false) const;

  bool Flow(Side side);
  
  friend Serializer &operator << (Serializer &ser, const Cell &cell);
};

Serializer &operator << (Serializer &ser, const Cell &);

inline void Cell::SetWorld(World *world) { 
  this->world = world; 
}

inline World *Cell::GetWorld() const { 
  return this->world; 
}

inline IVector3 Cell::GetPosition() const { 
  return this->pos; 
}

inline const CellInfo &Cell::GetInfo() const { 
  return *this->info; 
}

inline const std::string &Cell::GetType() const { 
  return this->type; 
}

inline uint8_t Cell::GetVisibility() const {
  return this->visibility; 
}

inline void Cell::SetVisibility(uint8_t visibility) { 
  this->visibility = visibility; 
}

inline unsigned int Cell::GetTexture() const { 
  return this->texture; 
}

inline bool Cell::IsTopFlat() const {
  return YOfs(0) == 1.0 && YOfs(1) == 1.0 && YOfs(2) == 1.0 && YOfs(3) == 1.0;
}

inline bool Cell::IsBottomFlat() const {
  return YOfsb(0) == 0.0 && YOfsb(1) == 0.0 && YOfsb(2) == 0.0 && YOfsb(3) == 0.0;
}

inline bool Cell::IsTransparent() const {
  return 
    this->GetInfo().flags & CellFlags::Transparent || 
    this->GetInfo().flags & CellFlags::DoNotRender || 
    !IsTopFlat() || !IsBottomFlat();
}

inline bool Cell::IsSolid() const {
  return this->info->flags & CellFlags::Solid;
}

inline const IColor &Cell::GetLightLevel() const { 
  return this->lightLevel; 
}

inline bool Cell::SetLightLevel(const IColor &level, bool force) { 
  // only update when changing
  if (!force && level == this->lightLevel) return false;
  
  lightLevel = level; 
  return true;
}

inline bool Cell::IsLocked() const { 
  return this->isLocked; 
}

inline void Cell::SetLocked(bool locked) { 
  this->isLocked = locked; 
}
  
inline bool Cell::GetIgnoreLock() const { 
  return this->ignoreLock; 
}

inline void Cell::SetIgnoreLock(bool ignore) { 
  this->ignoreLock = ignore; 
}
  
inline bool Cell::GetIgnoreWrite() const { 
  return this->ignoreWrite; 
}

inline void Cell::SetIgnoreWrite(bool ignore) { 
  this->ignoreWrite = ignore; 
}

inline uint32_t Cell::GetDetail() const { 
  return this->detail; 
}

inline void Cell::SetDetail(uint32_t detail) { 
  this->detail = detail;
}

void LoadCells();
bool IsCellTypeNameValid(const std::string &name);

#endif

