#ifndef BARFOOS_ICOLOR_H
#define BARFOOS_ICOLOR_H

#include <cmath>

struct IColor {
  int16_t r,g,b;

  IColor() : r(0), g(0), b(0) {}
  IColor(int r, int g, int b) : r(r), g(g), b(b) {}

  IColor Saturate(int max = 255) const {
    IColor c(*this);
    if (c.r < 0) c.r = 0;
    if (c.r > max) c.r = max;
    if (c.g < 0) c.g = 0;
    if (c.g > max) c.g = max;
    if (c.b < 0) c.b = 0;
    if (c.b > max) c.b = max;
    return c;
  }

  bool IsBlack() const {
    IColor c = Saturate();
    return c.r == 0 && c.g == 0 && c.b == 0;
  }

  inline IColor Max(const IColor &o) const {
    return IColor( r > o.r ? r : o.r,
                   g > o.g ? g : o.g,
                   b > o.b ? b : o.b );
  }

  IColor operator-(int n) const {
    return IColor( r < n ? 0 : r-n,
                   g < n ? 0 : g-n,
                   b < n ? 0 : b-n );
  }

  IColor operator/(int n) const {
    return IColor( r/n, g/n, b/n);
  }

  IColor operator*(int n) const {
    return IColor( r*n, g*n, b*n);
  }

  IColor operator*(float f) const {
    return IColor( r*f, g*f, b*f);
  }

  IColor operator+(const IColor &o) const {
    return IColor( r+o.r, g+o.g, b+o.b );
  }

  bool operator==(const IColor &o) const {
    return r==o.r && g==o.g && b==o.b;
  }

  bool operator>=(const IColor &o) const {
    return r>=o.r || g>=o.g || b>=o.b;
  }

  bool operator>(const IColor &o) const {
    return r>o.r || g>o.g || b>o.b;
  }

  bool operator!=(const IColor &o) const {
    return !(r==o.r && g==o.g && b==o.b);
  }
  
  IColor Gamma(float gamma, int max=255) const {
    return IColor( std::pow(r/(float)max, gamma)*max,
                   std::pow(g/(float)max, gamma)*max,
                   std::pow(b/(float)max, gamma)*max );
  }

  static IColor Lerp(const IColor &a, const IColor &b, float t) {
    return IColor( a.r * (1.0-t) + b.r * t,
                   a.g * (1.0-t) + b.g * t,
                   a.b * (1.0-t) + b.b * t );
  }
};

#endif

