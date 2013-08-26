#include "audio.h"

#include "game.h"
#include "player.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
//#include <ogg/ogg.h>
//#include <vorbis/codec.h>
//#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

Audio::Audio() :
  device(nullptr),
  context(nullptr),
  isInited(false),
  player(nullptr)
{
}

Audio::~Audio() {
  if (this->isInited) this->Deinit();
}

bool Audio::Init() {
  const ALCchar *deviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
  this->device = alcOpenDevice(deviceName);
  if (!this->device) {
    Log("Could not open OpenAL device: %04x\n", alGetError());
    return true;
  }

  this->context = alcCreateContext((ALCdevice*)this->device, nullptr);

  alcMakeContextCurrent((ALCcontext*)this->context);
  alcProcessContext((ALCcontext*)this->context);

  return this->isInited = true;
}

void Audio::Deinit() {
  if (!this->isInited) return;

  for (auto &b:this->buffers) delete b.second;
  this->buffers.clear();

  this->sources.clear();
  alcMakeContextCurrent((ALCcontext*)this->context);
  alcDestroyContext((ALCcontext*)this->context);

  alcCloseDevice((ALCdevice*)this->device);

  this->context = nullptr;
  this->device = nullptr;

  this->isInited = false;
}

void Audio::Update(Game &) {
  if (!this->isInited) return;

  Vector3 listenerPos(      this->player ? this->player->GetSmoothEyePosition() : Vector3(0,0,0));
  Vector3 listenerForward(  this->player ? this->player->GetForward()           : Vector3(0,0,1));
  Vector3 listenerVelocity( this->player ? this->player->GetVelocity()          : Vector3(0,0,0));

  ALfloat listenerPosf[]      = { listenerPos.x,      listenerPos.y,      listenerPos.z      };
  ALfloat listenerVelocityf[] = { listenerVelocity.x, listenerVelocity.y, listenerVelocity.z };
  ALfloat listenerOrif[]      = { listenerForward.x,  listenerForward.y,  listenerForward.z,  0, 1, 0 };

  alListenerfv(AL_POSITION,     listenerPosf);
  alListenerfv(AL_VELOCITY,     listenerVelocityf);
  alListenerfv(AL_ORIENTATION,  listenerOrif);

  //Log("listening from %f %f %f\n", listenerPos.x, listenerPos.y, listenerPos.z);

  auto iter = this->sources.begin();
  while(iter != this->sources.end()) {
    if ((*iter)->IsStillPlaying()) {
      iter ++;
    } else {
      iter = this->sources.erase(iter);
    }
  }
}

Audio::Buffer *Audio::GetSoundBuffer(const std::string &name) {
  if (!this->isInited) return nullptr;

  auto iter = this->buffers.find(name);
  if (iter == this->buffers.end()) {
    this->buffers[name] = Audio::Buffer::LoadOgg(name);
  }
  return this->buffers[name];
}

void Audio::PlaySound(const std::string &name, const Vector3 &pos, float pitch) {
  if (!this->isInited) return;
  if (name == "") return;

  //Log("trying to play sound %s\n", name.c_str());
  this->sources.push_back(std::shared_ptr<Audio::Source>(new Audio::Source(this->GetSoundBuffer(name), pos, pitch)));
}

// =========================================================================

Audio::Buffer *Audio::Buffer::LoadOgg(const std::string &name) {
  Audio::Buffer *buffer = new Audio::Buffer();

  Log("Loading sound %s\n", name.c_str());
  FILE *f = openAsset("audio/"+name+".ogg");
  if (!f) {
    perror(name.c_str());
    return buffer;
  }

  OggVorbis_File oggFile;
  if (ov_open(f, &oggFile, NULL, 0) != 0) {
    Log("%s: ov_open failed", name.c_str());
    fclose(f);
    return buffer;
  }

  vorbis_info *info = ov_info(&oggFile, -1);
  ALuint format;
  if (info->channels == 1)      format = AL_FORMAT_MONO16;
  else if (info->channels == 2) format = AL_FORMAT_STEREO16;
  else {
    Log("%s: ogg has too many channels: %u", name.c_str(), info->channels);
    ov_clear(&oggFile);
    return buffer;
  }

  ALuint rate = info->rate;
  ogg_int64_t length = ov_pcm_total(&oggFile, -1) * info->channels * 2;
  char *data = new char[length];
  ogg_int64_t readPos = 0;
  int bitStream = -1;

  while(readPos < length) {
    long readCount = ov_read(&oggFile, data + readPos, length - readPos, 0, 2, 1, &bitStream);
    if (readCount <= 0) break;
    readPos += readCount;
  }

  Log("Read %llu / %llu bytes from sound %s\n", readPos, length, name.c_str());
  Log("Format is: %u channels, %u hz\n", info->channels, rate);

  alBufferData(buffer->buffer, format, data, readPos, rate);

  delete [] data;
  ov_clear(&oggFile);

  return buffer;
}

Audio::Buffer::Buffer() :
  buffer(0)
{
  alGenBuffers(1, &this->buffer);
}

Audio::Buffer::~Buffer() {
  if (this->buffer)
    alDeleteBuffers(1, &this->buffer);
}

// =========================================================================

Audio::Source::Source(Buffer *buffer, const Vector3 &pos, float pitch) :
  source(0)
{
  alGenSources(1, &this->source);
  //Log("alGenSources: %04x\n", alGetError());

  alSource3f(this->source, AL_POSITION, pos.x, pos.y, pos.z);
  alSourcef(this->source, AL_PITCH, pitch);
  alSourcei(this->source, AL_BUFFER, buffer->GetBuffer());
  //Log("alSourcei: %04x\n", alGetError());
  alSourcePlay(this->source);
  //Log("alSourcePlay: %04x\n", alGetError());

  //Log("playing source %u with buffer %u at pos %f %f %f\n", this->source, buffer->GetBuffer(), pos.x, pos.y, pos.z);
}

Audio::Source::~Source() {
  if (this->source) alDeleteSources(1, &this->source);
}

bool Audio::Source::IsStillPlaying() const {
  if (!this->source) return false;

  ALint state;
  alGetSourcei(this->source, AL_SOURCE_STATE, &state);

  return state != AL_STOPPED;
}
