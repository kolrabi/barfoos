#ifndef BARFOOS_MATRIX4_H
#define BARFOOS_MATRIX4_H

#include "common.h"

#include "vector3.h"

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
  
  Matrix4 Mat3() const {
    Matrix4 t(*this);
    t(3,0) = 0;
    t(3,1) = 0;
    t(3,2) = 0;
    t(0,3) = 0;
    t(1,3) = 0;
    t(2,3) = 0;
    t(3,3) = 1;
    return t;
  }
  
  Matrix4 Transpose() const {
    const Matrix4 &a = *this;
    Matrix4 t;
    for (int i=0; i<4; i++) {
      for (int j=0; j<4; j++) {
        t(i,j) = a(j,i);
      }
    }
    return t;
  }
  
  Matrix4 Inverse() const {
    const Matrix4 &m = *this;
    
    // Inverse = adjoint / det. (See linear algebra texts.)

    // pre-compute 2x2 dets for last two rows when computing
    // cofactors of first two rows.
    float d12 = (m(2,0) * m(3,1) - m(3,0) * m(2,1));
    float d13 = (m(2,0) * m(3,2) - m(3,0) * m(2,2));
    float d23 = (m(2,1) * m(3,2) - m(3,1) * m(2,2));
    float d24 = (m(2,1) * m(3,3) - m(3,1) * m(2,3));
    float d34 = (m(2,2) * m(3,3) - m(3,2) * m(2,3));
    float d41 = (m(2,3) * m(3,0) - m(3,3) * m(2,0));

    Matrix4 t;
    t(0,0) =  (m(1,1) * d34 - m(1,2) * d24 + m(1,3) * d23);
    t(0,1) = -(m(1,0) * d34 + m(1,2) * d41 + m(1,3) * d13);
    t(0,2) =  (m(1,0) * d24 + m(1,1) * d41 + m(1,3) * d12);
    t(0,3) = -(m(1,0) * d23 - m(1,1) * d13 + m(1,2) * d12);

    // Compute determinant as early as possible using these cofactors.
    float  det = m(0,0) * t(0,0) + m(0,1) * t(0,1) + m(0,2) * t(0,2) + m(0,3) * t(0,3);

    // Run singularity test.
    if (det == 0.0) {
      return Matrix4();
    }

    float  invDet = 1.0f / det;

    // Compute rest of inverse.
    t(0,0) *= invDet;
    t(0,1) *= invDet;
    t(0,2) *= invDet;
    t(0,3) *= invDet;

    t(1,0) = -(m(0,1) * d34 - m(0,2) * d24 + m(0,3) * d23) * invDet;
    t(1,1) =  (m(0,0) * d34 + m(0,2) * d41 + m(0,3) * d13) * invDet;
    t(1,2) = -(m(0,0) * d24 + m(0,1) * d41 + m(0,3) * d12) * invDet;
    t(1,3) =  (m(0,0) * d23 - m(0,1) * d13 + m(0,2) * d12) * invDet;

    // Pre-compute 2x2 dets for first two rows when computing cofactors
    // of last two rows.
    d12 = m(0,0) * m(1,1) - m(1,0) * m(0,1);
    d13 = m(0,0) * m(1,2) - m(1,0) * m(0,2);
    d23 = m(0,1) * m(1,2) - m(1,1) * m(0,2);
    d24 = m(0,1) * m(1,3) - m(1,1) * m(0,3);
    d34 = m(0,2) * m(1,3) - m(1,2) * m(0,3);
    d41 = m(0,3) * m(1,0) - m(1,3) * m(0,0);

    t(2,0) =  (m(3,1) * d34 - m(3,2) * d24 + m(3,3) * d23) * invDet;
    t(2,1) = -(m(3,0) * d34 + m(3,2) * d41 + m(3,3) * d13) * invDet;
    t(2,2) =  (m(3,0) * d24 + m(3,1) * d41 + m(3,3) * d12) * invDet;
    t(2,3) = -(m(3,0) * d23 - m(3,1) * d13 + m(3,2) * d12) * invDet;
    
    t(3,0) = -(m(2,1) * d34 - m(2,2) * d24 + m(2,3) * d23) * invDet;
    t(3,1) =  (m(2,0) * d34 + m(2,2) * d41 + m(2,3) * d13) * invDet;
    t(3,2) = -(m(2,0) * d24 + m(2,1) * d41 + m(2,3) * d12) * invDet;
    t(3,3) =  (m(2,0) * d23 - m(2,1) * d13 + m(2,2) * d12) * invDet;

    return t;
  }
  
  Matrix4 ModelViewToNormal() const {
    return Inverse().Transpose();
  };
  
  static Matrix4 Perspective(float fovy, float aspect, float znear, float zfar) {
    Matrix4 t;
    float f = 1.0 / std::tan(fovy/180*Const::pi_2); // cotan(fovy/2)
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
    float   a = angle/180*Const::pi;
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

#endif
