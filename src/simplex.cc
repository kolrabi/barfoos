#include "simplex.h"

#include "vector3.h"

static Vector3 grad3[12] = {
  {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
  {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
  {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
};

static int p[] = {
  151,160,137, 91, 90, 15,131, 13,201 ,95,
   96, 53,194,233,  7,225,140, 36,103, 30,
   69,142,  8, 99, 37,240, 21, 10, 23,190, 
    6,148,247,120,234, 75,  0, 26,197, 62,
   94,252,219,203,117, 35, 11, 32, 57,177,
   33, 88,237,149, 56, 87,174, 20,125,136,
  171,168, 68,175, 74,165, 71,134,139, 48,
   27,166, 77,146,158,231, 83,111,229,122,
   60,211,133,230,220,105, 92, 41, 55, 46,
  245, 40,244,102,143, 54, 65, 25, 63,161,
    1,216, 80, 73,209, 76,132,187,208, 89,
   18,169,200,196,135,130,116,188,159, 86,
  164,100,109,198,173,186,  3, 64, 52,217,
  226,250,124,123,  5,202, 38,147,118,126,
  255, 82, 85,212,207,206, 59,227, 47, 16,
   58, 17,182,189, 28, 42,223,183,170,213,
  119,248,152,  2, 44,154,163, 70,221,153,
  101,155,167, 43,172,  9,129, 22, 39,253,
   19, 98,108,110, 79,113,224,232,178,185,
  112,104,218,246, 97,228,251, 34,242,193,
  238,210,144, 12,191,179,162,241, 81, 51,
  145,235,249, 14,239,107, 49,192,214, 31,
  181,199,106,157,184, 84,204,176,115,121,
   50, 45,127,  4,150,254,138,236,205, 93,
  222,114, 67, 29, 24, 72,243,141,128,195,
   78, 66,215, 61,156,180
};

static int perm[512];

static bool initDone = false;

static void init() {
  // To remove the need for index wrapping, double the permutation table length
  for(int i=0; i<512; i++) perm[i]=p[i & 255]; 

  initDone = true;
}

float simplexNoise(const Vector3 &in) {
  if (!initDone) init();

  float n0, n1, n2, n3; // Noise contributions from the four corners
  float s = (in.x+in.y+in.z)/3.0;
  int i = std::floor(in.x+s);
  int j = std::floor(in.y+s);
  int k = std::floor(in.z+s);

  float t = (i+j+k)/6.0;
  Vector3 V0(i-t, j-t, k-t);                   // unskew the cell origin back to (x,y,z) space
  Vector3 v0(in.x-V0.x, in.y-V0.y, in.z-V0.z); // distance from cell origin

  // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
  // Determine which simplex we are in.
  int i1, j1, k1;
  // Offsets for second corner of simplex in (i,j,k) coords
  int i2, j2, k2;
  // Offsets for third corner of simplex in (i,j,k) coords
  if (v0.x>=v0.y) {
    if      (v0.y>=v0.z) { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; } // X Y Z order
    else if (v0.x>=v0.z) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; } // X Z Y order
    else                 { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; } // Z X Y order
  } else { // x0<y0
    if      (v0.y<v0.z)  { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; } // Z Y X order
    else if (v0.x<v0.z)  { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; } // Y Z X order
    else                 { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; } // Y X Z order
  }

  // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
  // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
  // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
  // c = 1/6.

  Vector3 v1(v0.x - i1 + 1.0/6.0, v0.y - j1 + 1.0/6.0, v0.z - k1 + 1.0/6.0);
  Vector3 v2(v0.x - i2 + 2.0/6.0, v0.y - j2 + 2.0/6.0, v0.z - k2 + 2.0/6.0);
  Vector3 v3(v0.x -  1 + 3.0/6.0, v0.y -  1 + 3.0/6.0, v0.z -  1 + 3.0/6.0);

  // Work out the hashed gradient indices of the four simplex corners
  int ii = i & 255;
  int jj = j & 255;
  int kk = k & 255;
  int gi0 = perm[ii+perm[jj+perm[kk]]] % 12;
  int gi1 = perm[ii+i1+perm[jj+j1+perm[kk+k1]]] % 12;
  int gi2 = perm[ii+i2+perm[jj+j2+perm[kk+k2]]] % 12;
  int gi3 = perm[ii+1+perm[jj+1+perm[kk+1]]] % 12;

  // Calculate the contribution from the four corners
  float t0 = 0.6 - v0.GetSquareMag();
  float t1 = 0.6 - v1.GetSquareMag();
  float t2 = 0.6 - v2.GetSquareMag();
  float t3 = 0.6 - v3.GetSquareMag();

  if(t0 < 0) n0 = 0.0; else n0 = t0 * t0 * t0 * t0 * grad3[gi0].Dot(v0);
  if(t1 < 0) n1 = 0.0; else n1 = t1 * t1 * t1 * t1 * grad3[gi1].Dot(v1);
  if(t2 < 0) n2 = 0.0; else n2 = t2 * t2 * t2 * t2 * grad3[gi2].Dot(v2);
  if(t3 < 0) n3 = 0.0; else n3 = t3 * t3 * t3 * t3 * grad3[gi3].Dot(v3);

  return 32.0*(n0+n1+n2+n3);
}

