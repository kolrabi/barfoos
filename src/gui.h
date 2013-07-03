#ifndef BARFOOS_GUI_H
#define BARFOOS_GUI_H

#include "common.h"

class Gfx;

class Gui {
public:
  
  Gui();
  virtual ~Gui();

  virtual void Update(float t);
  virtual void OnMouseMove(const Point &point);
  virtual void OnMouseClick(const Point &pos, int button, bool down);
  virtual void OnHide() {}
  virtual void OnShow() {}

  virtual void Draw(Gfx &gfx, const Point &parentPos);

  virtual bool IsOver(const Point &p) const;
  virtual Gui *GetChildAt(const Point &p);
 
  virtual void SetPosition(const Point &p);
  virtual void SetSize(const Point &s);
  virtual void SetGravity(bool gravN, bool gravE, bool gravS, bool gravW);

protected:

  Rect rect;
  Point bottomRight;

  float lastT;
  float deltaT;

  std::vector<Gui*> children;

  bool gravN, gravE, gravS, gravW;
};

#endif

