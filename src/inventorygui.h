#ifndef BARFOOS_INVENTORY_GUI_H
#define BARFOOS_INVENTORY_GUI_H

#include "common.h"

#include "gui.h"

class Entity;
class InventoryGui;
class Item;

class InventorySlotGui : public Gui {
public: 
  InventorySlotGui(InventoryGui *parent, const std::shared_ptr<Entity> &entity, InventorySlot slot);
  virtual ~InventorySlotGui();

  virtual void Draw(const Point &parentPos);  
  virtual void OnMouseClick(const Point &pos, int button, bool down);

private:
  InventoryGui *parent;
  std::shared_ptr<Entity> entity;
  InventorySlot slot;

  unsigned int slotTex;
};

class InventoryGui : public Gui {
public:
  
  InventoryGui(const std::shared_ptr<Entity> &entity);
  virtual ~InventoryGui();

  virtual void Update(float t);
  virtual void OnMouseMove(const Point &point);
  virtual void OnMouseClick(const Point &pos, int button, bool down);
  virtual void OnHide();

  virtual void Draw(const Point &parentPos);  

//  void SetDragItem(const std::shared_ptr<Item> &item);
//  std::shared_ptr<Item> GetDragItem();
  std::shared_ptr<Item> dragItem;

protected:

  std::shared_ptr<Entity> entity;
  Point mousePos;

private:

  Gui* AddSlotGui(const Point &p, InventorySlot slot);
};

#endif

