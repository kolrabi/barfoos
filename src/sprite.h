#ifndef BARFOOS_SPRITE_H
#define BARFOOS_SPRITE_H

#include "animation.h"

/** A sprite. */
struct Sprite {
  /** Texture to use when drawing the sprite. */
  const Texture *texture = nullptr;
  
  /** Sprite width. */
  float width = 1.0;
  
  /** Sprite height. */
  float height = 1.0;
  
  /** Horizontal offset used when drawing. */
  float offsetX = 0.0;
  
  /** Vertical offset used when drawing. */
  float offsetY = 0.0;
  
  /** Is this to be drawn as a vertical billboard? */
  bool vertical = false;
  
  /** Total number of frames in texture. */
  size_t totalFrames = 1;
  
  /** Current animation frame. */
  size_t currentFrame = 0;
  
  /** Current animation time. */
  float t = 0;
  
  /** Active animation index. */
  size_t currentAnimation = 0;
  
  /** All animations. */
  std::vector<Animation> animations;
  
  std::list<size_t> animQueue;
  
  /** Update the sprite.
    * Advances animation time and frame. Resets animation to 0 when
    * last frame of animation reached.
    * @param deltaT Amount by which to advance the sprite.
    */
  void Update(float deltaT) {
    if (this->animations.size() > 0) {
      const Animation &anim = this->animations[currentAnimation];
      t += anim.fps * deltaT;
      currentFrame = t;
      if (t > anim.frameCount + anim.firstFrame) {
        size_t next = 0;
        if (animQueue.size() != 0) {
          next = animQueue.front();
          animQueue.pop_front();
        }
        
        currentAnimation = next;
        t = t - currentFrame + this->animations[next].firstFrame;
      	currentFrame = t;
      }
    }
  }
 
  void StartAnim(size_t anim) {
    this->animQueue.clear();
    
    if (anim >= this->animations.size()) return;
    
    this->currentAnimation = anim;
    this->t = this->animations[anim].firstFrame;
    this->currentFrame = t;
  }
  
  void QueueAnim(size_t anim) {
    if (anim >= this->animations.size()) {
      return;
    }
    this->animQueue.push_back(anim);
  }
};

#endif

