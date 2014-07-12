#ifndef BARFOOS_SMOOTH_H
#define BARFOOS_sMOOTH_H

template<class T>
struct Smooth {

  Smooth(float f, const T &v = T() ) : 
    target(v),
    current(v),
    f(f)
  {}

  struct Smooth<T> &operator=(T value) {
    this->target = value;
    return *this;
  }
  
  void Update(float dt) {
    if (dt > 1.0 / this->f) {
      this->SnapTo(this->target);
    } else {
      this->current = this->current + (this->target - this->current) * dt * this->f;
    }
  }
  
  void SnapTo(T value) {
    this->target  = value;
    this->current = value;
  }

  T operator+(const T &o) const { return this->current + o; }
  T operator-(const T &o) const { return this->current - o; }
  
  operator const T&()     const { return this->current; }
  operator T()                  { return this->current; }
  
  const T &GetValue()     const { return this->current; }
  const T &GetTarget()    const { return this->target;  }
  
private:

  T target, current;
  float f;
};

#endif

