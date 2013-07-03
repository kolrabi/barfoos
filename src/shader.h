#ifndef BARFOOS_SHADER_H
#define BARFOOS_SHADER_H

#include "common.h"

struct IColor;

class Shader final {
public:

  Shader(const std::string &name);
  ~Shader();

  void Uniform(const std::string &name, int value) const;
  void Uniform(const std::string &name, float value) const;
  void Uniform(const std::string &name, const IColor &value) const;
  
  unsigned int GetProgram() const { return program; }

private:

  unsigned int program;
};


#endif
