#ifndef BARFOOS_TEMPLATE_H
#define BARFOOS_TEMPLATE_H

#include "common.h"
 
class World; 

class Template {
public:
  virtual IVector3 GetSize() const = 0;
  virtual void Apply(const IVector3 &origin, World &world) const = 0;
};

void LoadTemplates();
const Template *GetTemplate(const std::string &name);

#endif

