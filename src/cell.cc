#include "common.h"

#include "cell.h"
#include "world.h"

#include "gfx.h"
#include "texture.h"

#include "audio.h"

#include "runningstate.h"
#include "player.h"
#include "item.h"

#include "vertex.h"

// -------------------------------------------------------------------------

/** C'tor.
  * @param type Cell type.
  */
CellBase::CellBase(const std::string &type) :
  Triggerable(),
  info(&GetCellProperties(type)),
  world(nullptr),
  pos(0,0,0),
  tickInterval( info->flags & CellFlags::Viscous ? 32 : 5 ),
  proto()
{
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;

  this->vertsDirty = false;
  this->colorDirty = false;
}
 
CellBase::CellBase(const CellBase &that) :
  Triggerable(that),
  info(that.info),
  world(nullptr),
  pos(0,0,0),
  tickInterval( info->flags & CellFlags::Viscous ? 32 : 5 ),
  proto(that.proto)
{
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;

  this->vertsDirty = false;
  this->colorDirty = false;
}

CellBase::CellBase(const Cell_Proto &proto) :
  Triggerable(),
  info(&GetCellProperties(proto.type())),
  world(nullptr),
  pos(0,0,0),
  tickInterval( info->flags & CellFlags::Viscous ? 32 : 5 ),
  proto(proto)
{
  for (size_t i=0; i<6; i++)
    this->neighbours[i] = nullptr;

  this->vertsDirty = false;
  this->colorDirty = false;
}

/** Lock a cell (for use with a key).
  * @param id Lock/key ID.
  */
void
CellBase::Lock(uint32_t id) {
  if (this->GetLockID() == id) return;

  this->SetLockID(id);

  for (int i=0; i<6; i++) {
    if (info->onUseCascade & (1<<i) && this->neighbours[i]->info == info)
      this->neighbours[i]->Lock(id);
  }
}

/** Unlock a cell (with a key).
  * Also unlocks neighbours on sides marked onUseCascade.
  */
void
CellBase::Unlock() {
  if (!this->IsLocked()) return;

  this->ClearLockID();

  for (int i=0; i<6; i++) {
    if (info->onUseCascade & (1<<i) && this->neighbours[i]->info == info)
      this->neighbours[i]->Unlock();
  }
}

void
CellBase::SetReversed(bool top, bool bottom) { 
  this->proto.set_is_top_reversed(top); 
  this->proto.set_is_bottom_reversed(bottom); 
  if (this->world && !this->IsDynamic()) 
    this->world->MarkForUpdateNeighbours(&this->world->GetCell(this->GetPosition()));
}

// -------------------------------------------------------------------------

/** C'tor.
  * @param type Cell type.
  */
Cell::Cell(const std::string &type) :
  CellRender(type),
  tickPhase(0)
{
}

/** Copy c'tor.
  * @param that Cell from which to copy.
  */
Cell::Cell(const Cell &that) :
  CellRender(that),
  tickPhase(0)
{
}

Cell::Cell(const Cell_Proto &proto) :
  CellRender(proto),
  tickPhase(0)
{
  Log("%s\n", __PRETTY_FUNCTION__);
}

/** D'tor. */
Cell::~Cell() {
}

/** Update this cell.
  * @param state The running game state.
  */
void
Cell::Update(
  RunningState &state
) {
  if (!world) return;

  float t = state.GetGame().GetTime();

  this->uvTime = t;

  // Spawn entities if activated
  if (this->HasSpawnOnActive() && this->IsTriggered() && t > this->GetNextActivationTime()) {
    // spawn in adjacent cell
    Entity *entity = state.GetEntity(state.SpawnInAABB(this->GetSpawnOnActiveMobType(), self[this->GetSpawnOnActiveSide()].GetAABB()));

    // if it is a projectile, set velocity
    Mob *mob = dynamic_cast<Mob*>(entity);
    if (mob) mob->SetVelocity(Vector3(this->GetSpawnOnActiveSide()) * mob->GetProperties()->maxSpeed);

    if (this->GetSpawnOnActiveRate() == 0.0) {
      // spawn only once
      this->TriggerOff();
    } else {
      // spawn again a bit later
      this->SetNextActivationTime(t + this->GetSpawnOnActiveRate());
    }
  }

  // update liquids
  if (this->IsLiquid()) {
    if (this->GetLiquidAmount() == 0) {
      // this cell lost all its liquid, replace by air
      this->world->SetCell(this->pos, Cell("air"));

      // "this" is now the new air cell
    } else {
      // update heights
      float h = this->GetLiquidAmount()/16.0;

      if (self[Side::Up].info == info && self[Side::Down].info == info) {
        // liquid and top and bottom cells are the same as this one
        this->SetTopHeights(1,1,1,1);
        this->SetBottomHeights(0,0,0,0);
      } else if (self[Side::Up].info == info && self[Side::Down].info != info) {
        // liquid and liquid above and nothing liquid below
        this->SetTopHeights(1,1,1,1);
        this->SetBottomHeights(1-h,1-h,1-h,1-h);
      } else {
        // no liquid above
        this->SetBottomHeights(0,0,0,0);
        this->SetTopHeights(h,h,h,h);
      }
    } 
  }
}

/** Let a mob interact with this cell without an item.
  * @param state Running state.
  * @param user Mob that is using the cell.
  * @param force If true ignore the cell's useChance parameter.
  */
void
Cell::OnUse(RunningState &state, Mob &user, bool force) {
  // enforce delay
  if (state.GetGame().GetTime() - this->GetLastUseTime() < this->info->useDelay) 
    return;

  // enforce chance
  if (!force && !state.GetRandom().Chance(this->info->useChance)) 
    return;

  // update use time
  this->SetLastUseTime(state.GetGame().GetTime());

  // check if it is locked
  if (this->IsLocked()) {
    state.GetPlayer().AddMessage("It is locked!");
    PlaySound(state, "use.locked");
    return;
  }

  // replace cell if wanted
  const CellProperties *info = this->info;
  if (info->flags & CellFlags::OnUseReplace) {
    this->world->SetCell(GetPosition(), Cell(info->replace)).SetLastUseTime(state.GetGame().GetTime());
  }

  // cascade through neighbours
  for (int i=0; i<6; i++) {
    if (info->onUseCascade & (1<<i) && this->neighbours[i]->info == info)
      this->neighbours[i]->OnUse(state, user, true);
  }

  if (!force) PlaySound(state, "use");
}

/** Let a cell tick.
  * Updates tick phase. Let's liquid cells flow.
  * @param state Running state.
  */
void 
Cell::Tick(RunningState &state) {
  this->tickPhase = (this->tickPhase + 1) % this->tickInterval;
  if (this->tickPhase) return;

  if (this->neighbours[0] == nullptr) return;

  if ((this->info->flags & CellFlags::Liquid) && this->GetLiquidAmount() > 0) {
    if (self[Side::Up].info == self.info && this->GetLiquidAmount() < 16) return;

    // if we can't flow down and have more than 1 unit of liquid
    if (!this->Flow(Side::Down) && this->GetLiquidAmount() > 1) {
      // try flowing to one random side
      Side sides[4] = { Side::Left, Side::Right, Side::Forward, Side::Backward };
      int n = state.GetRandom().Integer(4);
      for (int i=0; i<4; i++) {
        if (this->Flow(sides[(n+i)%4])) {
          break;
        }
      }

      // if we have too much liquid, even allow flowing up
      if (this->GetLiquidAmount() > 16) this->Flow(Side::Up);
    }
  }

  // if this cell wants to be replaced if detail falls below a certain level
  if (info->detailBelowReplace && this->GetLiquidAmount() < this->info->detailBelowReplace && this->info->replace != "") {
    // check if we are connected to liquid neighbours
    bool liquidNeighbours = false;
    liquidNeighbours |= this->neighbours[(int)Side::Left]->info == this->info     && this->neighbours[(int)Side::Left]->GetLiquidAmount() > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Right]->info == this->info    && this->neighbours[(int)Side::Right]->GetLiquidAmount() > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Forward]->info == this->info  && this->neighbours[(int)Side::Forward]->GetLiquidAmount() > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Backward]->info == this->info && this->neighbours[(int)Side::Backward]->GetLiquidAmount() > info->detailBelowReplace;
    liquidNeighbours |= this->neighbours[(int)Side::Down]->info == this->info     && this->neighbours[(int)Side::Down]->GetLiquidAmount() > info->detailBelowReplace;

    // if not connected, take a chance and replace
    if (!liquidNeighbours && state.GetRandom().Chance(this->info->replaceChance)) {
      this->world->SetCell(GetPosition(), Cell(info->replace));
      // this is no longer valid
      return;
    }
  }
}

/** Try to flow a cell's content to one of its sides.
  * @param side Side to which to flow.
  * @return true if successful, false if not a liquid or side cell is solid.
  */
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
    if ((cell->GetLiquidAmount() >= this->GetLiquidAmount() && side != Side::Down) || cell->GetLiquidAmount() >= 16) return false;
  }

  this->SetLiquidAmount(this->GetLiquidAmount() - 1);

  if (cell->info != this->info) {
    // replace target cell if not already of this type
    this->world->SetCell(this->pos[side], Cell(info->type));
    cell->SetLiquidAmount(1);
  } else {
    // otherwise just increase liquid
    cell->SetLiquidAmount(cell->GetLiquidAmount() + 1);
  }

  world->MarkForUpdateNeighbours(this);
  world->MarkForUpdateNeighbours(cell);
  return true;
}

/** Update this cell because one or more of its neighbours has changed.
  */
void
Cell::UpdateNeighbours(
  size_t
) {
  if (this->world == nullptr) return;

  size_t oldvis = this->visibility;
  this->visibility = 0;

  // TODO: if liquid check sides for non liquid or different liquid amount

  // check if vertically out of box
  bool oversize = !this->IsTopFlat() || !this->IsBottomFlat();

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
      color = color.Max(cell.GetLightLevel());
    }

    // propagate light
    color = (color * this->info->lightFactor) - this->info->lightFade;

    // emit light
    color = color.Max(this->info->light);
  }


  // update this cell and neighbours recursively until nothing changes anymore
  bool lightChanged = this->SetLightLevel(color);
  bool visChanged = this->visibility != oldvis;
  bool updated = lightChanged || visChanged;
  if (updated) {
    updated = true;
    if (visChanged) {
      this->vertsDirty = true;
    }
    this->colorDirty |= lightChanged;
  }

  if (lightChanged) {
    for (size_t i=0; i<6; i++) {
      world->MarkForUpdateNeighbours(this->neighbours[i]);
    }

    // update corner cells as well
    this->world->GetCell(this->pos + IVector3( 1,  1,  1)).colorDirty = true;
    this->world->GetCell(this->pos + IVector3(-1,  1,  1)).colorDirty = true;
    this->world->GetCell(this->pos + IVector3( 1, -1,  1)).colorDirty = true;
    this->world->GetCell(this->pos + IVector3(-1, -1,  1)).colorDirty = true;
    this->world->GetCell(this->pos + IVector3( 1,  1, -1)).colorDirty = true;
    this->world->GetCell(this->pos + IVector3(-1,  1, -1)).colorDirty = true;
    this->world->GetCell(this->pos + IVector3( 1, -1, -1)).colorDirty = true;
    this->world->GetCell(this->pos + IVector3(-1, -1, -1)).colorDirty = true;
    for (size_t i=0; i<6; i++) {
      this->neighbours[i]->colorDirty = true;
    }
  }
}

/** Get the light color of a cell for a given side and corner.
  * @param side Side for which to get the light.
  * @param corner From 0 - 3 the index of the corner for the given side.
  * @return The light color. 
  */
IColor
CellRender::SideCornerColor(Side side, size_t corner) const {
  // not initialized yet?
  if (!world) return IColor();

  // is cell emitting light?
  if (!GetInfo().light.IsBlack()) {
    return GetInfo().light;
  }

  // for each corner of the side, average the four surrounding light
  // values.

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

// TODO: move to cellbase
/** Set top corner heights.
  * @param a Height of corner 0.
  * @param b Height of corner 1.
  * @param c Height of corner 2.
  * @param d Height of corner 3.
  * @return The updated cell.
  */
Cell &
Cell::SetTopHeights(float a, float b, float c, float d) {
  bool change = this->GetTopHeight(0) != a ||
                this->GetTopHeight(1) != b ||
                this->GetTopHeight(2) != c ||
                this->GetTopHeight(3) != d;
  if (!change) return *this;
                
  this->SetTopHeight(0, a);
  this->SetTopHeight(1, b);
  this->SetTopHeight(2, c);
  this->SetTopHeight(3, d);

  if (world && !IsDynamic()) 
    world->MarkForUpdateNeighbours(this);

  return *this;
}

// TODO: move to cellbase
/** Set bottom corner heights.
  * @param a Height of corner 0.
  * @param b Height of corner 1.
  * @param c Height of corner 2.
  * @param d Height of corner 3.
  * @return The updated cell.
  */
Cell &
Cell::SetBottomHeights(float a, float b, float c, float d) {
  bool change = this->GetBottomHeight(0) != a ||
                this->GetBottomHeight(1) != b ||
                this->GetBottomHeight(2) != c ||
                this->GetBottomHeight(3) != d;
  if (!change) return *this;
                
  this->SetBottomHeight(0, a);
  this->SetBottomHeight(1, b);
  this->SetBottomHeight(2, c);
  this->SetBottomHeight(3, d);

  if (world && !IsDynamic()) 
    world->MarkForUpdateNeighbours(this);

  return *this;
}

/** Check solidity of a cells side.
  * Also checks the solidity of the adjacent cell.
  * @param side Side to check.
  * @param org Origin vector to use for cell floor height check.
  * @param sneak If true, prevent going over ledges.
  * @return true if cell clips movement out of the given side or
  *         if the adjacent cell clips movement into it.
  */
bool
Cell::CheckSideSolid(Side side, const Vector3 &org, bool sneak) const {
  if (!this->world) return true;

  const Cell &cell = self[side];

  if (sneak) {
    if (side != Side::Up && side != Side::Down) {
      if (self[Side::Down].IsSolid() && !cell[Side::Down].IsSolid())
        return true;
    }
  }

  // check for clipping movement into cell from opposite side
  bool clipIn  = (cell.info->clipSidesIn  & (1 <<(-side)));

  // check for clipping movement out the cell
  bool clipOut = (this->info->clipSidesOut & (1 <<  side ));

  if (side == Side::Up || side == Side::Down) {
    return (clipIn || clipOut);
  }

  float x = org.x-(int)org.x + (side==Side::Left     ? 1.0 : 0.0);
  float z = org.z-(int)org.z + (side==Side::Backward ? 1.0 : 0.0);

  bool heightCheck = org.y < (this->pos.y+cell.GetHeightClamp( x, z ) );
  return (clipIn && heightCheck) || clipOut;
}

/** Set world and position.
  * This is comparable to Entity::Start. Selects a random texture.
  * Gets neighbour cells from world. Marks cell for update.
  * @param world World of which the cell is part.
  * @param pos Position of cell in world.
  */
void
Cell::SetWorld(World *world, const IVector3 &pos) {
  this->world = world;

  if (info->textures.empty()) {
    this->SetTexture(0, info->flags & MultiSided);
  } else {
    size_t idx = world->GetState().GetRandom().Integer(info->textures.size());
    this->SetTexture(info->textures[idx], info->flags & MultiSided);
    if (idx < info->emissiveTextures.size())
      this->SetEmissiveTexture(info->emissiveTextures[idx]);
  }

  if (this->info->activeTexture != "") this->activeTexture = Texture::Get("cells/texture/"+this->info->activeTexture);
  if (this->info->emissiveActiveTexture != "") this->emissiveActiveTexture = Texture::Get("cells/texture/"+this->info->emissiveActiveTexture);

  this->pos = pos;
  for (size_t i=0; i<6; i++) {
    this->neighbours[i] = &this->world->GetCell(pos[(Side)i]);
  }

  for (size_t i=0; i<6; i++) {
    world->MarkForUpdateNeighbours(this->neighbours[i]);
  }

  world->MarkForUpdateNeighbours(this);
}

/** Get the bounding box for this cell in world space.
  * @return The bounding box.
  */
AABB
Cell::GetAABB() const {
  AABB aabb;
  aabb.center = Vector3(this->pos) + Vector3(0.5,0.5,0.5);
  aabb.extents = Vector3(0.5,0.5,0.5);
  return aabb;
}

/** Check whether the player has seen this cell yet.
  * Compares the cell's feature id with the list of all seen features.
  * @return true if cell was seen.
  */
bool
Cell::IsSeen(size_t) const {
  if (!this->world) return false;
  if (this->GetFeatureID() == InvalidID) return false;

  if (!this->visibility && (info->flags & CellFlags::DoNotRender) == 0) return false;
  return this->world->GetMap().IsFeatureSeen(this->GetFeatureID());
}

/** Cast a ray against the triangles of the cell.
  * @param start Ray start point.
  * @param dir   Ray direction vector.
  * @param[out] t Time of hit if hit.
  * @param[out] p Position of hit if hit.
  * @return true if ray hit the cell, false otherwise.
  */
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

    if (Vector3::TriangleRay(tri, start, dir, ttt, pp)) {
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

/** Handle a mob stepping on the cell.
  * Takes care of teleportation and triggering.
  * @param state Running state.
  * @param mob Mob that stepped on the cell.
  */
void
Cell::OnStepOn(RunningState &state, Mob &mob) {
  if (this->IsTeleport() && state.GetGame().GetTime() > this->GetNextActivationTime()) {
    IVector3 target(this->world->GetCellPos(this->GetTeleportTarget()));
    Cell &targetCell = state.GetWorld().GetCell(target);

    this->SetNextActivationTime(state.GetGame().GetTime() + 2);
    targetCell.SetNextActivationTime(state.GetGame().GetTime() + 2);

    mob.Teleport(state, Vector3(target));
    this->PlaySound(state, "teleport");
    targetCell.PlaySound(state, "teleport");
  }

  if (this->IsTrigger()) {
    state.TriggerOn(this->GetTriggerTarget());
    PlaySound(state, "trigger.on");
  }
}

/** Handle a mob stepping off the cell.
  * Takes care of triggering.
  * @param mob Mob that stepped off. (unused)
  * @param state Running state.
  */
void
Cell::OnStepOff(RunningState &state, Mob &) {
  if (this->IsTrigger()) {
    state.TriggerOff(this->GetTriggerTarget());
    PlaySound(state, "trigger.off");
  }
}

/** Handle a mob using an item on the cell.
  * Replaces items or the cell if needed. Adds or removes liquid.
  * @param state Running state. (unused)
  * @param user Mob that uses the item. (unused)
  * @param item Item that was used.
  */
void
Cell::OnUseItem(RunningState &, Mob &, Item &item) {
  std::string itemType = item.GetType();

  auto iter1 = this->info->onUseItemReplaceItem.find(itemType);
  if (iter1 != this->info->onUseItemReplaceItem.end()) {
    item.ReplaceWith(iter1->second);
  }

  auto iter2 = this->info->onUseItemAddDetail.find(itemType);
  if (iter2 != this->info->onUseItemAddDetail.end()) {
    if (iter2->second < 0 && (int)(this->GetLiquidAmount()) < -iter2->second) {
      this->SetLiquidAmount(0);
    } else {
      this->SetLiquidAmount(this->GetLiquidAmount() + iter2->second);
    }
  }

  auto iter3 = this->info->onUseItemReplace.find(itemType);
  if (iter3 != this->info->onUseItemReplace.end()) {
    this->world->SetCell(this->GetPosition(), Cell(iter3->second));
  }
}

/** Rotate a cell by 90 degrees. 
  * Used for rotated features.
  */
void
Cell::Rotate() {
  float tmp;

  tmp = this->GetTopHeight(0);
  this->SetTopHeight(0, this->GetTopHeight(1));
  this->SetTopHeight(1, this->GetTopHeight(2));
  this->SetTopHeight(2, this->GetTopHeight(3));
  this->SetTopHeight(3, tmp);

  tmp = this->GetBottomHeight(0);
  this->SetBottomHeight(0, this->GetBottomHeight(1));
  this->SetBottomHeight(1, this->GetBottomHeight(2));
  this->SetBottomHeight(2, this->GetBottomHeight(3));
  this->SetBottomHeight(3, tmp);

  this->SetScale(this->GetScale().ZYX());
  this->SetSideReversed(!this->IsSideReversed());
  this->SetReversed(!this->IsTopReversed(), !this->IsBottomReversed());
}

/** Play a sound at the cell's position.
  * @param state Running state.
  * @param type Type of sound to play.
  */
void
Cell::PlaySound(RunningState &state, const std::string &type) {
  if (this->info->sounds.find(type) == this->info->sounds.end()) return;
  state.GetGame().GetAudio().PlaySound(this->info->sounds.at(type), GetAABB().center);
}
