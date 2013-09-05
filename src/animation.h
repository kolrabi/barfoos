#ifndef BARFOOS_ANIMATION_H
#define BARFOOS_ANIMATION_H

/** Animation information. */
struct Animation {
  /** Frame index where the animation begins. */
  size_t firstFrame;

  /** Total number of frames in the animation. */
  size_t frameCount;
  
  /** Frames per second. */
  float fps;
  
  /** Default animation, one frame starting from 0. */
  Animation() :
    firstFrame(0), 
    frameCount(1), 
    fps(0) 
  {}
  
  /** Animation c'tor
   * @param firstFrame Index of first frame in animation.
   * @param frameCount Total number of frames in animation.
   * @param fps Frames per seconds.
   */
  Animation(size_t firstFrame, size_t frameCount, float fps) : 
    firstFrame(firstFrame), 
    frameCount(frameCount), 
    fps(fps) 
  {}
};

#endif

