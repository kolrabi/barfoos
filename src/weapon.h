#ifndef BARFOOS_WEAPON_H
#define BARFOOS_WEAPON_H

#include "item.h"

class Weapon : public Item {
public:

  Weapon();
  virtual ~Weapon();

  //virtual void Draw(bool left);

protected:

  unsigned int texture;
};


#endif

