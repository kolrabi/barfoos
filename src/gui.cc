#include "gui.h"
#include "util.h"
#include "gfx.h"
#include "game.h"
#include "input.h"

Gui::Gui() {
  this->gravN = true;
  this->gravE = false;
  this->gravS = false;
  this->gravW = true;
  
  this->updateGravity = true;
}

Gui::~Gui() {
}

void 
Gui::Update(Game &game) {
  for (auto c : children) {
    c->Update(game);
  }

  const Point &vsize = game.GetGfx()->GetVirtualScreenSize();

  if (this->updateGravity) {
    bottomRight = vsize - (rect.pos + rect.size);
    this->updateGravity = false;
  }
  
  if (gravE) {
    int oldRight = rect.pos.x + rect.size.x;
    int newRight = vsize.x - bottomRight.x;
    if (gravW) {
      rect.size.x += newRight-oldRight;
    } else {
      rect.pos.x += newRight-oldRight;
    }
  }

  if (gravS) {
    int oldBottom = rect.pos.y + rect.size.y;
    int newBottom = vsize.y - bottomRight.y;
    if (gravN) {
      rect.size.y += newBottom-oldBottom;
    } else {
      rect.pos.y += newBottom-oldBottom;
    }
  }
}

void 
Gui::Draw(Gfx &gfx, const Point &parentPos) {
  gfx.SetColor(IColor(255,255,255));
  for (auto c : children) {
    c->Draw(gfx, parentPos+rect.pos);
  }
}

void
Gui::HandleEvent(const InputEvent &event) {
  for (auto c : children) {
    InputEvent event2(event);
    event2.p = event.p - c->rect.pos;
    c->HandleEvent(event2);
  }
}

bool Gui::IsOver(const Point &p) const { 
  if (p.x < 0 || p.y < 0) return false;
  if (p.x >= rect.size.x || p.y >= rect.size.x) return false;
  return true;
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
Gui::SetPosition(const Point &p) {
  rect.pos = p;
  SetGravity(gravN, gravE, gravS, gravW);
}

void 
Gui::SetSize(const Point &s) {
  rect.size = s;
  SetGravity(gravN, gravE, gravS, gravW);
}

void 
Gui::SetGravity(bool gravN, bool gravE, bool gravS, bool gravW) {
  this->gravN = gravN;
  this->gravE = gravE;
  this->gravS = gravS;
  this->gravW = gravW;

  this->updateGravity = true;
}
