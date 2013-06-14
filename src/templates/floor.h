#ifndef TEMPLATE_TEMPLATES_FLOOR_H
#define TEMPLATE_TEMPLATES_FLOOR_H

#include "template.h"
#include "simplex.h"
#include "cell.h"
#include "world.h"

class FloorTemplate : public Template {
public:
  FloorTemplate(const std::string &type, float scale = 0.5, float height = 0.5) : 
    type(type), scale(scale), height(height) {}

  virtual IVector3 GetSize() const { return IVector3(1,1,1); }
  virtual void Apply(const IVector3 &origin, World &world) const {
    world.SetCell(origin, Cell(type)).SetYOffsets(
      simplexNoise(Vector3(origin.x,   origin.y, origin.z  )*scale)*height+1.0-height,
      simplexNoise(Vector3(origin.x,   origin.y, origin.z+1)*scale)*height+1.0-height,
      simplexNoise(Vector3(origin.x+1, origin.y, origin.z+1)*scale)*height+1.0-height,
      simplexNoise(Vector3(origin.x+1, origin.y, origin.z  )*scale)*height+1.0-height
    );
  }

private:

  std::string type;
  float scale, height;
};

#endif

