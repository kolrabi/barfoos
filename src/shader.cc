#include "GLee.h"

#include "shader.h"
#include "icolor.h"
#include "util.h"
#include "matrix4.h"

Shader::Shader(const std::string &name) :
  program(glCreateProgramObjectARB())
{
  const char *txt;
  char tmp[1024];
  GLsizei l;

  unsigned int vshad = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
  std::string vtext = loadAssetAsString("shaders/"+name+".vs");
  txt = vtext.c_str();
  glShaderSourceARB(vshad, 1, &txt, 0);
  glCompileShader(vshad);
  glAttachObjectARB(program, vshad);

  glGetInfoLogARB(vshad, sizeof(tmp)-1, &l, tmp);
  tmp[l] = 0;
  
  if (l) Log("Vertex shader %s result:\n%s\n", name.c_str(), tmp);

  unsigned int fshad = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
  std::string ftext = loadAssetAsString("shaders/"+name+".fs");
  txt = ftext.c_str();
  glShaderSourceARB(fshad, 1, &txt, 0);
  glCompileShader(fshad);
  glAttachObjectARB(program, fshad);
  
  glGetInfoLogARB(fshad, sizeof(tmp)-1, &l, tmp);
  tmp[l] = 0;
  
  if (l) Log("Fragment shader %s result:\n%s\n", name.c_str(), tmp);

  glLinkProgramARB(program);
  
  glGetInfoLogARB(program, sizeof(tmp)-1, &l, tmp);
  tmp[l] = 0;
  
  if (l) Log("Shader program %s result:\n%s\n", name.c_str(), tmp);

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
Shader::Uniform(const std::string &name, const std::vector<IColor> &value) const {
  float rgb[4*value.size()];
  for (size_t i=0; i<value.size(); i++) {
    rgb[i*4+0] = value[i].r / 255.0;
    rgb[i*4+1] = value[i].g / 255.0;
    rgb[i*4+2] = value[i].b / 255.0;
    rgb[i*4+3] = 1.0;
  }

  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  glUniform4fv(loc, value.size(), rgb);
}

void 
Shader::Uniform(const std::string &name, const Vector3 &value) const {
  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  float xyz[3] = { value.x, value.y, value.z };
  glUniform3fv(loc, 1, xyz);
}

void 
Shader::Uniform(const std::string &name, const std::vector<Vector3> &value) const {
  float xyz[3 * value.size()];
  for (size_t i=0; i<value.size(); i++) {
    xyz[i*3+0] = value[i].x;
    xyz[i*3+1] = value[i].y;
    xyz[i*3+2] = value[i].z;
  }

  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  glUniform3fv(loc, value.size(), xyz);
}
  
void 
Shader::Uniform(const std::string &name, const Matrix4 &value) const {
  glUseProgramObjectARB(program);
  int loc = glGetUniformLocationARB(program, name.c_str());
  glUniformMatrix4fv(loc, 1, false, value.m);
}

