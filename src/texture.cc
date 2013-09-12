#include "common.h"

#include "texture.h"

#include "image.h"
#include "fileio.h"
#include "vector2.h"

#include "GLee.h"
#include <sys/time.h>
#include <unordered_map>

static std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
static time_t lastUpdate = 0;

Texture::Texture() :
  handle(0),
  size()
{}

Texture::Texture(const std::string &name) :
  name(name),
  handle(0),
  size()
{
  this->SetImage(Image::Load(name));
}

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

void Texture::Reload() {
  if (this->name == "") return;
  this->SetImage(Image::Load(this->name));
}

const Texture *Texture::Get(const std::string &name) {
  if (name == "") return nullptr;
  if (!textures[name])
    textures[name] = std::unique_ptr<Texture>(new Texture(name));
  return textures[name].get();
}

const Texture *Texture::Create(const std::string &name, const Image &image) {
  if (name == "") return nullptr;
  if (!textures[name])
    textures[name] = std::unique_ptr<Texture>(new Texture());
  textures[name]->SetImage(image);
  return textures[name].get();
}

void Texture::SetImage(const Image &image) {
  if (!this->handle) {
    glGenTextures(1, &this->handle);
  }

  this->size = image.GetSize();

  glBindTexture(GL_TEXTURE_2D, this->handle);
  glTexImage2D(GL_TEXTURE_2D, 0,
    image.HasAlpha() ? GL_RGBA : GL_RGB,
    this->size.x, this->size.y, 0,
    image.HasAlpha() ? GL_RGBA : GL_RGB,
    GL_UNSIGNED_BYTE,
    image.GetData()
  );

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void Texture::UpdateTextures() {
  time_t now = time(nullptr);
  if (now - lastUpdate < 2) return;

  // Log("Checking for updated textures since %u...\n", lastUpdate);

  for (auto &t : textures) {
    if (t.first[0] == '*') continue;
    time_t mtime = getFileChangeTime(t.first+".png");
    if (mtime > lastUpdate) {
      Log("Texture %s updated since %u...\n", t.first.c_str(), lastUpdate);
      t.second->Reload();
    }
  }

  lastUpdate = now;
}

void
Texture::GetFrameUV(size_t frame, size_t totalFrames, Vector2 &uv1, Vector2 &uv2) const {
  size_t countX = totalFrames;
  size_t countY = (totalFrames+7)/8;

  if (totalFrames > 8) {
    countX = 8;
  }

  float w = 1.0f/countX;
  float h = 1.0f/countY;

  uv1 = Vector2( (frame%countX)*w, (frame/countY)*h );
  uv2 = uv1 + Vector2( w, h );
}
