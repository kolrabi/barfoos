#include "common.h"

#include "gui.h"

#include "gfx.h"
#include "game.h"
#include "input.h"
#include "text.h"

#include "vertex.h"
#include "texture.h"

Gui::Gui() :
  rect(),
  children(0),
  gravN(true),
  gravE(false),
  gravS(false),
  gravW(true),
  updateGravity(true),
  text(new RenderString("")),
  state(GuiState::Enabled)
{
  SetColor(GuiState::Enabled,   IColor(192, 192, 192));
  SetColor(GuiState::Disabled, IColor(128, 128, 128));
  SetColor(GuiState::Hover,    IColor(255, 255, 255));
  SetColor(GuiState::Active,   IColor(255, 255, 0));
}

Gui::~Gui() {
  delete text;

  for (auto c:children) {
    delete c;
  }
}

void
Gui::Update(Game &game) {
  for (auto c : children) {
    c->Update(game);
  }

  const Point &vsize = game.GetGfx().GetScreen().GetVirtualSize();

  if (this->updateGravity) {
    lastVSize = vsize;
    this->updateGravity = false;
  }

  Point diff( vsize - lastVSize );

  if (gravE) {
    if (gravW) {
      rect.size.x += diff.x;
    } else {
      rect.pos.x  += diff.x;
    }
  } else if (!gravW) {
    rect.pos.x += diff.x / 2;
  }

  if (gravS) {
    if (gravN) {
      rect.size.y += diff.y;
    } else {
      rect.pos.y  += diff.y;
    }
  } else if (!gravN) {
    rect.pos.y += diff.y / 2;
  }

  lastVSize = vsize;
}

void
Gui::Draw(Gfx &gfx, const Point &parentPos) {
  Rect r = rect;
  r.pos = rect.pos + parentPos;

  NinePatch &background = backgrounds[state];
  if (background.texture) {
    VertexBuffer verts(background.GetVerts(r));

    gfx.SetColor(IColor(255,255,255));
    gfx.SetTextureFrame(background.texture);
    gfx.DrawQuads(verts);
  }

  gfx.SetColor(colors[state]);
  this->text->Draw(gfx, (parentPos.x + rect.pos.x)+rect.size.x*0.5, (parentPos.y + rect.pos.y)+rect.size.y*0.5, int(Align::HorizCenter | Align::VertMiddle));

  gfx.SetColor(IColor(255,255,255));
  for (auto c : children) {
    c->Draw(gfx, parentPos+rect.pos);
  }
}

void
Gui::DrawTooltip(Gfx &gfx, const Point &parentPos) {
  gfx.SetColor(IColor(255,255,255));
  for (auto c : children) {
    c->DrawTooltip(gfx, parentPos+rect.pos);
  }
}

void
Gui::HandleEvent(const InputEvent &event) {
  if (state == GuiState::Disabled) return;

  for (auto c : children) {
    InputEvent event2(event);
    event2.p = event.p - c->rect.pos;
    c->HandleEvent(event2);
  }

  if (event.type == InputEventType::Key && event.key == InputKey::MouseLeft) {
    if (event.down && IsOver(event.p)) {
      state = GuiState::Active;
    } else if (!event.down && state == GuiState::Active) {
      if (onActivate && !GetChildAt(event.p)) onActivate(this);
    }
  }

  if (event.type == InputEventType::MouseMove) {
    if (IsOver(event.p)) {
      if (state != GuiState::Active) state = GuiState::Hover;
    } else {
      state = GuiState::Enabled;
    }
  }
}

bool Gui::IsOver(const Point &p) const {
  if (p.x < 0 || p.y < 0) return false;
  if (p.x >= rect.size.x || p.y >= rect.size.y) return false;
  return true;
}

void Gui::AddChild(Gui *child) {
  this->children.push_back(child);
}

Gui *Gui::GetChildAt(const Point &p) {
  for (size_t i=0; i<children.size(); i++) {
    Gui *child = children[i];
    if (child->IsOver(p - child->rect.pos)) {
      return child;
    }
  }
  return nullptr;
}

void
Gui::SetSize(const Point &s) {
  rect.size = s;
  SetGravity(gravN, gravE, gravS, gravW);
}

void
Gui::SetPosition(const Point &p) {
  rect.pos = p;
  SetGravity(gravN, gravE, gravS, gravW);
}

void
Gui::SetCenter(const Point &p) {
  SetPosition(p - rect.size / 2);
}

void
Gui::SetGravity(bool gravN, bool gravE, bool gravS, bool gravW) {
  this->gravN = gravN;
  this->gravE = gravE;
  this->gravS = gravS;
  this->gravW = gravW;

  this->updateGravity = true;
}

void
Gui::SetText(const std::string &str, const std::string &font) {
  delete this->text;
  this->text = new RenderString(str, font);
}

void
Gui::SetEnabled(bool enable) {
  if (enable && state != GuiState::Disabled) return;

  if (enable) state = GuiState::Enabled; else state = GuiState::Disabled;
}


static void ninePatchStretch(const Texture *tex, const Rect &src, const Rect &dest, std::vector<Vertex> &verts) {
  if (!tex) return;

  float u1 = float(src.pos.x)/tex->size.x;
  float u2 = float(src.pos.x + src.size.x)/tex->size.x;
  float v1 = 1.0-float(src.pos.y + src.size.y)/tex->size.y;
  float v2 = 1.0-float(src.pos.y)/tex->size.y;

  verts.push_back(Vertex(Vector3(dest.pos.x,             dest.pos.y+dest.size.y,  0), IColor(255,255,255), u1, v1, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(dest.pos.x+dest.size.x, dest.pos.y+dest.size.y,  0), IColor(255,255,255), u2, v1, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(dest.pos.x+dest.size.x, dest.pos.y,              0), IColor(255,255,255), u2, v2, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(dest.pos.x,             dest.pos.y,              0), IColor(255,255,255), u1, v2, Vector3( 0, 0, 1)));
}

NinePatch::NinePatch(const std::string &name, const Rect &innerRect) {
  texture = Texture::Get(name);

  srcRects[0].pos = Point(0,                                  0);
  srcRects[1].pos = Point(innerRect.pos.x,                    0);
  srcRects[2].pos = Point(innerRect.pos.x + innerRect.size.x, 0);

  srcRects[3].pos = Point(0,                                  innerRect.pos.y);
  srcRects[4].pos = Point(innerRect.pos.x,                    innerRect.pos.y);
  srcRects[5].pos = Point(innerRect.pos.x + innerRect.size.x, innerRect.pos.y);

  srcRects[6].pos = Point(0,                                  innerRect.pos.y + innerRect.size.y);
  srcRects[7].pos = Point(innerRect.pos.x,                    innerRect.pos.y + innerRect.size.y);
  srcRects[8].pos = Point(innerRect.pos.x + innerRect.size.x, innerRect.pos.y + innerRect.size.y);

  srcRects[0].size = Point(innerRect.pos.x,                     innerRect.pos.y);
  srcRects[1].size = Point(innerRect.size.x,                    innerRect.pos.y);
  srcRects[2].size = Point(texture->size.x - srcRects[2].pos.x, innerRect.pos.y);

  srcRects[3].size = Point(innerRect.pos.x,                     innerRect.size.y);
  srcRects[4].size = Point(innerRect.size.x,                    innerRect.size.y);
  srcRects[5].size = Point(texture->size.x - srcRects[2].pos.x, innerRect.size.y);

  srcRects[6].size = Point(innerRect.pos.x,                     texture->size.y - srcRects[6].pos.y);
  srcRects[7].size = Point(innerRect.size.x,                    texture->size.y - srcRects[7].pos.y);
  srcRects[8].size = Point(texture->size.x - srcRects[2].pos.x, texture->size.y - srcRects[8].pos.y);
}

NinePatch::NinePatch(const std::string &name) {
  texture = Texture::Get(name);

  Rect innerRect(texture->size/4, texture->size/2);

  srcRects[0].pos = Point(0,                                  0);
  srcRects[1].pos = Point(innerRect.pos.x,                    0);
  srcRects[2].pos = Point(innerRect.pos.x + innerRect.size.x, 0);

  srcRects[3].pos = Point(0,                                  innerRect.pos.y);
  srcRects[4].pos = Point(innerRect.pos.x,                    innerRect.pos.y);
  srcRects[5].pos = Point(innerRect.pos.x + innerRect.size.x, innerRect.pos.y);

  srcRects[6].pos = Point(0,                                  innerRect.pos.y + innerRect.size.y);
  srcRects[7].pos = Point(innerRect.pos.x,                    innerRect.pos.y + innerRect.size.y);
  srcRects[8].pos = Point(innerRect.pos.x + innerRect.size.x, innerRect.pos.y + innerRect.size.y);

  srcRects[0].size = Point(innerRect.pos.x,                     innerRect.pos.y);
  srcRects[1].size = Point(innerRect.size.x,                    innerRect.pos.y);
  srcRects[2].size = Point(texture->size.x - srcRects[2].pos.x, innerRect.pos.y);

  srcRects[3].size = Point(innerRect.pos.x,                     innerRect.size.y);
  srcRects[4].size = Point(innerRect.size.x,                    innerRect.size.y);
  srcRects[5].size = Point(texture->size.x - srcRects[2].pos.x, innerRect.size.y);

  srcRects[6].size = Point(innerRect.pos.x,                     texture->size.y - srcRects[6].pos.y);
  srcRects[7].size = Point(innerRect.size.x,                    texture->size.y - srcRects[7].pos.y);
  srcRects[8].size = Point(texture->size.x - srcRects[2].pos.x, texture->size.y - srcRects[8].pos.y);
}

std::vector<Vertex> NinePatch::GetVerts(const Rect &rect) const {
  std::vector<Vertex> verts;
  Point p1 = srcRects[0].size;
  Point p2 = rect.size - srcRects[8].size;

  ninePatchStretch(texture, srcRects[0], Rect(rect.pos + Point(   0,    0), srcRects[0].size),                     verts);
  ninePatchStretch(texture, srcRects[1], Rect(rect.pos + Point(p1.x,    0), Point(p2.x-p1.x, srcRects[1].size.y)), verts);
  ninePatchStretch(texture, srcRects[2], Rect(rect.pos + Point(p2.x,    0), srcRects[2].size),                     verts);

  ninePatchStretch(texture, srcRects[3], Rect(rect.pos + Point(   0, p1.y), Point(srcRects[3].size.x, p2.y-p1.y)), verts);
  ninePatchStretch(texture, srcRects[4], Rect(rect.pos + Point(p1.x, p1.y), p2-p1),                                verts);
  ninePatchStretch(texture, srcRects[5], Rect(rect.pos + Point(p2.x, p1.y), Point(srcRects[5].size.x, p2.y-p1.y)), verts);

  ninePatchStretch(texture, srcRects[6], Rect(rect.pos + Point(   0, p2.y), srcRects[6].size),                     verts);
  ninePatchStretch(texture, srcRects[7], Rect(rect.pos + Point(p1.x, p2.y), Point(p2.x-p1.x, srcRects[7].size.y)), verts);
  ninePatchStretch(texture, srcRects[8], Rect(rect.pos + Point(p2.x, p2.y), srcRects[8].size),                     verts);

  return verts;
}

