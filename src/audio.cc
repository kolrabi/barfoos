#include "common.h"

#include "audio.h"

#include "fileio.h"
#include "game.h"
#include "player.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <vorbis/vorbisfile.h>

/** C'tor. */
Audio::Audio() :
  device(nullptr),
  context(nullptr),
  isInited(false),
  player(nullptr)
{
}

/** D'tor. Deinitializes audio if needed. */
Audio::~Audio() {
  if (this->isInited) this->Deinit();
}

/** Initialize audio.
 * @return true if successful.
 */
bool
Audio::Init() {
  const ALCchar *deviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

  Log("Trying to open audio device '%s'...\n", deviceName);
  this->device = alcOpenDevice(deviceName);
  if (!this->device) {
    Log("Could not open OpenAL device: %04x\n", alGetError());
    return true;
  }

  Log("Creating audio context...\n", deviceName);
  this->context = alcCreateContext((ALCdevice*)this->device, nullptr);
  if (!this->context) {
    Log("Could not create audio context: %04x\n", alGetError());
    alcCloseDevice((ALCdevice*)this->device);
    this->device = nullptr;
    return true;
  }

  alcMakeContextCurrent((ALCcontext*)this->context);
  alcProcessContext((ALCcontext*)this->context);

  return this->isInited = true;
}

/** Deinitialize audio.
 * Stops all sources, releases all buffers.
 * Destroys audio context and closes device.
 */
void
Audio::Deinit() {
  if (!this->isInited) return;

  Log("Deinitializing audio...\n");

  Log("Clearing out audio sources...\n");
  this->sources.clear();

  Log("Destroying audio buffers...\n");
  for (auto &b:this->buffers) delete b.second;
  this->buffers.clear();

  Log("Destroying audio context...\n");
  alcMakeContextCurrent((ALCcontext*)this->context);
  alcDestroyContext((ALCcontext*)this->context);

  Log("Closing audio device...\n");
  alcCloseDevice((ALCdevice*)this->device);

  this->context = nullptr;
  this->device = nullptr;

  this->isInited = false;
}

/** Update game audio.
 * Updates listener position and velocity to that of the player
 * if it has been set. Removes all finished sources. 
 */
void 
Audio::Update(Game &) {
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

/** Retrieve the audio buffer for a sound.
 * Loads it if neccessary.
 * @param name Name of sound to load.
 * @return The loaded buffer (empty if sound is not found). nullptr if Audio was not initialized.
 */
Audio::Buffer *
Audio::GetSoundBuffer(const std::string &name) {
  if (!this->isInited) return nullptr;

  auto iter = this->buffers.find(name);
  if (iter == this->buffers.end()) {
    this->buffers[name] = Audio::Buffer::LoadOgg(name);
  }
  return this->buffers[name];
}

/** Play a sound.
  * @param name Name of sound to play.
  * @param pos Initial position of sound source.
  * @param loop If true loop the sound indefinitely. Don't forget to store the
  *             source pointer or you won't be able to ever stop it.
  * @param pitch Relative pitch of sound.
  * @return The new source playing the requested sound.
  */
std::shared_ptr<Audio::Source>
Audio::PlaySound(const std::string &name, const Vector3 &pos, const Vector3 &velocity, bool loop, float volume, float pitch) {

  if (!this->isInited) return nullptr;
  if (name == "") return nullptr;

  //Log("trying to play sound %s\n", name.c_str());
  std::shared_ptr<Audio::Source> source(new Audio::Source(this->GetSoundBuffer(name), pos, velocity, loop, volume, pitch));
  this->sources.push_back(source);
  return source;
}

// =========================================================================

/** Load and decode an ogg vorbis file and put it in a buffer.
  * @param name Name of sound asset to load.
  * @return The loaded sound asset in an audio buffer.
  */
Audio::Buffer *
Audio::Buffer::LoadOgg(const std::string &name) {
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

/** C'tor. */
Audio::Buffer::Buffer() :
  buffer(0)
{
  alGenBuffers(1, &this->buffer);
}

/** D'tor. */
Audio::Buffer::~Buffer() {
  if (this->buffer)
    alDeleteBuffers(1, &this->buffer);
}

// =========================================================================

Audio::Source::Source(Buffer *buffer, const Vector3 &pos, const Vector3 &velocity, bool loop, float volume, float pitch) :
  source(0)
{
  alGenSources(1, &this->source);

  alSource3f(this->source, AL_POSITION, pos.x, pos.y, pos.z);
  alSource3f(this->source, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
  alSourcef(this->source, AL_GAIN, volume);
  alSourcef(this->source, AL_PITCH, pitch);
  alSourcei(this->source, AL_BUFFER, buffer->buffer);
  alSourcei(this->source, AL_LOOPING, loop );
  alSourcePlay(this->source);

  //Log("playing source %u with buffer %u at pos %f %f %f\n", this->source, buffer->GetBuffer(), pos.x, pos.y, pos.z);
}

/** D'tor. */
Audio::Source::~Source() {
  if (this->source) alDeleteSources(1, &this->source);
}

/** Check playing state of a source.
  * @return true if source is still playing.
  */
bool 
Audio::Source::IsStillPlaying() const {
  if (!this->source) return false;

  ALint state;
  alGetSourcei(this->source, AL_SOURCE_STATE, &state);

  return state != AL_STOPPED;
}

/** Update the position of a source.
  * @param pos The new position.
  */
void 
Audio::Source::SetPosition(const Vector3 &pos) {
  alSource3f(this->source, AL_POSITION, pos.x, pos.y, pos.z);
}

/** Update the velocity of a source.
  * @param pos The new velocity.
  */
void
Audio::Source::SetVelocity(const Vector3 &velocity) {
  alSource3f(this->source, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
}

/** Update the pitch of a source.
  * @param pos The new pitch.
  */
void
Audio::Source::SetPitch(float pitch) {
  alSourcef(this->source, AL_PITCH, pitch);
}

/** Update the volume of a source.
  * @param pos The new volume.
  */
void
Audio::Source::SetVolume(float volume) {
  alSourcef(this->source, AL_GAIN, volume);
}
