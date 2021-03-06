#ifndef BARFOOS_CELLPROPERTIES_H
#define BARFOOS_CELLPROPERTIES_H

#include "io/properties.h"

#include <unordered_map>

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

  /** Render back sides. */
  DoubleSided = (1<<9),

  /** Items can be used on this. */
  Pickable    = (1<<10),

  /** If used, replace this cell. */
  OnUseReplace = (1<<11),

  /** Mobs may climb upwards when inside this cell. */
  Ladder = (1<<12)
};

/** Information about a cell shared by cells of same type. */
struct CellProperties : public Properties {
  /** Name of cell type. */
  std::string type;

  /** An array of textures to randomly choose from when cell is added to world. */
  std::vector<const Texture *> textures;

  /** An array of textures to use for emission. */
  std::vector<const Texture *> emissiveTextures;

  /** Texture to use when cell is "active". */
  std::string activeTexture;

  /** Emissive texture to use when cell is "active". */
  std::string emissiveActiveTexture;

  /** Light emission from this kind of cell.
    * Default: Black, no light is emitted.
    */
  IColor light;

  /** Default flags for cell. @see CellFlags.
    * Default: Nonsolid, nontransparent, render.
    */
  uint32_t flags;

  /** Light attenuation factor of light passing through this cell.
    * This is only used for transparent cells. Nontransparent cells
    * always have a factor of 0.
    * Default: 85%.
    */
  float lightFactor;

  /** Light reduction value. This value is subtracted from all components of light passing through.
    * Default: 0, don't reduce light.
    */
  int lightFade;

  /** Name of cell type with which to replace this cell under certain conditions.
    * Default: "", don't replace.
    */
  std::string replace;

  /** If nonzero, the chance per tick to replace this cell.
    * Default: 0.0, don't replace.
    */
  float replaceChance;

  /** If nonzero, replace this cell when cell detail goes below this value.
    * Default: 0, don't replace.
    */
  uint32_t detailBelowReplace;

  /** Item replacement on use: olditem -> newitem. */
  std::unordered_map<std::string, std::string> onUseItemReplaceItem;

  /** Cell replacement on use: item -> newcell. */
  std::unordered_map<std::string, std::string> onUseItemReplace;

  /** Cell liquid update on use: item -> detaildiff. */
  std::unordered_map<std::string, int> onUseItemAddDetail;

  /** Rendering scale of this cell. Has no effect on collision detection.
    * Default: [1,1,1], don't change size.
    */
  Vector3 scale;

  /** Movement speed factor. */
  float speedModifier;

  /** Friction factor. */
  float friction;

  /** Always show these sides. */
  uint32_t showSides;

  /** Always hide these sides (overrides showSides). */
  uint32_t hideSides;

  /** Clip movement into cell from these sides. */
  uint32_t clipSidesIn;  // default: don't clip movement into cell from all sides when solid

  /** Clip movement out of cell towards these sides. */
  uint32_t clipSidesOut; // default: don't clip movement out of cell to all sides when solid

  /** On use, also trigger use in cells on these sides. */
  uint32_t onUseCascade;

  /** Time delay between uses. */
  float useDelay;

  /** Block break resistance. */
  float breakStrength;

  /** Lava damage per second. */
  float lavaDamage;

  /** Chance of successful use. */
  float useChance;

  /** Particle entity to use when cell breaks. */
  std::string breakParticle;

  /** Replace when flowing onto something: targetcelltype -> newtargetcelltype. */
  std::unordered_map<std::string, std::string> onFlowOntoReplaceTarget;

  /** Replace when flowing onto something: targetcelltype -> newowncelltype. */
  std::unordered_map<std::string, std::string> onFlowOntoReplaceSelf;

  /** Chance of this cell to be initially locked. (Cascades into onUseCascade neighbours.) */
  float lockedChance;

  /** A list of sounds: soundType -> soundfile. */
  std::unordered_map<std::string, std::string> sounds;

  CellProperties();

  virtual void ParseProperty(const std::string &name);
};

void LoadCells();
const CellProperties &GetCellProperties(const std::string &type);

#endif

