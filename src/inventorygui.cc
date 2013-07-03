#include "inventorygui.h"
#include "entity.h"
#include "util.h"
#include "item.h"
#include "itementity.h"
#include "game.h"
#include "gfx.h"
#include "icolor.h"

InventoryGui::InventoryGui(size_t entityId) 
: entityId(entityId) {

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

  temp_ptr<Entity> entity(Game::Instance->GetEntity(entityId));
  
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

void InventoryGui::Draw(Gfx &gfx, const Point &parentPos) {
  Gui::Draw(gfx, parentPos);
  
  if (dragItem != nullptr) {
    dragItem->DrawIcon(gfx, mousePos);
  }
}

void InventoryGui::OnMouseMove(const Point &pos) {
  Gui::OnMouseMove(pos);
  mousePos = pos;
}

void InventoryGui::OnMouseClick(const Point &pos, int button, bool down) {
  Gui::OnMouseClick(pos, button, down);
  
  temp_ptr<Entity> entity = Game::Instance->GetEntity(entityId);
  
  if (down == false) {
    if (dragItem) {
      if (entity) {
        // drop item into world
        Mob *itemEntity = new ItemEntity(dragItem);
      
        itemEntity->SetPosition(entity->GetAABB().center + this->forward);
        itemEntity->AddVelocity(this->forward * 10);
      
        Game::Instance->AddEntity(itemEntity);
      }
      
      this->dragItem = nullptr;
    }
  }
}

Gui *InventoryGui::AddSlotGui(const Point &p, InventorySlot slot) {
  Gui *gui = new InventorySlotGui(this, entityId, slot);
  gui->SetPosition(p - Point(16,16));
  gui->SetSize(Point(32,32));
  this->children.push_back(gui);
  return gui;
}

void InventoryGui::OnHide() {
  if (dragItem) {
    temp_ptr<Entity> entity = Game::Instance->GetEntity(entityId);
    entity->AddToInventory(dragItem);
    
    this->dragItem = nullptr;
  }
}

// -----------------------------------------------------------------------

InventorySlotGui::InventorySlotGui(
  InventoryGui *parent, size_t entityId, InventorySlot slot) {
  this->parent = parent;
  this->entityId = entityId;
  this->slot = slot;
  this->slotTex = loadTexture("gui/slot");
}

InventorySlotGui::~InventorySlotGui() {
}
  
void InventorySlotGui::OnMouseClick(const Point &pos, int button, bool down) {
  (void)pos;
  (void)button;
  
  temp_ptr<Entity> entity(Game::Instance->GetEntity(this->entityId));
  if (!entity) return;
  
  if (down) {
    std::shared_ptr<Item> item(entity->GetInventory(slot));
    if (item) {
      parent->dragItem = item;
      entity->Equip(nullptr, slot);
    }
  } else {
    std::shared_ptr<Item> item(parent->dragItem);
    if (!item) return;
    if (!entity->AddToInventory(item, slot)) {
      entity->AddToInventory(item);
    }
    parent->dragItem = nullptr;
  }
}

void 
InventorySlotGui::Draw(Gfx &gfx, const Point &parentPos) {
  const Texture *tex = slotTex;
  Sprite sprite;
  sprite.texture = tex;

  Point p = rect.pos+parentPos+Point(16,16);
  
  gfx.DrawIcon(sprite, p);

  temp_ptr<Entity> entity(Game::Instance->GetEntity(this->entityId));
  if (!entity) return;
  
  std::shared_ptr<Item> item = entity->GetInventory(slot);
  
  if (item != nullptr)
    item->DrawIcon(gfx, p);
}

