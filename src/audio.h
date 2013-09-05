#ifndef BARFOOS_AUDIO_H
#define BARFOOS_AUDIO_H

#include "vector3.h"

#include <unordered_map>
#include <vector>

/** Audio subsystem. */
class Audio final {
public:

  /** Audio buffer. */
  class Buffer {
  public:


  private:

    friend class ::Audio;

    static Buffer *LoadOgg(const std::string &name);

    Buffer();
    ~Buffer();

    /** OpenAL handle to buffer. */
    unsigned int buffer;
  };


  /** Audio source. */
  class Source {
  public:

    ~Source();
    
    bool IsStillPlaying() const;

    void SetPosition(const Vector3 &pos);
    void SetVelocity(const Vector3 &velocity);
    void SetPitch(float pitch);
    void SetVolume(float volume);

  private:

    friend class ::Audio;

    Source(Buffer *buffer, const Vector3 &pos, const Vector3 &velocity, bool loop, float volume, float pitch);

    /** OpenAL handle to source. */
    unsigned int source;
  };

  Audio();
  ~Audio();

  bool Init();

  void Update(Game &);

  void SetPlayer(const Player *player) { this->player = player; }

  std::shared_ptr<Source> PlaySound(const std::string &name, const Vector3 &pos, const Vector3 &velocity = Vector3(), bool loop = false, float volume = 1.0, float pitch = 1.0);

private:

  /** OpenAL device. */
  void *device;

  /** OpenAL context. */
  void *context;

  /** True if initialization was successful. */
  bool isInited;

  /** The player object used as a listener. */
  const Player *player;

  /** All loaded audio buffers. */
  std::unordered_map<std::string, Buffer *> buffers;

  /** All currently playing audio sources. */
  std::vector<std::shared_ptr<Source>> sources;

  Buffer *GetSoundBuffer(const std::string &name);
  void Deinit();
};

#endif

