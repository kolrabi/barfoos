#include "gui.h"
#include "gfx.h"
#include "game.h"
#include "input.h"
#include "text.h"

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
  hover(false)
{}

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

  const Point &vsize = game.GetGfx().GetVirtualScreenSize();

  if (this->updateGravity) {
    lastVSize = vsize;
    this->updateGravity = false;
  }
  
  Point diff( vsize - lastVSize );
  
  if (diff.x || diff.y) Log("%d %d\n", diff.x, diff.y);
  
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
  if (hover) {
    gfx.SetColor(IColor(255,255,0));
  } else {
    gfx.SetColor(IColor(192,192,192));
  }
  
  Rect r = rect;
  r.pos = rect.pos + parentPos;

  // TODO: set texture
  std::vector<Vertex> verts;
  verts.push_back(Vertex(Vector3(r.pos.x,          r.pos.y+r.size.y,  0), IColor(255,255,255), 0,1, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(r.pos.x+r.size.x, r.pos.y+r.size.y,  0), IColor(255,255,255), 1,1, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(r.pos.x+r.size.x, r.pos.y,           0), IColor(255,255,255), 1,0, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(r.pos.x,          r.pos.y,           0), IColor(255,255,255), 0,0, Vector3( 0, 0, 1)));
  gfx.DrawQuads(verts);
  
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
  for (auto c : children) {
    InputEvent event2(event);
    event2.p = event.p - c->rect.pos;
    c->HandleEvent(event2);
  }
  
  if (event.type == InputEventType::Key && !event.down && event.key == InputKey::MouseLeft && onActivate) {
    if (IsOver(event.p) && !GetChildAt(event.p)) onActivate(this);
  }
  
  if (event.type == InputEventType::MouseMove) {
    hover = IsOver(event.p);
  }
}

bool Gui::IsOver(const Point &p) const { 
  if (p.x < 0 || p.y < 0) return false;
  if (p.x >= rect.size.x || p.y >= rect.size.x) return false;
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
Gui::SetText(const std::string &str) {
  *this->text = str;
}
