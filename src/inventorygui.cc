#include "inventorygui.h"
#include "entity.h"
#include "util.h"
#include "item.h"
#include "itementity.h"
#include "world.h"

InventoryGui::InventoryGui(const std::shared_ptr<Entity> &entity) 
: entity(entity) {

  //                 x  x  x  x
  //   8  0          x  x  x  x
  //   4  1  5       x  x  x  x
  //   6  2  7       x  x  x  x
  //      3          x  x  x  x
  //                 x  x  x  x

  Point slotDist(36, 36);
  Point topLeft  = alignTopLeftScreen(Point(32,32), 16);
  Point topRight = alignTopRightScreen(Point(32,32), 16);

  AddSlotGui(topLeft+Point(slotDist.x*0, slotDist.y*0), InventorySlot::Amulet);
  AddSlotGui(topLeft+Point(slotDist.x*1, slotDist.y*0), InventorySlot::Helmet);
  AddSlotGui(topLeft+Point(slotDist.x*0, slotDist.y*1), InventorySlot::LeftRing);
  AddSlotGui(topLeft+Point(slotDist.x*1, slotDist.y*1), InventorySlot::Armor);
  AddSlotGui(topLeft+Point(slotDist.x*2, slotDist.y*1), InventorySlot::RightRing);
  AddSlotGui(topLeft+Point(slotDist.x*0, slotDist.y*2), InventorySlot::LeftHand);
  AddSlotGui(topLeft+Point(slotDist.x*1, slotDist.y*2), InventorySlot::Greaves);
  AddSlotGui(topLeft+Point(slotDist.x*2, slotDist.y*2), InventorySlot::RightHand);
  AddSlotGui(topLeft+Point(slotDist.x*1, slotDist.y*3), InventorySlot::Boots);

  for (size_t i=(size_t)InventorySlot::Backpack; i<entity->GetInventorySize(); i++) {
    int x = i%4-3;
    int y = (i-(size_t)InventorySlot::Backpack)/4;
    AddSlotGui(topRight+Point(slotDist.x*x, slotDist.y*y), (InventorySlot)i)->SetGravity(true, true, false, false);
  }
  
  this->dragItem = nullptr;
}

InventoryGui::~InventoryGui() {
}

void InventoryGui::Update(float t) {
  Gui::Update(t);
}

void InventoryGui::Draw(const Point &parentPos) {
  Gui::Draw(parentPos);
  
  if (dragItem != nullptr) {
    dragItem->DrawIcon(mousePos);
  }
}

void InventoryGui::OnMouseMove(const Point &pos) {
  Gui::OnMouseMove(pos);
  mousePos = pos;
}

void InventoryGui::OnMouseClick(const Point &pos, int button, bool down) {
  Gui::OnMouseClick(pos, button, down);
  if (down == false) {
    if (dragItem) {
      // drop item into world
      std::shared_ptr<Mob> entity(new ItemEntity(dragItem));
      entity->SetPosition(this->entity->GetAABB().center + this->forward);
      entity->AddVelocity(this->forward * 10);
      this->entity->GetWorld()->AddEntity(entity);
      
      this->dragItem = nullptr;
    }
  }
}

Gui *InventoryGui::AddSlotGui(const Point &p, InventorySlot slot) {
  Gui *gui = new InventorySlotGui(this, entity, slot);
  gui->SetPosition(p - Point(16,16));
  gui->SetSize(Point(32,32));
  this->children.push_back(gui);
  return gui;
}

void InventoryGui::OnHide() {
  if (dragItem) {
    this->entity->AddToInventory(dragItem);
    this->dragItem = nullptr;
  }
}

/*
void InventoryGui::SetDragItem(const std::shared_ptr<Item> &item) {
  (void)item;
  //this->dragItem = item;
}

std::shared_ptr<Item> InventoryGui::GetDragItem() {
  return this->dragItem;
}
*/
// -----------------------------------------------------------------------

InventorySlotGui::InventorySlotGui(
  InventoryGui *parent, const std::shared_ptr<Entity> &entity, InventorySlot slot) {
  this->parent = parent;
  this->entity = entity;
  this->slot = slot;
  this->slotTex = loadTexture("gui/slot");
  
  std::shared_ptr<Item> test(parent->dragItem);
  if (test) this->entity->AddToInventory(test);
}

InventorySlotGui::~InventorySlotGui() {
}
  
void InventorySlotGui::OnMouseClick(const Point &pos, int button, bool down) {
  (void)pos;
  (void)button;
  if (down) {
    std::shared_ptr<Item> item(this->entity->GetInventory(slot));
    if (item) {
      parent->dragItem = item;
      this->entity->Equip(nullptr, slot);
    }
  } else {
    std::shared_ptr<Item> item(parent->dragItem);
    if (!item) return;
    if (!this->entity->AddToInventory(item, slot)) {
      this->entity->AddToInventory(item);
    }
    parent->dragItem = nullptr;
  }
}

void 
InventorySlotGui::Draw(const Point &parentPos) {
  unsigned int tex = slotTex;
  Point p = rect.pos+parentPos+Point(16,16);
  
  drawIcon(p, Point(32,32), tex);

  std::shared_ptr<Item> item = entity->GetInventory(slot);
  
  if (item != nullptr)
    item->DrawIcon(p);
}

