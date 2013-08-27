#include "texture.h"
#include "image.h"

#include "util.h"

#include "GLee.h"

#include <sys/time.h>
#include <unordered_map>

static std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
static time_t lastUpdate = 0;

Texture::Texture() :
  handle(0),
  size()
{}

Texture::~Texture() {
  if (handle) glDeleteTextures(1, &handle);
}

Texture::Texture(Texture &&rhs) :
  handle(rhs.handle),
  size(rhs.size)
{
  rhs.handle = 0;
  rhs.size = Point();
}

const Texture *loadTexture(const std::string &name, const Texture * tex) {
  if (name == "") return nullptr;

  if (textures[name] && !tex) return textures[name].get();
  
  Image image = Image::Load(name);
  return updateTexture(name, image);
}

const Texture *updateTexture(const std::string &name, const Image &image) {
  if (!textures[name]) {
    textures[name] = std::unique_ptr<Texture>(new Texture());
    glGenTextures(1, &textures[name]->handle);
  }

  Texture *tex = textures[name].get();
  tex->size = image.GetSize();

  glBindTexture(GL_TEXTURE_2D, tex->handle);

  glTexImage2D(GL_TEXTURE_2D, 0, image.HasAlpha() ? GL_RGBA : GL_RGB, tex->size.x, tex->size.y, 0, image.HasAlpha() ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, image.GetData());
  
  //glGenerateMipmapEXT(GL_TEXTURE_2D);
  
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  return tex;
}

const Texture *
noiseTexture(const Point &size, const Vector3 &scale, const Vector3 &offset) {
  Image image = Image::Noise(size, scale, offset);

  unsigned int texture = 0;
  glGenTextures(1, &texture);

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, image.HasAlpha() ? GL_RGBA : GL_RGB, size.x, size.y, 0, image.HasAlpha() ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, image.GetData());
  glGenerateMipmapEXT(GL_TEXTURE_2D);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  std::string name = "*";
  name += size;
  name += scale;
  name += offset;

  Log("Generated noise texture '%s' as %u.\n", name.c_str(), texture);

  textures[name] = std::unique_ptr<Texture>(new Texture());
  textures[name]->handle = texture;
  textures[name]->size = size;

  return textures[name].get();
}

void updateTextures() {
  time_t now = time(nullptr);
  if (now - lastUpdate < 2) return;

  // Log("Checking for updated textures since %u...\n", lastUpdate);

  for (auto &t : textures) {
    if (t.first[0] == '*') continue;
    time_t mtime = getFileChangeTime(t.first+".png");
    if (mtime > lastUpdate) {
      Log("Texture %s updated since %u...\n", t.first.c_str(), lastUpdate);
      loadTexture(t.first, t.second.get());
    }
  }

  lastUpdate = now;
}

