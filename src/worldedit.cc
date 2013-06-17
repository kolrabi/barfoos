#include "worldedit.h"
#include "world.h"

WorldEdit::WorldEdit(World *world) : world(world) {}
WorldEdit::WorldEdit(const std::shared_ptr<World> &world) : WorldEdit(world.get()) {}
WorldEdit::~WorldEdit() {}

WorldEdit &WorldEdit::SetBrush(const Cell &brush) {
  this->brush = brush; 
  return *this; 
}

WorldEdit &WorldEdit::ApplyBrush(const IVector3 &pos) { 
  this->world->SetCell(pos, this->brush); 
  return *this; 
}

WorldEdit &WorldEdit::LineX(const IVector3 &pos, size_t length) {
  for (size_t x = 0; x<length && x+pos.x<world->GetSize().x; x++)
    ApplyBrush(IVector3(pos.x+x, pos.y, pos.z));
  return *this;
}
WorldEdit &WorldEdit::LineY(const IVector3 &pos, size_t length) {
  for (size_t y = 0; y<length && y+pos.y<world->GetSize().y; y++)
    ApplyBrush(IVector3(pos.x, pos.y+y, pos.z));
  return *this;
}

WorldEdit &WorldEdit::LineZ(const IVector3 &pos, size_t length) {
  for (size_t z = 0; z<length && z+pos.z<world->GetSize().z; z++)
    ApplyBrush(IVector3(pos.x, pos.y, pos.z+z));
  return *this;
}

WorldEdit &WorldEdit::WallYZ(const IVector3 &pos, size_t height, size_t depth) {
  for (size_t y = 0; y<height; y++)
    LineZ(IVector3(pos.x, pos.y+y, pos.z), depth);
  return *this;
}

WorldEdit &WorldEdit::WallXZ(const IVector3 &pos, size_t width, size_t depth) {
  for (size_t x = 0; x<width; x++)
    LineZ(IVector3(pos.x+x, pos.y, pos.z), depth);
  return *this;
}

WorldEdit &WorldEdit::WallXY(const IVector3 &pos, size_t width, size_t height) {
  for (size_t y = 0; y<height; y++)
    LineX(IVector3(pos.x, pos.y+y, pos.z), width);
  return *this;
}

WorldEdit &WorldEdit::FilledBox(const IVector3 &pos, const IVector3 &size) {
  for (size_t z = 0; z<size.z; z++) 
    for (size_t y = 0; y<size.y; y++) 
      LineX(IVector3(pos.x, pos.y+y, pos.z+z), size.x);
  return *this;
}

WorldEdit &
WorldEdit::Explosion(const IVector3 &pos, const IVector3 &size, float strength) {
  Vector3 v(pos);
  Vector3 vs(size);
  
  IVector3(size.x*2+1, size.y*2+1, size.z*2+1).For( [&] (IVector3 p) {
    IVector3 pp = pos - size + p;
    Vector3 vpp(pp);
    float d = (vpp-v).GetSquareMag()/4;
    float prob = vs.GetMag()/2-d;
    if (rand()%100 < (prob*100*strength)) {
      ApplyBrush(pp);
    }
  });
  
  return *this; 
}

WorldEdit &
WorldEdit::Ellipsoid(const IVector3 &pos, const IVector3 &size) {
  Vector3 v(pos);
  Vector3 vs(size);
  for (size_t x = 0; x<size.x*2+1; x++) {
    for (size_t y = 0; y<size.y*2+1; y++) {
      for (size_t z = 0; z<size.z*2+1; z++) {
        IVector3 pp = pos - size + IVector3(x, y, z);
        Vector3 vpp(pp);
        float d = ((vpp-v)/vs).GetSquareMag();
        if (d < 1.0) {
          ApplyBrush(pp);
        }
      }
    }
  }
  return *this; 
}
