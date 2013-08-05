#include <cstring>
#include <cstdio>
#include <algorithm>

#include "deserializer.h"
#include "ivector3.h"
#include "vector3.h"
#include "icolor.h"

Deserializer::Deserializer() :
  strings(),
  byteCount(0),
  bytePos(0),
  bytes(nullptr)
{
}

Deserializer::~Deserializer() {
  delete [] this->bytes;
}

bool Deserializer::LoadFromFile(FILE *f, const char *magic, size_t magiclen) {
  strings.clear();
  delete [] this->bytes;
  this->bytes = nullptr;
  this->byteCount = 0;
  this->bytePos = 0;
  
  char magic2[magiclen];
  if (!fread(magic2, magiclen, 1, f)) return false;
  if (memcmp(magic2, magic, magiclen)) return false;
  
  uint32_t stringCount = fgetc(f) | (fgetc(f) << 8) | (fgetc(f) << 16) | (fgetc(f) << 24);
  if (feof(f)) return false;
  
  std::string str;
  
  for (size_t i = 0; i<stringCount; i++) {
    int c = fgetc(f);
    while(c) {
      if (c == EOF) return false;
      
      str += (char)c;
      c = fgetc(f);
    }
    Log("string: %u %s\n", i, str.c_str());
    strings.push_back(str);
    str = "";
  }
  
  this->byteCount = fgetc(f) | (fgetc(f) << 8) | (fgetc(f) << 16) | (fgetc(f) << 24);
  if (feof(f)) return false;
  
  this->bytes = new uint8_t[this->byteCount];
  
  if (!fread(this->bytes, this->byteCount, 1, f)) return f;
  
  return true;
}

Deserializer &Deserializer::operator >> (uint8_t &v) {
  v = this->bytes[this->bytePos++];
  return *this;
}

Deserializer &Deserializer::operator >> (uint16_t &v) {
  uint8_t l,h;
  self >> l;
  self >> h;
  v = l | (h<<8);  
  return *this;
}

Deserializer &Deserializer::operator >> (uint32_t &v) {
  uint16_t l,h;
  self >> l;
  self >> h;
  v = l | (h<<16);  
  return *this;
}

Deserializer &Deserializer::operator >> (uint64_t &v) {
  uint32_t l,h;
  self >> l;
  self >> h;
  v = l | ((uint64_t)h<<32);  
  return *this;
}

Deserializer &Deserializer::operator >> (int8_t &v) {
  return self >> (uint8_t&)v;
}

Deserializer &Deserializer::operator >> (int16_t &v) {
  return self >> (uint16_t&)v;
}

Deserializer &Deserializer::operator >> (int32_t &v) {
  return self >> (uint32_t&)v;
}

Deserializer &Deserializer::operator >> (int64_t &v) {
  return self >> (uint64_t&)v;
}

Deserializer &Deserializer::operator >> (float &v) {
  union {
    float f;
    uint32_t i;
  } u;
  self >> u.i;
  v = u.f;
  return self;
}

Deserializer &Deserializer::operator >> (bool &v) {
  uint8_t b;
  self >> b;
  v = b;
  return self;
}

Deserializer &Deserializer::operator >> (std::string & str) {
  uint32_t index;
  self >> index;
  if (index >= strings.size()) {
    Log("string index %u out of bounds\n", index);
//    return self;
  }
  str = strings[index];
  return self;
}

Deserializer &Deserializer::operator >> (std::vector<bool> &v) {
  uint32_t size;
  self >> size;

  uint32_t s = size/8+1;
  
  Log("deser vector<bool>: %u bits %u bytes\n", size, s);
  
  v.clear();
  
  size_t count = 0;
  
  for (uint32_t n = 0; n<s; n++) {
    size_t pos = n*8;
    uint8_t t;
    self >> t;
    count++;
    for (size_t i = 0; i<8; i++) {
      if (i+pos < size) v.push_back(t & (1 << i));
    }
  }
  
  Log("%u %u\n", v.size(), count);
  
  return self;
}

Deserializer &Deserializer::operator >> (IVector3 &v) {
  return self >> v.x >> v.y >> v.z;
}

Deserializer &Deserializer::operator >> (Vector3 &v) {
  return self >> v.x >> v.y >> v.z;
}

Deserializer &Deserializer::operator >> (IColor &v) {
  return self >> v.r >> v.g >> v.b;
}
