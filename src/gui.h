#ifndef BARFOOS_GUI_H
#define BARFOOS_GUI_H

#include "common.h"

class Gfx;
class Game;
struct InputEvent;

class Gui {
public:
  
  Gui();
  virtual ~Gui();

  virtual void Update(Game &game);
  virtual void HandleEvent(const InputEvent &event);
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

  std::vector<Gui*> children;

  bool gravN, gravE, gravS, gravW;
  bool updateGravity;
};

#endif

