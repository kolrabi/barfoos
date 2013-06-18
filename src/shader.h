#ifndef BARFOOS_SHADER_H
#define BARFOOS_SHADER_H

#include "common.h"

class Shader final {
public:

  Shader(const std::string &name);
  ~Shader();

  void Uniform(const std::string &name, int value);
  void Uniform(const std::string &name, const IColor &value);

  void Bind();
  static void Unbind();

private:

  unsigned int program;
};


#endif
