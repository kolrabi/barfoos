#ifndef BARFOOS_VECTOR2_H
#define BARFOOS_VECTOR2_H

struct Vector2 {
  float x,y;

  Vector2()                  : x(0), y(0) {}
  Vector2(float f)           : x(f), y(f) {}
  Vector2(float x, float y)  : x(x), y(y) {}

  bool operator ==(const Vector2 &o) const {
    return x == o.x && y == o.y;
  }

  bool operator !=(const Vector2 &o) const {
    return x != o.x || y != o.y;
  }

  Vector2 operator +(const Vector2 &o) const {
    return Vector2(x+o.x, y+o.y);
  }

  Vector2 operator -(const Vector2 &o) const {
    return Vector2(x-o.x, y-o.y);
  }

  Vector2 operator +(const float &o) const {
    return Vector2(x+o, y+o);
  }

  Vector2 operator -(const float &o) const {
    return Vector2(x-o, y-o);
  }

  Vector2 operator -() const {
    return Vector2(-x, -y);
  }

  Vector2 operator *(const float f) const {
    return Vector2(x*f, y*f);
  }

  Vector2 operator /(const float f) const {
    return Vector2(x/f, y/f);
  }

  Vector2 operator *(const Vector2 &o) const {
    return Vector2(x*o.x, y*o.y);
  }

  Vector2 operator /(const Vector2 &o) const {
    return Vector2(x/o.x, y/o.y);
  }

  Vector2 Min(const Vector2 &o) const {
    return Vector2( std::min(x,o.x), std::min(y,o.y) );
  }

  Vector2 Max(const Vector2 &o) const {
    return Vector2( std::max(x,o.x), std::max(y,o.y) );
  }

  float GetSquareMag() const {
    return x*x + y*y;
  }

  float GetMag() const {
    return std::sqrt(x*x + y*y);
  }

  float Dot(const Vector2 &o) const {
    return x*o.x + y*o.y;
  }

  Vector2 Normalize() const {
    return *this * (1.0/GetMag());
  }

  Vector2 XY() const { return *this; }
  Vector2 YX() const { return Vector2(y,x); }

  operator std::string() const;
};


#endif
