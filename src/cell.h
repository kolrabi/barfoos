#ifndef BARFOOS_CELL_H
#define BARFOOS_CELL_H

#include "common.h"
#include "icolor.h"

class World;
class Random;
class Serializer;
class Game;

struct Vertex;

/** Optional flags of a cell. */
enum CellFlags {
  /** Enable collision detection. */
  Solid       = (1<<0),
  
  /** Pass light through and always draw adjacent cells sides. */
  Transparent = (1<<1),
  
  /** Don't put cell in static vertex buffer, it's vertices will change. */
  Dynamic     = (1<<2),
  
  /** Don't bother rendering this cell. */
  DoNotRender = (1<<3),
  
  /** Cell behaves like a liquid and flows, detail is amount of liquid in cell. @TODO higher friction than air. */
  Liquid      = (1<<4),
  
  /** If liquid, make the cell flow slower. @TODO More friction for mobs inside. */
  Viscous     = (1<<5),
  
  /** Use a different part of the texture for each side. 
    * Texture must contain eight parts, one for each side from left to right:
    * Left, Right, Back, Front, Top, Bottom, and 2 unused.
    */    
  MultiSided  = (1<<6),
  
  /** Apply a turbulence effect on the texture coordinates of the cell. */
  UVTurb      = (1<<7),
  
  /** Make the top surface wave. */
  Waving      = (1<<8),
};

/** Information about a cell shared by cells of same type. */
struct CellInfo {
  /** An array of textures to randomly choose from on cell creation. */
  std::vector<const Texture *> textures;
  
  /** Light emission from this kind of cell. 
    * Default: Black, no light is emitted. 
    */
  IColor light;
  
  /** Default flags for cell. @see CellFlags.
    * Default: Nonsolid, nontransparent, render.
    */
  uint32_t flags = 0;
  
  /** Light attenuation factor of light passing through this cell. 
    * This is only used for transparent cells. Nontransparent cells 
    * always have a factor of 0.
    * Default: 85%.
    */
  float lightFactor = 0.85;
  
  /** Light reduction value. This value is subtracted from all components of light passing through. 
    * Default: 0, don't reduce light.
    */
  int lightFade = 0;

  /** Name of cell type with which to replace this cell under certain conditions. 
    * Default: "", don't replace.
    */
  std::string replace;
  
  /** If nonzero, the chance per tick to replace this cell. 
    * Default: 0.0, don't replace.
    */
  float replaceChance = 0.0;
  
  /** If nonzero, replace this cell when cell detail goes below this value. 
    * Default: 0, don't replace.
    */
  size_t detailBelowReplace = 0;
  
  /** Rendering scale of this cell. Has no effect on collision detection. 
    * Default: [1,1,1], don't change size.
    */
  Vector3 scale = Vector3(1,1,1);

/*  
  size_t noclipSidesIn = 0;     // default: never allow moving in from any side when solid
  size_t noclipSidesOut = ~0;   // default: always allow moving out to any side when solid
*/

  CellInfo() {}
  CellInfo(FILE *f);
};

/** A cell in the world. */
class Cell {
public:

  Cell(const std::string &type = "default");
  virtual ~Cell();

  virtual void Update(Game &game);
  virtual bool UpdateNeighbours();
  
  virtual void Draw(std::vector<Vertex> &vertices) const;
  virtual void DrawHighlight(std::vector<Vertex> &vertices) const;
  
  const std::string &GetType() const;
  const CellInfo &GetInfo() const;

  uint8_t GetVisibility() const;
  void SetVisibility(uint8_t visibility);

  Cell &SetOrder(bool topReversed, bool bottomReversed);

  Cell &SetYOffsets(float a, float b, float c, float d);
  Cell &SetYOffsetsBottom(float a, float b, float c, float d);

  float GetHeight(float x, float z) const;
  float GetHeightBottom(float x, float z) const;
  float GetHeightClamp(float x, float z) const;
  float GetHeightBottomClamp(float x, float z) const;

  Cell &SetTexture(const Texture *tex, bool multi = false);
  const Texture *GetTexture() const;

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

  size_t GetFeatureID() const;
  void SetFeatureID(size_t f);
  bool IsFeatureBorder() const;
  
  bool HasSolidSides() const;
  bool CheckSideSolid(Side side, const Vector3 &org) const;
  
  void SetDirty() { dirty = 1; }
  bool IsDirty() const { return dirty; }
  
  AABB GetAABB() const;

  void UpdateVertices();
  void Tick(Game &game);
  
  bool Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const;
  
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
  size_t featureID;

  // rendering  
  bool reversedTop;
  bool reversedBottom;
  bool reversedSides;
  
  int8_t topHeights[4], bottomHeights[4];
  float u[4], v[4];
  uint8_t visibility;
  uint8_t visibilityOverride;
  const Texture *texture;
  float uscale;
  Vector3 corners[8];
  std::vector<Vertex> verts;
  
  size_t tickPhase;
  size_t tickInterval;
  
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

inline const Texture *Cell::GetTexture() const { 
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

inline size_t Cell::GetFeatureID() const { 
  return this->featureID; 
}

inline void Cell::SetFeatureID(size_t f) { 
  this->featureID = f;
}

void LoadCells();
bool IsCellTypeNameValid(const std::string &name);

#endif

