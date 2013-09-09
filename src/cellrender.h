#ifndef BARFOOS_CELLRENDER_H
#define BARFOOS_CELLRENDER_H

#include "cellbase.h"

#include "vertex.h"

class CellRender : public CellBase {
public:

  void                      SetTexture          (const Texture *tex, bool multi = false);
  const Texture *           GetTexture          ()                                        const;

  void                      SetEmissiveTexture  (const Texture *tex);
  const Texture *           GetEmissiveTexture  ()                                        const;

  bool                      UpdateVertices      ();
  void                      UpdateColors        ();

  void                      Draw                (std::vector<Vertex> &vertices)           const;
  void                      DrawEmissive        (std::vector<Vertex> &vertices)           const;
  
  uint8_t                   GetVisibility       ()                                        const;
  void                      SetVisibility       (uint8_t visibility);

  void                      SetReversedSides    (bool rev)                                      { this->reversedSides = rev; }

protected:

                            CellRender          (const std::string &type);

  /** Side visibility. */
  uint8_t                   visibility;

  /** Whether or not to draw the sides reversed. */
  bool                      reversedSides;

  /** Corner positions relative to cell. */
  Vector3                   corners[8];

  /** Cell vertices in world space. */
  std::vector<Vertex>       verts;

  /** Applied texture. */
  const Texture *           texture;

  /** Applied emissive texture. */
  const Texture *           emissiveTexture;

  /** Texture to use when "active". */
  const Texture *           activeTexture;

  /** Emissive texture to use when "active". */
  const Texture *           emissiveActiveTexture;

  /** Texture u coordinate scale. */
  float                     uscale;

  IColor                    SideCornerColor     (Side side, size_t corner)                const;
  void                      SideColors          (Side side, IColor *colors)               const;
  void                      SideColors          (Side side, std::vector<IColor> &outcolors, bool reverse)     const;
  void                      SideVerts           (Side side, std::vector<Vertex> &verts, bool reverse = false) const;
};

/** Get current texture.
  * @return Normal texture for this cell, or active. 
  */
inline const Texture *CellRender::GetTexture() const {
  if (this->activeTexture && this->lastT < this->nextActivationT) {
    return this->activeTexture;
  }
  return this->texture;
}

/** Get current emissive texture.
  * @return Normal texture for this cell, or active. 
  */
inline const Texture *CellRender::GetEmissiveTexture() const {
  if (this->emissiveActiveTexture && this->lastT < this->nextActivationT) {
    return this->emissiveActiveTexture;
  }
  return this->emissiveTexture;
}

/** Get side visibility.
  * @return Visibility mask.
  */
inline uint8_t CellRender::GetVisibility() const {
  return this->visibility;
}

/** Set side visibility.
  * @param visibility Visibility mask.
  */
inline void CellRender::SetVisibility(uint8_t visibility) {
  this->visibility = visibility;
}

/** Append cell vertices to a vector.
  * @param verts Where to copy the vertices.
  */
inline void
CellRender::Draw(std::vector<Vertex> &verts) const {
  if (info->flags & CellFlags::DoNotRender || visibility == 0) return;

  for (const Vertex &v : this->verts) {
    verts.push_back(v);
  }
}

/** Append cell vertices to a vector, with color values set to white.
  * @param verts Where to copy the vertices.
  */
inline void
CellRender::DrawEmissive(std::vector<Vertex> &verts) const {
  Draw(verts);
  /*
  if (info->flags & CellFlags::DoNotRender || visibility == 0) return;

  for (const Vertex &v : this->verts) {
    Vertex vv = v;
    vv.rgb[0] = vv.rgb[1] = vv.rgb[2] = vv.rgb[3] = 1.0;
    verts.push_back(vv);
  }*/
}

#endif

