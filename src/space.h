#ifndef BARFOOS_SPACE_H
#define BARFOOS_SPACE_H

// ====================================================================

enum Axis : int {
  X = (1<<0),
  Y = (1<<1),
  Z = (1<<2),
  Horizontal = X | Z
};

// ====================================================================

enum Corner : int8_t {
  CornerX = 1,
  CornerY = 2,
  CornerZ = 4,

  Corner000 = 0,
  Corner100 = CornerX,
  Corner010 = CornerY,
  Corner110 = CornerX | CornerY,
  Corner001 = CornerZ,
  Corner101 = CornerX | CornerZ,
  Corner011 = CornerY | CornerZ,
  Corner111 = CornerX | CornerY | CornerZ,
};

// ====================================================================

enum class Side : int8_t {
  Right       = 0,
  Left        = 1,
  Up          = 2,
  Down        = 3,
  Forward     = 4,
  Backward    = 5,
  InvalidSide = -1
};

static inline Side operator-(Side side) {
  return (Side)(((int)side)^1);
}

static inline uint8_t operator << (uint8_t n, Side side) {
  return (n << (uint8_t)side);
}

static inline Side Rotate(Side side) {
  switch(side) {
    case Side::Right: return Side::Backward;
    case Side::Backward: return Side::Left;
    case Side::Left: return Side::Forward;
    default: return side;
  }
}

#endif

