#ifndef BARFOOS_CELLRENDER_H
#define BARFOOS_CELLRENDER_H

#include "cellbase.h"

class CellRender : public CellBase {
public:

  void  SetTexture(const Texture *tex, bool multi = false);
  const Texture *GetTexture() const;

  void UpdateVertices();

  void Draw(std::vector<Vertex> &vertices) const;
  void DrawHighlight(std::vector<Vertex> &vertices) const;

protected:

  CellRender(const std::string &type);
  
  uint8_t visibility;
  bool reversedSides;
  Vector3 corners[8];
  std::vector<Vertex> verts;
  const Texture *texture;
  const Texture *activeTexture;
  float uscale;

  IColor SideCornerColor(Side side, size_t corner) const;
  void SideColors(Side side, IColor *colors) const;
  void SideVerts(Side side, std::vector<Vertex> &verts, bool reverse = false) const;
};

inline const Texture *CellRender::GetTexture() const { 
  if (this->activeTexture && this->lastT < this->nextActivationT) {
    return this->activeTexture;
  }
  return this->texture; 
}

#endif

