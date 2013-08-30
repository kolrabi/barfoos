#include <cstring>
#include <cstdio>
#include <algorithm>

#include "common.h"

#include "serializer.h"

#include "ivector3.h"
#include "vector3.h"
#include "icolor.h"

Serializer::Serializer(
  const char *magic, 
  size_t magiclen
) :
  strings(),
  byteCount(0),
  byteCapacity(1024),
  bytes(new uint8_t[this->byteCapacity]),
  magic(new char[magiclen]),
  magicLen(magiclen)
{
  memcpy(this->magic, magic, magiclen);
}

Serializer::~Serializer() {
  delete [] this->magic;
  delete [] this->bytes;
}

void Serializer::GrowBy(size_t n) {
  if (n+this->byteCount < this->byteCapacity) 
    return;
    
  size_t newCapacity = this->byteCapacity;
  while (n+this->byteCount >= newCapacity) 
    newCapacity *= 2;
  
  uint8_t *newBytes = new uint8_t[newCapacity];
  memcpy(newBytes, this->bytes, this->byteCount);
  delete [] this->bytes;
  this->bytes = newBytes;
  this->byteCapacity = newCapacity;
}

uint32_t Serializer::AddString(const std::string &str) {
  auto iter = std::find(strings.begin(), strings.end(), str);
  if (iter == strings.end()) {
    strings.push_back(str);
    return strings.size()-1;
  }
  return iter - strings.begin();
}

bool Serializer::WriteToFile(FILE *f) {
  if (!f) return false;
  
  if (fwrite(this->magic, this->magicLen, 1, f) != 1) {
    return false;
  }

  uint8_t len[4] = {
    uint8_t((strings.size()    ) & 0xFF),
    uint8_t((strings.size()>> 8) & 0xFF),
    uint8_t((strings.size()>>16) & 0xFF),
    uint8_t((strings.size()>>24) & 0xFF)
  };  
  
  if (fwrite(len, sizeof(len), 1, f) != 1) return false;
  
  for (auto &s:strings) {
    if (fwrite(s.c_str(), s.size()+1, 1, f) != 1) return false;
  }

  uint8_t count[4] = {
    uint8_t((this->byteCount    ) & 0xFF),
    uint8_t((this->byteCount>> 8) & 0xFF),
    uint8_t((this->byteCount>>16) & 0xFF),
    uint8_t((this->byteCount>>24) & 0xFF)
  };  
  
  if (fwrite(count, sizeof(count), 1, f) != 1) return false;
  
  if (fwrite(this->bytes, this->byteCount, 1, f) != 1) {
    return false;
  }
  return true;
}

Serializer &Serializer::operator << (uint8_t v) {
  GrowBy(1);
  this->bytes[this->byteCount++] = v;
  return *this;
}

Serializer &Serializer::operator << (uint16_t v) {
  GrowBy(2);
  (*this) << (uint8_t)((v   )&0xFF);
  (*this) << (uint8_t)((v>>8)&0xFF);
  return *this;
}

Serializer &Serializer::operator << (uint32_t v) {
  GrowBy(4);
  (*this) << (uint16_t)((v    )&0xFFFF);
  (*this) << (uint16_t)((v>>16)&0xFFFF);
  return *this;
}

Serializer &Serializer::operator << (uint64_t v) {
  GrowBy(8);
  (*this) << (uint32_t)((v    )&0xFFFFFFFF);
  (*this) << (uint32_t)((v>>32)&0xFFFFFFFF);
  return *this;
}

Serializer &Serializer::operator << (int8_t v) {
  return (*this) << (uint8_t)v;
}

Serializer &Serializer::operator << (int16_t v) {
  return (*this) << (uint16_t)v;
}

Serializer &Serializer::operator << (int32_t v) {
  return (*this) << (uint32_t)v;
}

Serializer &Serializer::operator << (int64_t v) {
  return (*this) << (uint64_t)v;
}

Serializer &Serializer::operator << (float v) {
  union {
    float f;
    uint32_t i;
  } u;
  u.f = v;
  return (*this) << u.i;
}

Serializer &Serializer::operator << (bool v) {
  return self << uint8_t(v?0xAA:0);
}

Serializer &Serializer::operator << (const std::string & str) {
  return (*this) << AddString(str);
}

Serializer &Serializer::operator << (const std::vector<bool> &v) {
  uint32_t s = v.size()/8+1;
  
  Log("ser vector<bool>: %u bits %u bytes\n", v.size(), s);
  
  self << (uint32_t)v.size();
  for (uint32_t n = 0; n<s; n++) {
    size_t pos = n*8;
    uint8_t t = 0;
    for (size_t i = 0; i<8; i++) {
      if (i+pos < v.size() && v[i+pos]) t |= 1 << i;
    }
    self << t;
  }
  
  return self;
}

Serializer &Serializer::operator << (const IVector3 &v) {
  return self << v.x << v.y << v.z;
}

Serializer &Serializer::operator << (const Vector3 &v) {
  return self << v.x << v.y << v.z;
}

Serializer &Serializer::operator << (const IColor &v) {
  return self << v.r << v.g << v.b;
}
