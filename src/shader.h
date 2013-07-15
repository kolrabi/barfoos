#ifndef BARFOOS_SHADER_H
#define BARFOOS_SHADER_H

#include "common.h"

class Shader final {
public:

  Shader(const std::string &name);
  ~Shader();

  void Uniform(const std::string &name, int value) const;
  void Uniform(const std::string &name, float value) const;
  void Uniform(const std::string &name, const IColor &value, float alpha = 1.0) const;
  void Uniform(const std::string &name, const std::vector<IColor> &value) const;

  void Uniform(const std::string &name, const Vector3 &value) const;
  void Uniform(const std::string &name, const std::vector<Vector3> &value) const;
  void Uniform(const std::string &name, const Matrix4 &value) const;
  
  unsigned int GetProgram() const { return program; }

private:

  unsigned int program;
};


#endif
