#ifndef BARFOOS_GUI_H
#define BARFOOS_GUI_H

#include "common.h"
#include "2d.h"

class Gui {
public:
  
  Gui();
  virtual ~Gui();

  virtual void Update(Game &game);
  virtual void HandleEvent(const InputEvent &event);
  virtual void OnHide() {}
  virtual void OnShow() {}

  virtual void Draw(Gfx &gfx, const Point &parentPos);
  virtual void DrawTooltip(Gfx &gfx, const Point &parentPos);

  bool IsOver(const Point &p) const;
  
  void AddChild(Gui *child);
  Gui *GetChildAt(const Point &p);
 
  void SetSize(const Point &s);
  void SetPosition(const Point &p);
  void SetCenter(const Point &p);
  void SetGravity(bool gravN, bool gravE, bool gravS, bool gravW);

  void SetText(const std::string &str);
  
  std::function<void(Gui *)> GetOnActivate() { return onActivate; }
  void SetOnActivate(std::function<void(Gui *)> func) { onActivate = func; }

protected:

  Rect rect;
  Point lastVSize;

  std::vector<Gui*> children;

  bool gravN, gravE, gravS, gravW;
  bool updateGravity;
  
  RenderString *text;
  
  bool hover;
  
  std::function<void(Gui *)> onActivate;
};

#endif

