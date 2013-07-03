#ifndef BARFOOS_SPACE_H
#define BARFOOS_SPACE_H

// ====================================================================

enum Axis {
  X = (1<<0),
  Y = (1<<1),
  Z = (1<<2),
  Horizontal = X | Z
};

static inline std::ostream & operator<< (std::ostream &out, Axis a) {
  out << "Axis::";
  if (a & Axis::X) out << "X";
  if (a & Axis::Y) out << "Y";
  if (a & Axis::Z) out << "Z";
  return out;
}

// ====================================================================

enum Corner {
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

enum class Side {
  Right = 0,
  Left  = 1,
  Up = 2,
  Down = 3,
  Forward = 4,
  Backward = 5,
  InvalidSide = -1
};

static inline std::ostream & operator<< (std::ostream &out, Side side) {
  switch(side) {
    case Side::Right:    out << "+X"; break;
    case Side::Left:     out << "-X"; break;
    case Side::Up:       out << "+Y"; break;
    case Side::Down:     out << "-Y"; break;
    case Side::Forward:  out << "+Z"; break;
    case Side::Backward: out << "-Z"; break;
    default:       out << "<?" "?>";
  }
  return out;
}

static inline Side operator-(Side side) {
  return (Side)(((int)side)^1);
}

static inline uint8_t operator << (uint8_t n, Side side) {
  return (n << (uint8_t)side);
}

#endif

