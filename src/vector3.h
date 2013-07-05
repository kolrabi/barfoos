
#include <cmath>
static constexpr float pi = std::atan(1)*4;

struct Vector3 {
  float x,y,z;

  Vector3() : x(0), y(0), z(0) {}
  Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

  Vector3 operator +(const Vector3 &o) const {
    return Vector3(x+o.x, y+o.y, z+o.z);
  }
  
  Vector3 operator -(const Vector3 &o) const {
    return Vector3(x-o.x, y-o.y, z-o.z);
  }

  Vector3 operator -() const {
    return Vector3(-x, -y, -z);
  }
  
  Vector3 operator *(const float f) const {
    return Vector3(x*f, y*f, z*f);
  }

  Vector3 operator /(const float f) const {
    return Vector3(x/f, y/f, z/f);
  }

  Vector3 operator *(const Vector3 &o) const {
    return Vector3(x*o.x, y*o.y, z*o.z);
  }
  
  Vector3 operator /(const Vector3 &o) const {
    return Vector3(x/o.x, y/o.y, z/o.z);
  }

  float GetSquareMag() const {
    return x*x + y*y + z*z;
  }
  
  float GetMag() const {
    return std::sqrt(x*x + y*y + z*z);
  }

  Vector3 EulerToVector() const { 
    return Vector3( cos(x)*cos(y), sin(y), sin(x)*cos(y) );
  }
  
  Vector3 Cross(const Vector3 &o) const {
    return Vector3( o.z * y - o.y * z, o.x * z - o.z * x, o.y * x - o.x * y);
  }

  float Dot(const Vector3 &o) const {
    return x*o.x + y*o.y + z*o.z; 
  }

  Vector3 Normalize() const {
    return *this * (1.0/GetMag());
  }

  Vector3 Horiz() const {
    return Vector3(x, 0, z);
  }
   
  Vector3 Vert() const {
    return Vector3(0, y, 0);
  }
  
  static Vector3 Rand() {
    return Vector3(rand()/(float)RAND_MAX, rand()/(float)RAND_MAX, rand()/(float)RAND_MAX);
  }
  
  Vector3 XYZ() const { return *this; }
  Vector3 XZY() const { return Vector3(x,z,y); }
  Vector3 YXZ() const { return Vector3(y,x,z); }
  Vector3 YZX() const { return Vector3(y,z,x); }
  Vector3 ZXY() const { return Vector3(z,x,y); }
  Vector3 ZYX() const { return Vector3(z,y,x); }
};

static inline std::ostream & operator<< (std::ostream &out, const Vector3 &v) {
  out << "[" << v.x << ", " << v.y << ", " << v.z << "]";
  return out;
}

// TODO: move to matrix4.h

/*
 * OpenGL order:
 * +-           -+
 * | 0  4  8  12 |
 * | 1  5  9  13 |
 * | 2  6  10 14 |
 * | 3  7  11 15 |
 * +-           -+
 */
struct Matrix4 {
  float m[16];
  
  Matrix4() {
    m[ 0] = 1; m[ 1] = 0; m[ 2] = 0; m[ 3] = 0;
    m[ 4] = 0; m[ 5] = 1; m[ 6] = 0; m[ 7] = 0;
    m[ 8] = 0; m[ 9] = 0; m[10] = 1; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
  }
  
  Matrix4(const Matrix4 &o) {
    ::memcpy(m, o.m, sizeof(m));
  }

  Matrix4(float *o) {
    ::memcpy(m, o, sizeof(m));
  }
  
  float &operator()(int i, int j) {
    return m[i*4+j];
  }

  float operator()(int i, int j) const {
    return m[i*4+j];
  }
  
  Matrix4 operator*(const Matrix4 &a) const {
    const Matrix4 &b = *this;
    Matrix4 t;
  
    // AB_ij = Sum(k: 1 -> m | A_ik * b_kj)
    for (int i=0; i<4; i++) {
      for (int j=0; j<4; j++) {
        t(i,j) = a(i,0)*b(0,j) + a(i,1)*b(1,j) + a(i,2)*b(2,j) + a(i,3)*b(3,j);
      }
    }
    
    return t;
  }
  
  Vector3 operator*(const Vector3 &v) const {
    const Matrix4 &a = *this;
    
    float w[4];
    w[0] = a(0,0) * v.x + a(0,1) * v.y + a(0,2) * v.z + a(0,3);
    w[1] = a(1,0) * v.x + a(1,1) * v.y + a(1,2) * v.z + a(1,3);
    w[2] = a(2,0) * v.x + a(2,1) * v.y + a(2,2) * v.z + a(1,3);
    w[3] = a(3,0) * v.x + a(3,1) * v.y + a(3,2) * v.z + a(1,3);
    
    if (w[3]) {
      return Vector3(w[0]/w[3], w[1]/w[3], w[2]/w[3]);
    } else {
      return Vector3(w[0], w[1], w[2]);
    }
  }
  
  static Matrix4 Perspective(float fovy, float aspect, float znear, float zfar) {
    Matrix4 t;
    float f = 1.0 / std::tan((fovy/180*pi)/2); // cotan(fovy/2)
    t(0,0) = f/aspect;
    t(1,1) = f;
    t(2,2) = (zfar+znear)/(znear-zfar);
    t(3,2) = (2*zfar*znear)/(znear-zfar);
    t(2,3) = -1;
    t(3,3) = 0;
    return t;
  }
  
  static Matrix4 Ortho(float left, float right, float bottom, float top, float znear, float zfar) {
    Matrix4 t;
    t(0,0) =  2 / (right-left); t(3,0) = - (right+left) / (right-left);
    t(1,1) =  2 / (top-bottom); t(3,1) = - (top+bottom) / (top-bottom);
    t(2,2) = -2 / (zfar-znear); t(3,2) = - (zfar+znear) / (zfar-znear);
    return t;
  }
  
  static Matrix4 LookAt(const Vector3 &eye, const Vector3 &center, const Vector3 &up) {
    return LookFrom(eye, center-eye, up);
  }

  static Matrix4 LookFrom(const Vector3 &eye, const Vector3 &fwd, const Vector3 &up) {
    Vector3 f = fwd.Normalize();
    Vector3 u = up.Normalize();
    Vector3 s = f.Cross(u).Normalize();
    u = s.Cross(f);
    
    Matrix4 t;
    t(0,0) =  s.x; t(1,0) =  s.y; t(2,0) =  s.z;
    t(0,1) =  u.x; t(1,1) =  u.y; t(2,1) =  u.z;
    t(0,2) = -f.x; t(1,2) = -f.y; t(2,2) = -f.z;
    return t * Translate(-eye);
  }
  
  static Matrix4 Translate(const Vector3 &v) {
    Matrix4 t;
    t(3,0) = v.x;
    t(3,1) = v.y;
    t(3,2) = v.z;
    return t;
  }
  
  static Matrix4 Rotate(float angle, const Vector3 &axis) {
    Vector3 v = axis.Normalize();
    float   a = angle/180*pi;
    float   c = std::cos(a);
    float   s = std::sin(a);
    
    Matrix4 t;
    t(0,0) = v.x*v.x*(1-c)+    c; t(1,0) = v.x*v.y*(1-c)-v.z*s; t(2,0) = v.x*v.z*(1-c)+v.y*s;
    t(0,1) = v.y*v.x*(1-c)+v.z*s; t(1,1) = v.y*v.y*(1-c)+    c; t(2,1) = v.y*v.z*(1-c)-v.x*s;
    t(0,2) = v.z*v.x*(1-c)-v.y*s; t(1,2) = v.z*v.y*(1-c)+v.x*s; t(2,2) = v.z*v.z*(1-c)+    c;
    return t;
  }
  
  static Matrix4 Scale(const Vector3 &f) {
    Matrix4 t;
    t(0,0) = f.x;
    t(1,1) = f.y;
    t(2,2) = f.z;
    return t;
  }
};

static inline std::ostream & operator<< (std::ostream &out, const Matrix4 &m) {
  out << "/" << m(0,0) << " " << m(1,0) << m(2,0) << " " << m(3,0) << "\\" << std::endl;
  out << "|" << m(0,1) << " " << m(1,1) << m(2,1) << " " << m(3,1) << "|"  << std::endl;
  out << "|" << m(0,2) << " " << m(1,2) << m(2,2) << " " << m(3,2) << "|"  << std::endl;
  out << "\\"<< m(0,3) << " " << m(1,3) << m(2,3) << " " << m(3,3) << "/"  << std::endl;
  return out;
}
