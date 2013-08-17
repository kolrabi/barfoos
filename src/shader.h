#ifndef BARFOOS_SHADER_H
#define BARFOOS_SHADER_H

#include "common.h"

#include "GLee.h"

#include <unordered_map>

class Shader final {
public:

  Shader(const std::string &name);
  Shader(const Shader &) = delete;
  ~Shader();
  
  Shader &operator=(const Shader &) = delete;

  void Uniform(const std::string &name, int value) const;
  void Uniform(const std::string &name, float value) const;
  void Uniform(const std::string &name, const IColor &value, float alpha = 1.0) const;
  void Uniform(const std::string &name, const std::vector<IColor> &value) const;

  void Uniform(const std::string &name, const Vector3 &value) const;
  void Uniform(const std::string &name, const std::vector<Vector3> &value) const;
  void Uniform(const std::string &name, const Matrix4 &value) const;
  
  GLhandleARB GetProgram() const { return program; }

private:

  GLhandleARB program;

  int GetUniformLocation(const std::string &name) const;
  mutable std::unordered_map<std::string, int> locations;
};


#endif
