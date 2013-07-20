#ifndef BARFOOS_2D_H
#define BARFOOS_2D_H

/** A 2d point. */
struct Point {
  /** X coordinate. */
  int x;
  
  /** Y coordinate. */
  int y;

  Point()             : x(0), y(0) {}  
  Point(int x, int y) : x(x), y(y) {}

  Point operator +(const Point &o) const { return Point(x+o.x, y+o.y); }
  Point operator -(const Point &o) const { return Point(x-o.x, y-o.y); }

  operator std::string() const;
};

/** A 2d rectangle. */
struct Rect {
  /** Top left corner of rectangle. */
  Point pos;
  
  /** Size of rectangle. */
  Point size;
  
  Rect()                                    : pos(),    size()      {} 
  Rect(const Point &pos, const Point &size) : pos(pos), size(size)  {}

  /** Check if a point is inside the rectangle. 
    * @param p Point to test.
    * @return true if @a p is inside the rectangle.
    */
  bool IsInside(const Point &p) const {    
    return p.x >= pos.x && p.x < pos.x+size.x &&
           p.y >= pos.y && p.y < pos.y+size.y;
  }

  /** Adjust rectangle so that it encloses a point.
    * Won't change the rectangle if point already lies inside.
    * @param o Point to use.
    * @return The updated rectangle.
    */
  Rect &operator +=(const Point &o) {
    if (o.x < pos.x) {
      size.x += pos.x-o.x;
      pos.x = o.x;
    }
    if (o.y < pos.y) {
      size.y += pos.y-o.y;
      pos.y = o.y;
    }
    if (o.x > pos.x+size.x) {
      size.x += pos.x+size.x-o.x;
    }
    if (o.y > pos.y) {
      size.y += pos.y+size.y-o.y;
    }
    return *this;
  }
  
  operator std::string () const;
};

#endif

