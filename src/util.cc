#include "common.h"

#include "util.h"

#include "2d.h"
#include "vector2.h"
#include "vector3.h"
#include "vector4.h"
#include "matrix4.h"
#include "aabb.h"

#include "serializer.h"
#include "deserializer.h"

#include <cstring>
#include <cstdio>

float Wave(float x, float z, float t, float a) {
  return a * cos( ((x+z)*0.4+t*0.4) )
           * cos( ((x-z)*0.6+t*0.7) );
}

Point::operator std::string() const {
  char tmp[256];
  snprintf(tmp, sizeof(tmp), "[%d,%d]", x,y);
  return tmp;
}

Vector2::operator std::string() const {
  char tmp[256];
  snprintf(tmp, sizeof(tmp), "[%f,%f]", x,y);
  return tmp;
}

Vector3::operator std::string() const {
  char tmp[256];
  snprintf(tmp, sizeof(tmp), "[%f,%f,%f]", x,y,z);
  return tmp;
}

Vector4::operator std::string() const {
  char tmp[256];
  snprintf(tmp, sizeof(tmp), "[%f,%f,%f,%f]", x,y,z,w);
  return tmp;
}

Matrix4::operator std::string() const {
  char tmp[256];
  snprintf(tmp, sizeof(tmp), "[%2.5f,%2.5f,%2.5f,%2.5f\n"
                             " %2.5f,%2.5f,%2.5f,%2.5f\n" 
                             " %2.5f,%2.5f,%2.5f,%2.5f\n" 
                             " %2.5f,%2.5f,%2.5f,%2.5f]\n",
  
  self(0,0), self(1,0), self(2,0), self(3,0), self(0,1), self(1,1), self(2,1), self(3,1),
  self(0,2), self(1,2), self(2,2), self(3,2), self(0,3), self(1,3), self(2,3), self(3,3)
  
  );
  return tmp;
}

std::vector<std::string> Tokenize(const char *l) {
  std::vector<std::string> tokens;
  
  std::string line = l;
  char *p = &line[0];
  char *q;
  
  // skip whitespace
  while(*p && strchr(" \r\n\t", *p)) p++;
  
  // end of string or first character is #?
  if (*p == 0 || *p == '#') return tokens;
  
  bool inString = *p == '\"';
  if (inString) p++;
  
  do {
    // find end of token
    q = p;
    while(*q) {
      if (inString) {
        if (*q == '\"') {
          inString = false;
          break;
        }
      } else {
        if (strchr(" \r\n\t", *q)) break;
      }
      q++;
    }
    
    // terminate token
    if (q) *q = 0;
    tokens.push_back(p);
    
    // not end of line? then skip whitespace to next token
    if (q) { 
      p = q+1; 
      while(*p && strchr(" \r\n\t", *p)) p++;
    }
    inString = *p == '\"';
    if (inString) p++;
  } while(q && *p);
  return tokens;
}

Serializer &operator << (Serializer &ser, const AABB &aabb) {
  return ser << aabb.center << aabb.extents;
}

Deserializer &operator >> (Deserializer &deser, AABB &aabb) {
  return deser >> aabb.center >> aabb.extents;
}
