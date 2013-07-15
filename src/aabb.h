#ifndef BARFOOS_AABB_H
#define BARFOOS_AABB_H

#include "vector3.h"

/** An axis aligned bounding box. */
struct AABB {
  /** Center of the box. */
  Vector3 center;
  
  /** Half the size of the box. */
  Vector3 extents;

  /** Test if another AABB overlaps the box. 
   * @param o AABB to check against.
   * @return true if @a this and @a o overlap.
   */
  bool Overlap(const AABB &o) const {
    return std::abs(center.x - o.center.x) <= (extents.x + o.extents.x) &&
           std::abs(center.y - o.center.y) <= (extents.y + o.extents.y) &&
           std::abs(center.z - o.center.z) <= (extents.z + o.extents.z);
  }

  /** Get the minimum vector of the box. */
  inline Vector3 Min() const { return center - extents; }
  
  /** Get the maximum vector of the box. */
  inline Vector3 Max() const { return center + extents; }

  /** Check if a point is inside the box. */
  bool PointInside(const Vector3 &p) const {
    if (p.x>center.x+extents.x) return false;
    if (p.y>center.y+extents.y) return false;
    if (p.z>center.z+extents.z) return false;
    if (p.x<center.x-extents.x) return false;
    if (p.y<center.y-extents.y) return false;
    if (p.z<center.z-extents.z) return false;
    return true;
  }
  
  AABB Grow(float amount) const {
    AABB aabb(*this);
    aabb.extents.x += amount;
    aabb.extents.y += amount;
    aabb.extents.z += amount;
    return aabb;
  }
  
  AABB Combine(const AABB &o) const {
    Vector3 min = Min().Min(o.Min());
    Vector3 max = Max().Max(o.Max());
    AABB aabb;
    aabb.center = (min+max) * 0.5;
    aabb.extents = max - aabb.center;
    return aabb;
  }
  
  /** Generate corner vertices for the box.
    * Also generates vertices for the sides.
    * @param[out] verts Where to store the vertices.
    * @TODO automate/optimize generation for a given aabb size
    * @TODO also cache these
    */
  void GetVertices(std::vector<Vector3> &verts) const {
    verts.push_back(Vector3(-extents.x, -extents.y, -extents.z));
    verts.push_back(Vector3(-extents.x, -extents.y,  extents.z));
    verts.push_back(Vector3( extents.x, -extents.y, -extents.z));
    verts.push_back(Vector3( extents.x, -extents.y,  extents.z));

    verts.push_back(Vector3(-extents.x, -extents.y/2, -extents.z));
    verts.push_back(Vector3(-extents.x, -extents.y/2,  extents.z));
    verts.push_back(Vector3( extents.x, -extents.y/2, -extents.z));
    verts.push_back(Vector3( extents.x, -extents.y/2,  extents.z));

    verts.push_back(Vector3(-extents.x,               0, -extents.z));
    verts.push_back(Vector3(-extents.x,               0,  extents.z));
    verts.push_back(Vector3( extents.x,               0, -extents.z));
    verts.push_back(Vector3( extents.x,               0,  extents.z));

    verts.push_back(Vector3(-extents.x,  extents.y/2, -extents.z));
    verts.push_back(Vector3(-extents.x,  extents.y/2,  extents.z));
    verts.push_back(Vector3( extents.x,  extents.y/2, -extents.z));
    verts.push_back(Vector3( extents.x,  extents.y/2,  extents.z));

    verts.push_back(Vector3(-extents.x,  extents.y, -extents.z));
    verts.push_back(Vector3(-extents.x,  extents.y,  extents.z));
    verts.push_back(Vector3( extents.x,  extents.y, -extents.z));
    verts.push_back(Vector3( extents.x,  extents.y,  extents.z));
  }

  /** Do a raycast against the box.
    * @param[in] start Start point of the ray.
    * @param[in] dir Normalized direction of the ray.
    * @param[out] t The distance of the hit if any.
    * @param[out] p The position of the hit if any.
    * @return true if ray hits the box.
    */
  bool Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const {
    if (PointInside(start)) return false;

    float tnear = -INFINITY;
    float tfar  =  INFINITY;

    Vector3 tmin, tmax;
    tmin.x = (Min().x - start.x) / dir.x; 
    tmax.x = (Max().x - start.x) / dir.x;

    if (tmin.x > tmax.x) std::swap(tmin.x, tmax.x); 

    if (tmin.x > tnear) tnear = tmin.x;
    if (tmax.x < tfar ) tfar  = tmax.x;

    if (tnear > tfar) {
      return false;
    }
 
    tmin.y = (Min().y - start.y) / dir.y;
    tmax.y = (Max().y - start.y) / dir.y;

    if (tmin.y > tmax.y) std::swap(tmin.y, tmax.y);
    
    if (tmin.y > tnear) tnear = tmin.y;
    if (tmax.y < tfar ) tfar  = tmax.y;
    
    if (tnear > tfar) {
      return false;
    }
    
    tmin.z = (Min().z - start.z) / dir.z;
    tmax.z = (Max().z - start.z) / dir.z;
    
    if (tmin.z > tmax.z) std::swap(tmin.z, tmax.z);

    if (tmin.z > tnear) tnear = tmin.z;
    if (tmax.z < tfar ) tfar  = tmax.z;
    
    if (tnear > tfar) {
      return false;
    }
    
    t = tnear;
    p = start + dir * t;
    return true;
  }
};

static inline std::ostream & operator<< (std::ostream &out, const AABB &aabb) {
  out << "{" << aabb.center << " +/- " << aabb.extents << "}";
  return out;
}

#endif

