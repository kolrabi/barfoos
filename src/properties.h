#ifndef BARFOOS_PROPERTIES_H
#define BARFOOS_PROPERTIES_H

#include "common.h"

struct IColor;

struct Properties {
  void ParseFile(FILE *f);
  
  virtual void ParseProperty(const std::string &name) = 0;
  
  void Parse(int &i);
  void Parse(size_t &i);
  void Parse(float &f);
  void Parse(Vector3 &v);
  void Parse(IVector3 &iv);
  void Parse(IColor &c);
  void Parse(const std::string &prefix, const Texture *&t);
  void Parse(std::string &s);
  
private:

  std::vector<std::string> tokens;
  std::string lastError;
 
protected:

  void SetError(const std::string &);
};

#endif

