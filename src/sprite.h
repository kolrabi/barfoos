#ifndef BARFOOS_SPRITE_H
#define BARFOOS_SPRITE_H

#include "animation.h"
#include "texture.h"

#include <list>
#include <vector>
#include <string>

/** A sprite. */
struct Sprite {
  /** Texture to use when drawing the sprite. */
  const Texture *texture;
  const Texture *emissiveTexture;
  
  /** Sprite width. */
  float width;
  
  /** Sprite height. */
  float height;
  
  /** Horizontal offset used when drawing. */
  float offsetX;
  
  /** Vertical offset used when drawing. */
  float offsetY;
  
  /** Is this to be drawn as a vertical billboard? */
  bool vertical;
  
  /** Total number of frames in texture. */
  uint32_t totalFrames;
  
  /** Current animation frame. */
  uint32_t currentFrame;
  
  /** Current animation time. */
  float t;
  
  /** Active animation index. */
  uint32_t currentAnimation;
  
  /** All animations. */
  std::vector<Animation> animations;
  
  std::list<ID> animQueue;

  // TODO: animation names instead of ids
  
  Sprite() :
    texture(nullptr),
    emissiveTexture(nullptr),
    width(1.0),
    height(1.0),
    offsetX(0.0),
    offsetY(0.0),
    vertical(false),
    totalFrames(1),
    currentFrame(0),
    t(0.0),
    currentAnimation(0),
    animations(0),
    animQueue(0)
  {}  

  Sprite(const std::string &textureName, const std::string &emissiveTextureName = "") :
    texture(loadTexture(textureName)),
    emissiveTexture(emissiveTextureName == "" ? nullptr : loadTexture(emissiveTextureName)),
    width(1.0),
    height(1.0),
    offsetX(0.0),
    offsetY(0.0),
    vertical(false),
    totalFrames(1),
    currentFrame(0),
    t(0.0),
    currentAnimation(0),
    animations(0),
    animQueue(0)
  {}  
  
  Sprite(const Sprite &) = default;
  Sprite &operator=(const Sprite &) = default;
  
  /** Update the sprite.
    * Advances animation time and frame. Resets animation to 0 when
    * last frame of animation reached.
    * @param deltaT Amount by which to advance the sprite.
    */
  void Update(float deltaT) {
    if (this->animations.empty()) return;

    // increase frame time
    const Animation &anim = this->animations[currentAnimation];
    t += anim.fps * deltaT;
    currentFrame = t;

    if (t > anim.frameCount + anim.firstFrame) {
      // end of animation reached
      uint32_t next = currentAnimation;

      if (!animQueue.empty()) {
        // switch to next animation in queue
        next = animQueue.front();
        animQueue.pop_front();
      }
      
      currentAnimation = next;
      t = t - currentFrame + this->animations[next].firstFrame;
    	currentFrame = t;
    }
  }
 
  void StartAnim(ID anim) {
    this->animQueue.clear();
    
    if (anim >= this->animations.size()) return;
    
    this->currentAnimation = anim;
    this->t = this->animations[anim].firstFrame;
    this->currentFrame = t;
  }
  
  void QueueAnim(ID anim) {
    if (anim >= this->animations.size()) {
      return;
    }
    this->animQueue.push_back(anim);
  }
  
  friend Serializer &operator << (Serializer &ser, const Sprite &sprite);
};

#endif

