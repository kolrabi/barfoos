#ifndef BARFOOS_WORLDEDIT_H
#define BARFOOS_WORLDEDIT_H

#include "common.h"
#include "cell.h"
#include "world.h"
#include "template.h"

class World;

class WorldEdit {
public:

  WorldEdit(World *world);
  WorldEdit(const std::shared_ptr<World> &world);
  ~WorldEdit();
 
  WorldEdit &SetBrush(const Cell &brush) { this->brush = brush; return *this; }
  WorldEdit &ApplyBrush(const IVector3 &pos) { this->world->SetCell(pos, this->brush); return *this; }

  WorldEdit &LineX(const IVector3 &pos, size_t length);
  WorldEdit &LineY(const IVector3 &pos, size_t length);
  WorldEdit &LineZ(const IVector3 &pos, size_t length);

  WorldEdit &WallXY(const IVector3 &pos, size_t height, size_t depth);
  WorldEdit &WallXZ(const IVector3 &pos, size_t height, size_t depth);
  WorldEdit &WallYZ(const IVector3 &pos, size_t height, size_t depth);

  WorldEdit &FilledBox(const IVector3 &pos, const IVector3 &size);
  
  WorldEdit &Explosion(const IVector3 &pos, const IVector3 &size, float strength);
  WorldEdit &Ellipsoid(const IVector3 &pos, const IVector3 &size);

private:

  World *world;
  Cell brush;
};

#endif

