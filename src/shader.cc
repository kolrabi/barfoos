#include "shader.h"
#include "icolor.h"
#include "util.h"

#include "GLee.h"

Shader::Shader(const std::string &name) {
  const char *txt;
  char tmp[1024];
  GLsizei l;

  program = glCreateProgramObjectARB();
  
  unsigned int vshad = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
  std::string vtext = loadAssetAsString("shaders/"+name+".vs");
  txt = vtext.c_str();
  glShaderSourceARB(vshad, 1, &txt, 0);
  glCompileShader(vshad);
  glAttachObjectARB(program, vshad);

  std::cerr << "vertex shader " << name << " result:" << std::endl;
  glGetInfoLogARB(vshad, sizeof(tmp)-1, &l, tmp);
  tmp[l] = 0;
  std::cerr << tmp << std::endl;

  unsigned int fshad = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
  std::string ftext = loadAssetAsString("shaders/"+name+".fs");
  txt = ftext.c_str();
  glShaderSourceARB(fshad, 1, &txt, 0);
  glCompileShader(fshad);
  glAttachObjectARB(program, fshad);
  
  std::cerr << "fragment shader " << name << " result:" << std::endl;
  glGetInfoLogARB(fshad, sizeof(tmp)-1, &l, tmp);
  tmp[l] = 0;
  std::cerr << tmp << std::endl;

  glLinkProgramARB(program);
  
  std::cerr << "program " << name << " result:" << std::endl;
  glGetInfoLogARB(program, sizeof(tmp)-1, &l, tmp);
  tmp[l] = 0;
  std::cerr << tmp << std::endl;

  glDeleteObjectARB(vshad);
  glDeleteObjectARB(fshad);
}

Shader::~Shader() {
  glDeleteObjectARB(program);
}

void
Shader::Uniform(const std::string &name, int value) const {
  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  glUniform1iARB(loc, value);
}

void
Shader::Uniform(const std::string &name, float value) const {
  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  glUniform1fARB(loc, value);
}

void 
Shader::Uniform(const std::string &name, const IColor &value, float alpha) const {
  float rgb[4] = { value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, alpha };
  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  glUniform4fv(loc, 1, rgb);
}

void 
Shader::Uniform(const std::string &name, const Vector3 &value) const {
  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  float xyz[3] = { value.x, value.y, value.z };
  glUniform3fv(loc, 1, xyz);
}
  
void 
Shader::Uniform(const std::string &name, const Matrix4 &value) const {
  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  glUniformMatrix4fv(loc, 1, false, value.m);
}

