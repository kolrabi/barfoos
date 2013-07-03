#ifndef BARFOOS_INVENTORY_GUI_H
#define BARFOOS_INVENTORY_GUI_H

#include "common.h"
#include "gui.h"

class Entity;
class InventoryGui;
class Item;
class Gfx;

class InventorySlotGui : public Gui {
public: 
  InventorySlotGui(InventoryGui *parent, size_t entityId, InventorySlot slot);
  virtual ~InventorySlotGui();

  virtual void Draw(Gfx &gfx, const Point &parentPos);  
  virtual void OnMouseClick(const Point &pos, int button, bool down);

private:
  InventoryGui *parent;
  size_t entityId;
  InventorySlot slot;

  const Texture *slotTex;
};

class InventoryGui : public Gui {
public:
  
  InventoryGui(size_t entityId);
  virtual ~InventoryGui();

  virtual void Update(float t);
  virtual void OnMouseMove(const Point &point);
  virtual void OnMouseClick(const Point &pos, int button, bool down);
  virtual void OnHide();

  virtual void Draw(Gfx &gfx, const Point &parentPos);  
  
  void SetForward(const Vector3 &forward) { this->forward = forward; }

  std::shared_ptr<Item> dragItem;

protected:

  size_t entityId;
  Point mousePos;
  Vector3 forward;

private:

  Gui* AddSlotGui(const Point &p, InventorySlot slot);
};

#endif

