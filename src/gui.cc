#include "gui.h"
#include "util.h"

Gui::Gui() {
  this->gravN = true;
  this->gravE = false;
  this->gravS = false;
  this->gravW = true;
}

Gui::~Gui() {
}

void 
Gui::Update(float t) {
  if (lastT == 0) lastT = t;
  this->deltaT = lastT - t;

  for (auto c : children) {
    c->Update(t);
  }

  if (gravE) {
    int oldRight = rect.pos.x + rect.size.x;
    int newRight = virtualScreenWidth - bottomRight.x;
    if (gravW) {
      rect.size.x += newRight-oldRight;
    } else {
      rect.pos.x += newRight-oldRight;
    }
  }

  if (gravS) {
    int oldBottom = rect.pos.y + rect.size.y;
    int newBottom = virtualScreenHeight - bottomRight.y;
    if (gravN) {
      rect.size.y += newBottom-oldBottom;
    } else {
      rect.pos.y += newBottom-oldBottom;
    }
  }
}

void 
Gui::Draw(const Point &parentPos) {
  for (auto c : children) {
    c->Draw(parentPos+rect.pos);
  }
}

void
Gui::OnMouseMove(const Point &point) {
  for (auto c : children) {
    c->OnMouseMove(point - c->rect.pos);
  }
}

void 
Gui::OnMouseClick(const Point &pos, int button, bool down) {
  Gui *child = GetChildAt(pos);
  if (child) 
    child->OnMouseClick(pos - child->rect.pos, button, down);
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

  bottomRight = Point(virtualScreenWidth, virtualScreenHeight) - (rect.pos + rect.size);
}
