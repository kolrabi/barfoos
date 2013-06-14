#ifndef BARFOOS_TEXT_H
#define BARFOOS_TEXT_H

#include "common.h"

class RenderString {
public:

  RenderString(const std::string &str);
  RenderString &operator =(const std::string &str);

  void Draw(float x, float y);

private:

  std::string str;
  bool dirty;
  std::vector<Vertex> vertices;
};

#endif

