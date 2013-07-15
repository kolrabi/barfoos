#ifndef BARFOOS_ANIMATION_H
#define BARFOOS_ANIMATION_H

/** Animation information. */
struct Animation {
  /** Frame index where the animation begins. */
  size_t firstFrame = 0;

  /** Total number of frames in the animation. */
  size_t frameCount = 0;
  
  /** Frames per second. */
  float fps         = 0;
  
  Animation() {}
  Animation(size_t firstFrame, size_t frameCount, float fps) : firstFrame(firstFrame), frameCount(frameCount), fps(fps) {}
};

#endif

