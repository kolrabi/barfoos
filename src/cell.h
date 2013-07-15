#ifndef BARFOOS_CELL_H
#define BARFOOS_CELL_H

#include "common.h"
#include "icolor.h"
#include "ivector3.h"

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
  
  /** Cell behaves like a liquid and flows, detail is amount of liquid in cell. */
  Liquid      = (1<<4),
  
  /** If liquid, make the cell flow slower. */
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
  
  DoubleSided = (1<<9),
  Pickable    = (1<<10),
  OnUseReplace = (1<<11),
};

/** Information about a cell shared by cells of same type. */
struct CellInfo {
  /** Name of cell type. */
  std::string type = "default";
  
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
  
  float speedModifier = 1.0;
  float friction = 1.0;

  size_t showSides = 0;
  size_t hideSides = 0;
  size_t clipSidesIn = 0;  // default: don't clip movement into cell from all sides when solid
  size_t clipSidesOut = 0; // default: don't clip movement out of cell to all sides when solid
  
  size_t onUseCascade = 0;
  float useDelay = 0.0;
  
  float breakStrength = 1.0;
  
  float lavaDamage = 0.0;

  CellInfo() {}
  CellInfo(const std::string &name, FILE *f);
  
  bool operator==(const CellInfo &that) const {
    return this == &that;
  }

  bool operator!=(const CellInfo &that) const {
    return this != &that;
  }
};

/** A cell in the world. */
class Cell {
public:

  Cell(const std::string &type = "default");
  Cell(const Cell &that);
  ~Cell();
  
  void Update(Game &game);
  
  void OnUse(Game &game, Mob &user);
  
  virtual void UpdateNeighbours();
  
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
  bool IsLiquid() const;

  void SetWorld(World *world, const IVector3 &pos);
  World *GetWorld() const;
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
  
  bool HasSolidSides() const;
  bool CheckSideSolid(Side side, const Vector3 &org) const;
  
  void SetDirty() { dirty = true; }
  
  AABB GetAABB() const;

  void UpdateVertices();
  void Tick(Game &game);
  
  bool Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const;
  Cell &operator[](Side side);
  
protected:

  static const int OffsetScale = 127;

  // unique information, that will change after assignment from different cell
  const CellInfo *info;
  World *world;
  IVector3 pos;
  bool dirty;
  
  size_t tickPhase;
  IColor lightLevel;
  float lastT;
  
  uint8_t visibility;
  bool reversedSides;
  Vector3 corners[8];
  std::vector<Vertex> verts;
  Cell *neighbours[6];
  const Texture *texture;
  float uscale;
  
  float lastUseT;

  // shared information, that will stay the same after assignment from different cell
  struct {
    size_t tickInterval;
    
    // map generation
    bool isLocked;    // disallow modification via World::SetCell...
    bool ignoreLock;  // ..unless this is true
    bool ignoreWrite; // World::SetCell will only pretend to succeed
    size_t featureID;

    // rendering  
    bool reversedTop;
    bool reversedBottom;
    
    uint8_t visibilityOverride;
    int8_t topHeights[4], bottomHeights[4];
    float u[4], v[4];

    uint32_t detail;
    float smoothDetail;
    
    uint32_t nextDetail;
  } shared;
  
  float YOfs(size_t n)  const { return this->shared.topHeights[n]/(float)OffsetScale; }
  float YOfsb(size_t n) const { return this->shared.bottomHeights[n]/(float)OffsetScale; }
  
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
  return this->info->type; 
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

inline bool Cell::IsLiquid() const {
  return this->info->flags & CellFlags::Liquid;
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
  return this->shared.isLocked; 
}

inline void Cell::SetLocked(bool locked) { 
  this->shared.isLocked = locked; 
}
  
inline bool Cell::GetIgnoreLock() const { 
  return this->shared.ignoreLock; 
}

inline void Cell::SetIgnoreLock(bool ignore) { 
  this->shared.ignoreLock = ignore; 
}
  
inline bool Cell::GetIgnoreWrite() const { 
  return this->shared.ignoreWrite; 
}

inline void Cell::SetIgnoreWrite(bool ignore) { 
  this->shared.ignoreWrite = ignore; 
}

inline uint32_t Cell::GetDetail() const { 
  return this->shared.detail; 
}

inline void Cell::SetDetail(uint32_t detail) { 
  this->shared.detail = detail;
}

inline size_t Cell::GetFeatureID() const { 
  return this->shared.featureID; 
}

inline void Cell::SetFeatureID(size_t f) { 
  this->shared.featureID = f;
}

inline Cell &Cell::operator[](Side side) {
  return *this->neighbours[(int)side];
}

void LoadCells();
bool IsCellTypeNameValid(const std::string &name);

#endif

