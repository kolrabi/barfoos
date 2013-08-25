#ifndef BARFOOS_AUDIO_H
#define BARFOOS_AUDIO_H

#include "common.h"

#include "vector3.h"

#include <unordered_map>

class Audio final {

public:

  Audio();
  ~Audio();

  bool Init();

  void Update(Game &);

  void SetPlayer(const Player *player) { this->player = player; }

  void PlaySound(const std::string &name, const Vector3 &pos = Vector3(0,0,0), float pitch = 1.0);

private:

  class Buffer {
  public:

    static Buffer *LoadOgg(const std::string &name);

    ~Buffer();

    unsigned int GetBuffer() const { return this->buffer; }

  private:

    Buffer();

    unsigned int buffer;
  };

  class Source {
  public:
    Source(Buffer *buffer, const Vector3 &pos, float pitch);
    ~Source();

    bool IsStillPlaying() const;

  private:
    unsigned int source;
  };

  void *device;
  void *context;

  bool isInited;
  const Player *player;

  std::unordered_map<std::string, Buffer *> buffers;
  std::vector<std::shared_ptr<Source>> sources;

  Buffer *GetSoundBuffer(const std::string &name);
  void Deinit();
};

#endif

